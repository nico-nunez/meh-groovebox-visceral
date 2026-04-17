#include "Sequencer.h"

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

} // namespace

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

} // namespace app::sequencer
