#include "Sequencer.h"
#include "app/Constants.h"
#include "app/Types.h"

#include <cassert>
#include <cmath>
#include <cstdint>

namespace app::sequencer {

namespace {

// =====================
// Event Factories
// =====================

SequencerEvent makeNoteOffEvent(uint8_t note) {
  SequencerEvent e{};
  e.kind = SequencerEvent::Kind::MIDI;
  e.data.midi.type = MIDIEvent::Type::NoteOff;
  e.data.midi.data.noteOff.note = note;
  e.data.midi.data.noteOff.velocity = 0;
  return e;
}

SequencerEvent makeNoteOnEvent(uint8_t note, uint8_t velocity) {
  SequencerEvent e{};
  e.kind = SequencerEvent::Kind::MIDI;
  e.data.midi.type = MIDIEvent::Type::NoteOn;
  e.data.midi.data.noteOn.note = note;
  e.data.midi.data.noteOn.velocity = velocity;
  return e;
}

SequencerEvent makeParamEvent(uint8_t paramID, float value) {
  SequencerEvent e{};
  e.kind = SequencerEvent::Kind::Param;
  e.data.param.id = paramID;
  e.data.param.value = value;
  return e;
}

// =====================
// P-Unlock
// =====================

// Resolves pending unlocks against the current step.
// For each pending unlock:
//   - held:    current step also locks this param — keep entry, lockedValue updated in applyParamLocks
//   - drop:    user edited the param since lock fired — clear entry, leave current value
//   - fire:    param unchanged since lock — emit restore, clear entry
void resolvePendingUnlocks(LaneState& laneState,
                           LaneEvents& laneOut,
                           const LaneContext& ctx,
                           const StepEvent& currentStep) {
  for (uint8_t i = 0; i < MAX_PENDING_UNLOCKS; ++i) {
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
    laneOut.push(makeParamEvent(unlock.paramID, unlock.restoreValue));
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
                     const LaneContext& ctx) {

  for (uint8_t l = 0; l < step.numLocks; ++l) {
    const ParamLock& lock = step.locks[l];

    ParamUnlock* existing = nullptr;
    for (uint8_t i = 0; i < MAX_PENDING_UNLOCKS; ++i) {
      if (laneState.unlocks.entries[i].pending &&
          laneState.unlocks.entries[i].paramID == lock.paramID) {
        existing = &laneState.unlocks.entries[i];
        break;
      }
    }

    if (existing) {
      existing->lockedValue = lock.value;
    } else {
      for (uint8_t i = 0; i < MAX_PENDING_UNLOCKS; ++i) {
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

    laneOut.push(makeParamEvent(lock.paramID, lock.value));
  }
}

// Fires all pending unlocks unconditionally — used on stop.
void fireAllPendingUnlocks(LaneState& laneState, LaneEvents& laneOut) {
  for (uint8_t i = 0; i < MAX_PENDING_UNLOCKS; ++i) {
    ParamUnlock& unlock = laneState.unlocks.entries[i];
    if (!unlock.pending)
      continue;
    laneOut.push(makeParamEvent(unlock.paramID, unlock.restoreValue));
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
              const LaneContext& ctx) {
  if (static_cast<int32_t>(i) == laneState.lastStep)
    return;

  const StepEvent& step = pattern.steps[i];

  resolvePendingUnlocks(laneState, laneOut, ctx, step); // always resolve at boundries

  if (!step.active)
    return;

  applyParamLocks(step, laneState, laneOut, ctx);

  if (step.noteOn) {
    if (laneState.noteActive) {
      if (!laneOut.push(makeNoteOffEvent(laneState.activeNote)))
        return;
    }
    if (!laneOut.push(makeNoteOnEvent(step.note, step.velocity)))
      return;

    laneState.noteActive = true;
    laneState.activeNote = step.note;
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

Result checkIsEditing(const SequencerState& state) {
  const char* errMsg = state.isEditing
                           ? nullptr
                           : "no active edit session; call seq.copyPattern or seq.newPattern first";

  return {state.isEditing, errMsg};
}

Result checkLaneBounds(uint8_t lane) {
  const char* errMsg = lane < MAX_TRACKS ? nullptr : "lane index out of range";
  return {lane < MAX_TRACKS, errMsg};
}

Result checkStepBounds(uint8_t step) {
  const char* errMsg = step < MAX_PATTERN_STEPS ? nullptr : "step index out of range";
  return {step < MAX_PATTERN_STEPS, errMsg};
}

Result validateArgs(const SequencerState& state, uint8_t lane = 0, uint8_t step = 0) {
  Result res;
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

    // ==== Stop handling ====
    if (block.stoppedThisBlock) {
      if (laneState.noteActive)
        laneOut.push(makeNoteOffEvent(laneState.activeNote));
      fireAllPendingUnlocks(laneState, laneOut);
      laneState.noteActive = false;
      laneState.lastStep = -1;
      continue;
    }

    // ==== Step evaluation ====
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
        if (stepBeat >= localStart && stepBeat < localEnd)
          fireStep(i, pattern, laneState, laneOut, ctx);
      }
    } else {
      const double headEnd = localEnd - patternLengthBeats;
      for (uint32_t i = 0; i < pattern.numSteps; ++i) {
        if (i * stepLengthBeats >= localStart)
          fireStep(i, pattern, laneState, laneOut, ctx);
      }
      for (uint32_t i = 0; i < pattern.numSteps; ++i) {
        if (i * stepLengthBeats < headEnd)
          fireStep(i, pattern, laneState, laneOut, ctx);
      }
    }
  }
}

// =====================
// Pattern Editing
// =====================

// Preps write buffer
Result beginPatternEdit(SequencerState& state, bool copy) {
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
Result commitPattern(SequencerState& state) {
  auto res = checkIsEditing(state);
  if (!res.ok)
    return res;

  uint32_t writeIndex = 1 - state.store.readIndex.load(std::memory_order_relaxed);
  state.store.readIndex.store(writeIndex, std::memory_order_release);
  state.isEditing = false;
  return res;
}

Result setStep(SequencerState& state, uint8_t lane, uint8_t step, const StepEvent& evt) {
  auto res = validateArgs(state, lane, step);
  if (!res.ok)
    return res;

  getWriteBuffer(state).lanes[lane].steps[step] = evt;
  return res;
}

Result setStepActive(SequencerState& state, uint8_t lane, uint8_t step, bool active) {
  auto res = validateArgs(state, lane, step);
  if (!res.ok)
    return res;

  getWriteBuffer(state).lanes[lane].steps[step].active = active;
  return res;
}

Result setStepNote(SequencerState& state, uint8_t lane, uint8_t step, uint8_t note) {
  auto res = validateArgs(state, lane, step);
  if (!res.ok)
    return res;

  getWriteBuffer(state).lanes[lane].steps[step].note = note;
  return res;
}

Result setStepVelocity(SequencerState& state, uint8_t lane, uint8_t step, uint8_t velocity) {
  auto res = validateArgs(state, lane, step);
  if (!res.ok)
    return res;

  getWriteBuffer(state).lanes[lane].steps[step].velocity = velocity;
  return res;
}

Result setStepNoteOn(SequencerState& state, uint8_t lane, uint8_t step, bool noteOn) {
  auto res = validateArgs(state, lane, step);
  if (!res.ok)
    return res;

  getWriteBuffer(state).lanes[lane].steps[step].noteOn = noteOn;
  return res;
}

Result
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

Result clearStepLock(SequencerState& state, uint8_t lane, uint8_t step, uint8_t paramID) {
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

Result clearStepLocks(SequencerState& state, uint8_t lane, uint8_t step) {
  auto res = validateArgs(state, lane, step);
  if (!res.ok)
    return res;

  getWriteBuffer(state).lanes[lane].steps[step].numLocks = 0;
  return res;
}

// ===== Multi-value/full-pattern input =====
Result setActivePattern(SequencerState& state, uint8_t lane, const uint8_t* values, uint8_t count) {
  auto res = validateArgs(state, lane);
  if (!res.ok)
    return res;

  LanePattern& lp = getWriteBuffer(state).lanes[lane];
  if (count != lp.numSteps)
    return {false, "table length must match numSteps"};

  for (uint8_t i = 0; i < count; ++i)
    lp.steps[i].active = values[i] != 0;
  return res;
}

Result setNotePattern(SequencerState& state, uint8_t lane, const uint8_t* values, uint8_t count) {
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

Result
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
