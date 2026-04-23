#include "Sequencer.h"

#include "app/Types.h"
#include "synth/events/Events.h"

#include <cassert>
#include <cmath>
#include <cstdint>

namespace app::sequencer {

using synth::events::ScheduledEventOrder;

namespace {

// =====================
// Event Factories
// =====================

ScheduledEvent makeNoteOffEvent(uint8_t note, uint32_t sampleOffset, ScheduledEventOrder order) {
  ScheduledEvent evt{};
  evt.sampleOffset = sampleOffset;
  evt.order = order;
  evt.kind = ScheduledEvent::Kind::MIDI;
  evt.data.midi.type = MIDIEvent::Type::NoteOff;
  evt.data.midi.data.noteOff.note = note;
  evt.data.midi.data.noteOff.velocity = 0;
  return evt;
}

ScheduledEvent makeNoteOnEvent(uint8_t note, uint8_t velocity, uint32_t sampleOffset) {
  ScheduledEvent evt{};
  evt.sampleOffset = sampleOffset;
  evt.order = ScheduledEventOrder::NoteOn;
  evt.kind = ScheduledEvent::Kind::MIDI;
  evt.data.midi.type = MIDIEvent::Type::NoteOn;
  evt.data.midi.data.noteOn.note = note;
  evt.data.midi.data.noteOn.velocity = velocity;
  return evt;
}

ScheduledEvent
makeParamEvent(uint8_t paramID, float value, uint32_t sampleOffset, ScheduledEventOrder order) {
  ScheduledEvent evt{};
  evt.sampleOffset = sampleOffset;
  evt.order = order;
  evt.kind = ScheduledEvent::Kind::Param;
  evt.data.param.id = paramID;
  evt.data.param.value = value;
  return evt;
}

uint32_t beatToSampleOffset(double beat, const SequencerBlockWindow& block) {
  double blockBeat = block.endBeat - block.startBeat;

  if (block.numFrames == 0 || blockBeat <= 0.0)
    return 0;

  double normalized = (beat - block.startBeat) / blockBeat;
  normalized = std::clamp(normalized, 0.0, 1.0);

  double scaled = std::floor(normalized * static_cast<double>(block.numFrames));
  return static_cast<uint32_t>(scaled);
}

// =====================
// P-Unlock
// =====================

/* Resolves pending unlocks against the current step.
 * For each pending unlock:
 *   - held:    current step also locks this param — keep entry, lockedValue updated in applyParamLocks
 *   - drop:    user edited the param since lock fired — clear entry, leave current value
 *   - fire:    param unchanged since lock — emit restore, clear entry
 *
 * NOTE: Locked value comparison is only safe while engine.params[] is written
 * exclusively via setParam/setParamDeferred. If any render-path code ever writes
 * back to params[] (i.e. param smoothing), replace with write serials.
 * Floating point equality is too fragile otherwise.
*/

void resolvePendingUnlocks(LaneState& laneState,
                           LaneEvents& laneOut,
                           const LaneContext& ctx,
                           const StepEvent& currentStep,
                           uint32_t sampleOffset) {
  for (uint32_t i = 0; i < MAX_PENDING_UNLOCKS; ++i) {
    ParamUnlock& unlock = laneState.unlocks.entries[i];
    if (!unlock.pending)
      continue;

    // Hold — current step also locks this param
    bool held = false;
    for (uint8_t l = 0; l < currentStep.numLocks; ++l) {
      if (currentStep.locks[l].paramID == unlock.paramID) {
        held = true;
        break;
      }
    }
    if (held)
      continue;

    // Drop — user edited the param since lock was applied
    float currentValue = ctx.getParamCallback(unlock.paramID, ctx.getParamCtx);
    if (currentValue != unlock.lockedValue) {
      unlock.pending = false;
      continue;
    }

    // Fire — restore to base value
    laneOut.push(makeParamEvent(unlock.paramID,
                                unlock.restoreValue,
                                sampleOffset,
                                ScheduledEventOrder::ParamUnlock));
    unlock.pending = false;
  }
}

// Applies p-locks for the current step.
// For each lock:
//   - if a pending unlock exists for this param (consecutive lock): update lockedValue, emit lock
//   - otherwise: snapshot base value via getParam, create pending unlock, emit lock
void applyParamLocks(const StepEvent& step,
                     LaneState& laneState,
                     LaneEvents& laneOut,
                     const LaneContext& ctx,
                     uint32_t sampleOffset) {

  for (uint8_t l = 0; l < step.numLocks; ++l) {
    const ParamLock& lock = step.locks[l];

    ParamUnlock* existing = nullptr;
    for (uint32_t i = 0; i < MAX_PENDING_UNLOCKS; ++i) {
      if (laneState.unlocks.entries[i].pending &&
          laneState.unlocks.entries[i].paramID == lock.paramID) {
        existing = &laneState.unlocks.entries[i];
        break;
      }
    }

    if (existing) {
      existing->lockedValue = lock.value;
    } else {
      for (uint32_t i = 0; i < MAX_PENDING_UNLOCKS; ++i) {
        ParamUnlock& entry = laneState.unlocks.entries[i];
        if (!entry.pending) {
          entry.paramID = lock.paramID;
          entry.restoreValue = ctx.getParamCallback(lock.paramID, ctx.getParamCtx);
          entry.lockedValue = lock.value;
          entry.pending = true;
          break;
        }
      }
    }

    laneOut.push(
        makeParamEvent(lock.paramID, lock.value, sampleOffset, ScheduledEventOrder::ParamLock));
  }
}

// Fires all pending unlocks unconditionally — used on stop.
void fireAllPendingUnlocks(LaneState& laneState, LaneEvents& laneOut) {
  for (uint32_t i = 0; i < MAX_PENDING_UNLOCKS; ++i) {
    ParamUnlock& unlock = laneState.unlocks.entries[i];
    if (!unlock.pending)
      continue;

    laneOut.push(
        makeParamEvent(unlock.paramID, unlock.restoreValue, 0, ScheduledEventOrder::ParamUnlock));
    unlock.pending = false;
  }
}

// =====================
// Step Processor
// =====================

void fireStep(uint32_t i,
              const LanePattern& pattern,
              LaneState& laneState,
              LaneEvents& laneOut,
              const LaneContext& ctx,
              double absStepBeat,
              double stepLengthBeats,
              const SequencerBlockWindow& block) {
  if (static_cast<int32_t>(i) == laneState.lastStep)
    return;

  const StepEvent& step = pattern.steps[i];
  uint32_t stepOffset = beatToSampleOffset(absStepBeat, block);

  resolvePendingUnlocks(laneState, laneOut, ctx, step, stepOffset);

  if (!step.active)
    return;

  applyParamLocks(step, laneState, laneOut, ctx, stepOffset);

  // NoteOn event
  if (step.noteOn) {
    if (laneState.noteActive) {
      // Kill prior
      if (!laneOut.push(
              makeNoteOffEvent(laneState.activeNote, stepOffset, ScheduledEventOrder::NoteOff)))
        return;
      laneState.noteActive = false;
      laneState.noteOffBeat = -1.0;
    }

    if (!laneOut.push(makeNoteOnEvent(step.note, step.velocity, stepOffset)))
      return;

    laneState.noteActive = true;
    laneState.activeNote = step.note;

    // GateNoteOff event
    double gateBeats = std::max(static_cast<double>(step.gate) * stepLengthBeats, MIN_GATE_BEAT);
    double scheduledNoteOffBeat = absStepBeat + gateBeats;

    if (step.legato) {
      laneState.noteOffBeat = -1; // hold note

    } else if (scheduledNoteOffBeat < block.endBeat) {
      uint32_t noteOffOffset = beatToSampleOffset(scheduledNoteOffBeat, block);
      if (!laneOut.push(
              makeNoteOffEvent(step.note, noteOffOffset, ScheduledEventOrder::GateNoteOff)))
        return;

      laneState.noteActive = false;
      laneState.noteOffBeat = -1.0;

    } else {
      laneState.noteOffBeat = scheduledNoteOffBeat;
    }
  }

  laneState.lastStep = static_cast<int32_t>(i);
}

// =====================
// Pattern Editing
// =====================

// Returns pointer to the inactive write buffer, or nullptr if out of range.
PatternSnapshot& getWriteBuffer(SequencerState& state) {
  uint32_t writeIndex = 1 - state.store.readIndex.load(std::memory_order_relaxed);
  assert(writeIndex < 2);

  return state.store.buffers[writeIndex];
}

VoidResult checkIsEditing(const SequencerState& state) {
  const char* errMsg = state.isEditing
                           ? nullptr
                           : "no active edit session; call seq.copyPattern or seq.newPattern first";

  return {state.isEditing, errMsg};
}

VoidResult checkLaneBounds(uint8_t lane) {
  const char* errMsg = lane < MAX_TRACKS ? nullptr : "lane index out of range";
  return {lane < MAX_TRACKS, errMsg};
}

VoidResult checkStepBounds(uint8_t step) {
  const char* errMsg = step < MAX_PATTERN_STEPS ? nullptr : "step index out of range";
  return {step < MAX_PATTERN_STEPS, errMsg};
}

VoidResult validateArgs(const SequencerState& state, uint8_t lane = 0, uint8_t step = 0) {
  VoidResult res;
  res = checkIsEditing(state);
  if (!res.ok)
    return res;

  res = checkLaneBounds(lane);
  if (!res.ok)
    return res;

  return checkStepBounds(step);
}

} // namespace

// =================
// Processing
// =================

void runSequencer(SequencerState& state, SequencerBlockWindow block, SequencerLaneEvents& evts) {
  const uint32_t readIndex = state.store.readIndex.load(std::memory_order_acquire);
  const PatternSnapshot& snapshot = state.store.buffers[readIndex];

  for (uint8_t laneIndex = 0; laneIndex < state.numLanes; ++laneIndex) {
    const LanePattern& pattern = snapshot.lanes[laneIndex];
    LaneState& laneState = state.lanes[laneIndex];
    LaneEvents& laneOut = evts.lanes[laneIndex];
    const LaneContext& ctx = state.laneCtxs[laneIndex];

    if (block.stoppedThisBlock) {
      if (laneState.noteActive)
        laneOut.push(makeNoteOffEvent(laneState.activeNote, 0, ScheduledEventOrder::NoteOff));

      fireAllPendingUnlocks(laneState, laneOut);
      laneState.noteActive = false;
      laneState.noteOffBeat = -1.0;
      laneState.lastStep = -1;
      continue;
    }

    if (laneState.noteActive && laneState.noteOffBeat >= block.startBeat &&
        laneState.noteOffBeat < block.endBeat) {
      uint32_t offset = beatToSampleOffset(laneState.noteOffBeat, block);
      laneOut.push(
          makeNoteOffEvent(laneState.activeNote, offset, ScheduledEventOrder::GateNoteOff));
      laneState.noteActive = false;
      laneState.noteOffBeat = -1.0;
    }

    if (pattern.numSteps == 0 || pattern.stepsPerBeat == 0)
      continue;

    const double stepLengthBeats = 1.0 / pattern.stepsPerBeat;
    const double patternLengthBeats = pattern.numSteps * stepLengthBeats;
    const double blockDuration = block.endBeat - block.startBeat;
    const double localStart = std::fmod(block.startBeat, patternLengthBeats);
    const double localEnd = localStart + blockDuration;
    const bool wraps = localEnd >= patternLengthBeats;

    if (!wraps) {
      for (uint32_t i = 0; i < pattern.numSteps; ++i) {
        const double stepBeat = i * stepLengthBeats;

        if (stepBeat >= localStart && stepBeat < localEnd) {
          double abs = block.startBeat + (stepBeat - localStart);
          fireStep(i, pattern, laneState, laneOut, ctx, abs, stepLengthBeats, block);
        }
      }
    } else {
      const double headEnd = localEnd - patternLengthBeats;

      for (uint32_t i = 0; i < pattern.numSteps; ++i) {
        const double stepBeat = i * stepLengthBeats;

        if (stepBeat >= localStart) { // tail
          double abs = block.startBeat + (stepBeat - localStart);
          fireStep(i, pattern, laneState, laneOut, ctx, abs, stepLengthBeats, block);
        }

        if (stepBeat < headEnd) { // head
          double abs = block.startBeat + (patternLengthBeats - localStart) + stepBeat;
          fireStep(i, pattern, laneState, laneOut, ctx, abs, stepLengthBeats, block);
        }
      }
    }
  }
}

// =====================
// Pattern Editing
// =====================

// TODO / IMPORTANT - Clamp input values!!!!!!

// Preps write buffer
VoidResult beginPatternEdit(SequencerState& state, bool copy) {
  if (state.isEditing)
    return {false, "Editing already in progress"};

  PatternSnapshot& writeBuf = getWriteBuffer(state);

  if (copy) {
    uint32_t readIndex = state.store.readIndex.load(std::memory_order_relaxed);
    writeBuf = state.store.buffers[readIndex];
  } else {
    writeBuf = PatternSnapshot{};
  }

  state.isEditing = true;
  return {true, nullptr};
}

// Swap write -> read buffer
VoidResult commitPattern(SequencerState& state) {
  auto res = checkIsEditing(state);
  if (!res.ok)
    return res;

  uint32_t writeIndex = 1 - state.store.readIndex.load(std::memory_order_relaxed);
  state.store.readIndex.store(writeIndex, std::memory_order_release);
  state.isEditing = false;
  return res;
}

VoidResult setStep(SequencerState& state, uint8_t lane, uint8_t step, const StepEvent& evt) {
  auto res = validateArgs(state, lane, step);
  if (!res.ok)
    return res;

  getWriteBuffer(state).lanes[lane].steps[step] = evt;
  return res;
}

VoidResult setStepActive(SequencerState& state, uint8_t lane, uint8_t step, bool active) {
  auto res = validateArgs(state, lane, step);
  if (!res.ok)
    return res;

  getWriteBuffer(state).lanes[lane].steps[step].active = active;
  return res;
}

VoidResult setStepNote(SequencerState& state, uint8_t lane, uint8_t step, uint8_t note) {
  auto res = validateArgs(state, lane, step);
  if (!res.ok)
    return res;

  getWriteBuffer(state).lanes[lane].steps[step].note = note;
  return res;
}

VoidResult setStepVelocity(SequencerState& state, uint8_t lane, uint8_t step, uint8_t velocity) {
  auto res = validateArgs(state, lane, step);
  if (!res.ok)
    return res;

  getWriteBuffer(state).lanes[lane].steps[step].velocity = velocity;
  return res;
}

VoidResult setStepNoteOn(SequencerState& state, uint8_t lane, uint8_t step, bool noteOn) {
  auto res = validateArgs(state, lane, step);
  if (!res.ok)
    return res;

  getWriteBuffer(state).lanes[lane].steps[step].noteOn = noteOn;
  return res;
}

VoidResult
setStepLock(SequencerState& state, uint8_t lane, uint8_t step, uint8_t paramID, float value) {
  auto res = validateArgs(state, lane, step);
  if (!res.ok)
    return res;

  StepEvent& s = getWriteBuffer(state).lanes[lane].steps[step];

  // Update existing lock for this paramID if present
  for (uint8_t i = 0; i < s.numLocks; ++i) {
    if (s.locks[i].paramID == paramID) {
      s.locks[i].value = value;
      return res;
    }
  }

  // Add new lock
  if (s.numLocks >= MAX_LOCKS_PER_STEP)
    return {false, "step lock capacity full"};

  s.locks[s.numLocks++] = {paramID, value};
  return res;
}

VoidResult clearStepLock(SequencerState& state, uint8_t lane, uint8_t step, uint8_t paramID) {
  auto res = validateArgs(state, lane, step);
  if (!res.ok)
    return res;

  StepEvent& s = getWriteBuffer(state).lanes[lane].steps[step];

  for (uint8_t i = 0; i < s.numLocks; ++i) {
    if (s.locks[i].paramID == paramID) {
      // Swap with last and decrement — order doesn't matter for locks
      s.locks[i] = s.locks[--s.numLocks];
      return res;
    }
  }

  return res;
}

VoidResult clearStepLocks(SequencerState& state, uint8_t lane, uint8_t step) {
  auto res = validateArgs(state, lane, step);
  if (!res.ok)
    return res;

  getWriteBuffer(state).lanes[lane].steps[step].numLocks = 0;
  return res;
}

// ===== Multi-value/full-pattern input =====
VoidResult
setActivePattern(SequencerState& state, uint8_t lane, const uint8_t* values, uint8_t count) {
  auto res = validateArgs(state, lane);
  if (!res.ok)
    return res;

  LanePattern& lp = getWriteBuffer(state).lanes[lane];
  if (count != lp.numSteps)
    return {false, "table length must match numSteps"};

  for (uint8_t i = 0; i < count; ++i) {
    lp.steps[i].active = values[i] != 0;
    lp.steps[i].noteOn = values[i] != 0;
  }
  return res;
}

VoidResult
setNotePattern(SequencerState& state, uint8_t lane, const uint8_t* values, uint8_t count) {
  auto res = validateArgs(state, lane);
  if (!res.ok)
    return res;

  LanePattern& lp = getWriteBuffer(state).lanes[lane];
  if (count != lp.numSteps)
    return {false, "table length must match numSteps"};

  for (uint8_t i = 0; i < count; ++i)
    lp.steps[i].note = values[i];
  return res;
}

VoidResult
setVelocityPattern(SequencerState& state, uint8_t lane, const uint8_t* values, uint8_t count) {
  auto res = validateArgs(state, lane);
  if (!res.ok)
    return res;

  LanePattern& lp = getWriteBuffer(state).lanes[lane];
  if (count != lp.numSteps)
    return {false, "table length must match numSteps"};

  for (uint8_t i = 0; i < count; ++i)
    lp.steps[i].velocity = values[i];
  return res;
}
} // namespace app::sequencer
