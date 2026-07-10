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

ScheduledEvent makePostNoteOnNoteOffEvent(uint8_t note, uint32_t sampleOffset) {
  return makeNoteOffEvent(note, sampleOffset, ScheduledEventOrder::PostNoteOnNoteOff);
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

struct StepOccurrence {
  uint32_t stepIndex = 0;
  int64_t cycleIndex = 0;
  double absoluteBeat = 0.0;
};

bool shouldSkipStepOccurrence(const LaneState& laneState, const StepOccurrence& occurrence) {
  return laneState.lastStep == static_cast<int32_t>(occurrence.stepIndex) &&
         laneState.lastStepCycle == occurrence.cycleIndex;
}

void markStepOccurrenceFired(LaneState& laneState, const StepOccurrence& occurrence) {
  laneState.lastStep = static_cast<int32_t>(occurrence.stepIndex);
  laneState.lastStepCycle = occurrence.cycleIndex;
}

bool beatIsInsideBlock(double beat, const SequencerBlockWindow& block) {
  return beat >= block.startBeat && beat < block.endBeat;
}

int64_t firstPatternCycleInBlock(const SequencerBlockWindow& block, double patternLengthBeats) {
  return static_cast<int64_t>(std::floor(block.startBeat / patternLengthBeats));
}

int64_t lastPatternCycleInBlock(const SequencerBlockWindow& block, double patternLengthBeats) {
  constexpr double kEndBeatEpsilon = 1e-9;
  return static_cast<int64_t>(std::floor((block.endBeat - kEndBeatEpsilon) / patternLengthBeats));
}

bool sameBeat(double a, double b) {
  constexpr double kBeatEpsilon = 1e-9;
  return std::fabs(a - b) <= kBeatEpsilon;
}

void clearPendingNoteOff(PendingNoteOff& pending) {
  pending.pending = false;
  pending.note = 0;
  pending.beat = -1.0;
}

VoidResult addPendingNoteOff(LaneState& laneState, uint8_t note, double beat) {
  for (uint16_t i = 0; i < MAX_PENDING_NOTE_OFFS; ++i) {
    PendingNoteOff& pending = laneState.noteOffs[i];
    if (!pending.pending) {
      pending.pending = true;
      pending.note = note;
      pending.beat = beat;
      return {true, nullptr};
    }
  }
  assert(true);
  return {false, "pending note-off capacity exceeded"};
}

PendingNoteOff* findPendingNoteOff(LaneState& laneState, uint8_t note) {
  for (uint16_t i = 0; i < MAX_PENDING_NOTE_OFFS; ++i) {
    PendingNoteOff& pending = laneState.noteOffs[i];
    if (pending.pending && pending.note == note)
      return &pending;
  }
  return nullptr;
}

double stepNoteOffBeat(const StepNote& note, double absStepBeat, double stepLengthBeats) {
  const double gateBeats =
      std::max(static_cast<double>(note.gate) * stepLengthBeats, MIN_GATE_BEAT);
  return absStepBeat + gateBeats;
}

bool stepHasActivePitch(const StepEvent& step, uint8_t note) {
  for (uint8_t i = 0; i < step.noteCount; ++i) {
    const StepNote& stepNote = step.notes[i];
    if (stepNote.noteOn && stepNote.note == note)
      return true;
  }
  return false;
}

void extendPendingNoteOff(PendingNoteOff* pending, double noteOffBeat) {
  pending->beat = noteOffBeat;
}

void firePendingNoteOffsBeforeBeat(LaneState& laneState,
                                   LaneEvents& laneOut,
                                   const SequencerBlockWindow& block,
                                   double beat) {
  for (uint16_t i = 0; i < MAX_PENDING_NOTE_OFFS; ++i) {
    PendingNoteOff& pending = laneState.noteOffs[i];
    if (!pending.pending)
      continue;

    if (pending.beat >= block.startBeat && pending.beat < block.endBeat && pending.beat < beat) {
      const uint32_t offset = beatToSampleOffset(pending.beat, block);
      laneOut.push(makeNoteOffEvent(pending.note, offset, ScheduledEventOrder::GateNoteOff));
      clearPendingNoteOff(pending);
    }
  }
}

void fireRemainingPendingNoteOffsInBlock(LaneState& laneState,
                                         LaneEvents& laneOut,
                                         const SequencerBlockWindow& block) {
  for (uint16_t i = 0; i < MAX_PENDING_NOTE_OFFS; ++i) {
    PendingNoteOff& pending = laneState.noteOffs[i];
    if (!pending.pending)
      continue;

    if (pending.beat >= block.startBeat && pending.beat < block.endBeat) {
      uint32_t offset = beatToSampleOffset(pending.beat, block);
      laneOut.push(makeNoteOffEvent(pending.note, offset, ScheduledEventOrder::GateNoteOff));
      clearPendingNoteOff(pending);
    }
  }
}

void fireAllPendingNoteOffs(LaneState& laneState, LaneEvents& laneOut) {
  for (uint16_t i = 0; i < MAX_PENDING_NOTE_OFFS; ++i) {
    PendingNoteOff& pending = laneState.noteOffs[i];
    if (!pending.pending)
      continue;

    laneOut.push(makeNoteOffEvent(pending.note, 0, ScheduledEventOrder::NoteOff));
    clearPendingNoteOff(pending);
  }
}

void cutSamePitchPendingNotes(LaneState& laneState,
                              LaneEvents& laneOut,
                              uint8_t note,
                              double absStepBeat,
                              uint32_t stepOffset) {
  for (uint16_t i = 0; i < MAX_PENDING_NOTE_OFFS; ++i) {
    PendingNoteOff& pending = laneState.noteOffs[i];
    if (!pending.pending || pending.note != note)
      continue;
    if (pending.beat >= absStepBeat) {
      laneOut.push(makeNoteOffEvent(note, stepOffset, ScheduledEventOrder::NoteOff));
      clearPendingNoteOff(pending);
    }
  }
}

void fireExactBoundaryNonStepPitchReleases(LaneState& laneState,
                                           LaneEvents& laneOut,
                                           const StepEvent& step,
                                           double absStepBeat,
                                           uint32_t stepOffset) {
  for (uint16_t i = 0; i < MAX_PENDING_NOTE_OFFS; ++i) {
    PendingNoteOff& pending = laneState.noteOffs[i];
    if (!pending.pending)
      continue;
    if (!sameBeat(pending.beat, absStepBeat))
      continue;
    if (stepHasActivePitch(step, pending.note))
      continue;
    laneOut.push(makePostNoteOnNoteOffEvent(pending.note, stepOffset));
    clearPendingNoteOff(pending);
  }
}

void fireStep(uint32_t i,
              const LanePattern* pattern,
              LaneState& laneState,
              LaneEvents& laneOut,
              const LaneContext& ctx,
              double absStepBeat,
              double stepLengthBeats,
              const SequencerBlockWindow& block) {
  const StepEvent& step = pattern->steps[i];
  const uint32_t stepOffset = beatToSampleOffset(absStepBeat, block);

  firePendingNoteOffsBeforeBeat(laneState, laneOut, block, absStepBeat);
  resolvePendingUnlocks(laneState, laneOut, ctx, step, stepOffset);

  if (!step.active)
    return;

  applyParamLocks(step, laneState, laneOut, ctx, stepOffset);

  bool heldTie[MAX_NOTES_PER_STEP]{};

  for (uint8_t noteIndex = 0; noteIndex < step.noteCount; ++noteIndex) {
    const StepNote& note = step.notes[noteIndex];
    if (!note.noteOn)
      continue;

    const double noteOffBeat = stepNoteOffBeat(note, absStepBeat, stepLengthBeats);
    PendingNoteOff* existing = findPendingNoteOff(laneState, note.note);

    if (note.tie && existing) {
      heldTie[noteIndex] = true;
      extendPendingNoteOff(existing, noteOffBeat);
      continue;
    }

    cutSamePitchPendingNotes(laneState, laneOut, note.note, absStepBeat, stepOffset);
  }

  for (uint8_t noteIndex = 0; noteIndex < step.noteCount; ++noteIndex) {
    const StepNote& note = step.notes[noteIndex];
    if (!note.noteOn || heldTie[noteIndex])
      continue;

    laneOut.push(makeNoteOnEvent(note.note, note.velocity, stepOffset));

    const double noteOffBeat = stepNoteOffBeat(note, absStepBeat, stepLengthBeats);
    addPendingNoteOff(laneState, note.note, noteOffBeat);
  }

  fireExactBoundaryNonStepPitchReleases(laneState, laneOut, step, absStepBeat, stepOffset);
}

// =====================
// Pattern Editing
// =====================

const PatternSnapshot& getReadBuffer(const SequencerState& state) {
  const uint32_t readIndex = state.store.readIndex.load(std::memory_order_acquire);
  return state.store.buffers[readIndex];
}

PatternSnapshot& getWriteBuffer(SequencerState& state) {
  uint32_t writeIndex = 1 - state.store.readIndex.load(std::memory_order_relaxed);
  assert(writeIndex < 2);

  return state.store.buffers[writeIndex];
}

VoidResult checkIsEditing(const SequencerState& state) {
  bool isEditing = state.isEditing.load(std::memory_order_acquire);

  const char* errMsg =
      isEditing ? nullptr : "no active edit session; call seq.copyPattern or seq.newPattern first";

  assert(errMsg == nullptr);
  return {isEditing, errMsg};
}

VoidResult checkLaneBounds(uint8_t lane) {
  const char* errMsg = lane < MAX_LANES ? nullptr : "lane index out of range";
  return {lane < MAX_LANES, errMsg};
}

// ======================
// Pattern Bank Helpers
// ======================

const PatternBank* getActiveLanePatternBank(const SequencerState& state, uint8_t lane) {
  if (lane >= MAX_LANES)
    return nullptr;
  return &getReadBuffer(state).lanes[lane];
}

PatternBank* getPendingLanePatternBank(SequencerState& state, uint8_t lane) {
  if (lane >= MAX_LANES)
    return nullptr;
  return &getWriteBuffer(state).lanes[lane];
}

const LanePattern* resolvePatternSlot(const PatternBank& bank, uint8_t slot) {
  if (slot >= PATTERNS_PER_LANE)
    return nullptr;

  const PatternBankSlot& patternSlot = bank.slots[slot];
  if (!patternSlot.occupied)
    return nullptr;

  return &patternSlot.pattern;
}

const LanePattern* resolveActivePatternForPlayback(const PatternBank& bank) {
  if (bank.activeSlot == INVALID_PATTERN_SLOT)
    return nullptr;

  return resolvePatternSlot(bank, bank.activeSlot);
}

LanePattern* getOrCreatePendingSelectedPattern(SequencerState& state, uint8_t lane) {
  PatternBank* bank = getPendingLanePatternBank(state, lane);
  if (!bank)
    return nullptr;

  if (bank->activeSlot == INVALID_PATTERN_SLOT) {
    bank->activeSlot = 0;
    bank->slots[0].occupied = true;
    bank->slots[0].pattern = LanePattern{};
  }

  const uint8_t slot = bank->activeSlot;
  if (slot >= PATTERNS_PER_LANE)
    return nullptr;

  PatternBankSlot& patternSlot = bank->slots[slot];
  if (!patternSlot.occupied) {
    patternSlot.occupied = true;
    patternSlot.pattern = LanePattern{};
  }

  return &patternSlot.pattern;
}

// ===============
// Validators
// ===============

VoidResult validateGate(float gate) {
  if (!std::isfinite(gate) || gate < 0.0f)
    return {false, "gate out of range"};
  return {true, nullptr};
}

VoidResult validateNote(uint8_t note) {
  if (note > 127)
    return {false, "note out of range"};

  return {true, nullptr};
}

VoidResult validateVelocity(uint8_t velocity) {
  if (velocity > 127)
    return {false, "velocity out of range"};

  return {true, nullptr};
}

VoidResult validateNumLocks(uint8_t numLocks) {
  if (numLocks > MAX_LOCKS_PER_STEP)
    return {false, "exceeds max step locks"};
  return {true, nullptr};
}

VoidResult validateNoteCount(uint8_t count) {
  return count <= MAX_NOTES_PER_STEP ? VoidResult{true, nullptr}
                                     : VoidResult{false, "too many notes in step"};
}

VoidResult validateUniqueActiveStepNotePitches(const StepEvent& evt) {
  for (uint8_t i = 0; i < evt.noteCount; ++i) {
    const StepNote& a = evt.notes[i];
    if (!a.noteOn)
      continue;
    for (uint8_t j = static_cast<uint8_t>(i + 1); j < evt.noteCount; ++j) {
      const StepNote& b = evt.notes[j];
      if (!b.noteOn)
        continue;
      if (a.note == b.note)
        return {false, "duplicate note pitch in step"};
    }
  }
  return {true, nullptr};
}

VoidResult validateStepNote(const StepNote& note) {
  CHECK_RESULT(validateNote(note.note));
  CHECK_RESULT(validateVelocity(note.velocity));
  CHECK_RESULT(validateGate(note.gate));
  return {true, nullptr};
}

VoidResult validateStepNoteForPattern(const StepNote& note, uint8_t maxGateSteps) {
  CHECK_RESULT(validateStepNote(note));
  if (note.gate > static_cast<float>(maxGateSteps))
    return {false, "gate exceeds pattern length"};
  return {true, nullptr};
}

VoidResult validateStepEventForPattern(const StepEvent& evt, uint8_t maxGateSteps) {
  CHECK_RESULT(validateNumLocks(evt.numLocks));
  CHECK_RESULT(validateNoteCount(evt.noteCount));
  for (uint8_t i = 0; i < evt.noteCount; ++i)
    CHECK_RESULT(validateStepNoteForPattern(evt.notes[i], maxGateSteps));
  CHECK_RESULT(validateUniqueActiveStepNotePitches(evt));
  return {true, nullptr};
}

VoidResult validateLanePattern(const LanePattern& pattern) {
  if (pattern.numSteps == 0 || pattern.numSteps > MAX_PATTERN_STEPS)
    return {false, "numSteps out of range"};

  if (pattern.stepsPerBeat == 0 || pattern.stepsPerBeat > MAX_STEPS_PER_BEAT)
    return {false, "stepsPerBeat out of range"};

  for (uint8_t i = 0; i < pattern.numSteps; ++i)
    CHECK_RESULT(validateStepEventForPattern(pattern.steps[i], pattern.numSteps));

  return {true, nullptr};
}

VoidResult validateActiveSlot(const PatternBank& bank) {
  if (bank.activeSlot == INVALID_PATTERN_SLOT)
    return {true, nullptr};

  if (bank.activeSlot >= PATTERNS_PER_LANE)
    return {false, "activeSlot out of range"};

  if (!bank.slots[bank.activeSlot].occupied)
    return {false, "activeSlot points to empty slot"};

  return {true, nullptr};
}

VoidResult validatePatternBank(const PatternBank& bank) {
  CHECK_RESULT(validateActiveSlot(bank));

  for (uint8_t slot = 0; slot < PATTERNS_PER_LANE; ++slot) {
    if (!bank.slots[slot].occupied)
      continue;
    CHECK_RESULT(validateLanePattern(bank.slots[slot].pattern));
  }

  return {true, nullptr};
}

// =====================
// Edit Session
// =====================
VoidResult beginPatternEdit(SequencerState& state, bool copy) {
  if (state.isEditing.load(std::memory_order_acquire))
    return {false, "edit session already in progress"};

  PatternSnapshot& writeBuf = getWriteBuffer(state);

  if (copy) {
    uint32_t readIndex = state.store.readIndex.load(std::memory_order_relaxed);
    writeBuf = state.store.buffers[readIndex];
  } else {
    writeBuf = PatternSnapshot{};
  }

  state.isEditing.store(true, std::memory_order_release);
  return {true, nullptr};
}

// Swap write -> read buffer
VoidResult commitPattern(SequencerState& state) {
  VoidResult res{};
  CHECK_RESULT(checkIsEditing(state));

  uint32_t writeIndex = 1 - state.store.readIndex.load(std::memory_order_relaxed);
  state.store.readIndex.store(writeIndex, std::memory_order_release);
  state.isEditing.store(false, std::memory_order_release);
  assert(res.ok);
  return res;
}

VoidResult abortPatternEdit(SequencerState& state) {
  if (!state.isEditing.load(std::memory_order_acquire))
    return {false, "no editing session in progress"};

  state.isEditing.store(false, std::memory_order_release);
  return {true, nullptr};
}

} // namespace

// ==================
// Helpers
// ==================
GetPatternResult getOrCreatePendingPattern(SequencerState& state, uint8_t lane) {
  GetPatternResult res{};
  if (!state.isEditing.load(std::memory_order_acquire)) {
    res.ok = false;
    res.err = "no editing session in progress";
    return res;
  }

  LanePattern* pattern = getOrCreatePendingSelectedPattern(state, lane);
  if (!pattern) {
    res.ok = false;
    res.err = "no pending selected pattern";
    return res;
  }

  res.value = pattern;
  return res;
}

GetPatternResult getPendingPattern(const SequencerState& state, uint8_t lane) {
  GetPatternResult res{};
  if (!state.isEditing.load(std::memory_order_acquire)) {
    res.ok = false;
    res.err = "no editing session in progress";
    return res;
  }

  const uint32_t writeIndex = 1 - state.store.readIndex.load(std::memory_order_relaxed);
  if (lane >= MAX_LANES) {
    res.ok = false;
    res.err = "lane out of range";
    return res;
  }

  const PatternBank& bank = state.store.buffers[writeIndex].lanes[lane];
  if (bank.activeSlot == INVALID_PATTERN_SLOT) {
    res.ok = false;
    res.err = "no pending selected pattern";
    return res;
  }

  const LanePattern* pattern = resolvePatternSlot(bank, bank.activeSlot);
  if (!pattern) {
    res.ok = false;
    res.err = "no pending selected pattern";
    return res;
  }

  res.value = pattern;
  return res;
}

GetPatternResult getActivePattern(const SequencerState& state, uint8_t lane) {
  GetPatternResult res{};
  const PatternBank* bank = getActiveLanePatternBank(state, lane);
  if (!bank) {
    res.ok = false;
    res.err = "lane out of range";
    return res;
  }

  const LanePattern* pattern = resolveActivePatternForPlayback(*bank);
  if (!pattern) {
    res.ok = false;
    res.err = "no active pattern";
    return res;
  }

  res.value = pattern;
  return res;
}

GetStepResult getStep(const SequencerState& state, uint8_t lane, uint8_t step) {
  GetStepResult res{};
  const PatternBank* bank = getActiveLanePatternBank(state, lane);
  if (!bank) {
    res.ok = false;
    res.err = "lane out of range";
    return res;
  }

  const LanePattern* pattern = resolveActivePatternForPlayback(*bank);
  if (!pattern) {
    res.ok = false;
    res.err = "no active pattern";
    return res;
  }

  if (step >= pattern->numSteps) {
    res.ok = false;
    res.err = "step out of range";
    return res;
  }

  res.value = &pattern->steps[step];
  return res;
}

GetPatternConfigResult getPatternConfig(const SequencerState& state, uint8_t lane) {
  GetPatternConfigResult res{};
  auto patternRes = getActivePattern(state, lane);
  if (!patternRes.ok) {
    res.ok = false;
    res.err = patternRes.err;
    return res;
  }
  res.value.numSteps = patternRes.value->numSteps;
  res.value.stepsPerBeat = patternRes.value->stepsPerBeat;
  return res;
}

GetStepLocksResult getStepLocks(const SequencerState& state, uint8_t lane, uint8_t step) {
  GetStepLocksResult res{};

  auto stepRes = getStep(state, lane, step);
  if (!stepRes.ok) {
    res.ok = false;
    res.err = stepRes.err;
    return res;
  }
  res.value.numLocks = stepRes.value->numLocks;
  res.value.locks = stepRes.value->locks;
  return res;
}

GetPatternBankResult getPatternBank(const SequencerState& state, uint8_t lane) {
  GetPatternBankResult res{};
  if (lane >= MAX_LANES) {
    res.ok = false;
    res.err = "lane out of range";
    return res;
  }

  res.value = &getReadBuffer(state).lanes[lane];
  return res;
}

GetPatternBankResult getPendingPatternBank(const SequencerState& state, uint8_t lane) {
  GetPatternBankResult res{};
  if (!state.isEditing.load(std::memory_order_acquire)) {
    res.ok = false;
    res.err = "no editing session in progress";
    return res;
  }
  if (lane >= MAX_LANES) {
    res.ok = false;
    res.err = "lane out of range";
    return res;
  }

  const uint32_t writeIndex = 1 - state.store.readIndex.load(std::memory_order_relaxed);
  res.value = &state.store.buffers[writeIndex].lanes[lane];
  return res;
}

GetPatternBankSlotResult getPatternBankSlot(const SequencerState& state,
                                            uint8_t lane,
                                            uint8_t slot) {
  GetPatternBankSlotResult res{};
  if (lane >= MAX_LANES) {
    res.ok = false;
    res.err = "lane out of range";
    return res;
  }
  if (slot >= PATTERNS_PER_LANE) {
    res.ok = false;
    res.err = "pattern slot out of range";
    return res;
  }

  res.value = &getReadBuffer(state).lanes[lane].slots[slot];
  return res;
}

// =================
// Initialization
// =================
void initSequencer(SequencerState& seq, InitSequencerContext ctx) {
  // init track bindings <-- NOTE: this feels off....
  for (uint8_t i = 0; i < ctx.numTracks; i++) {
    seq.laneCtxs[i].getParamCallback = ctx.callback;
    seq.laneCtxs[i].getParamCtx = &ctx.tracksArr[i].engine;
  }

  // init read & write buffers
  for (uint8_t bufferIndex = 0; bufferIndex < 2; ++bufferIndex) {
    for (uint8_t i = 0; i < MAX_LANES; i++) {
      PatternBank& bank = seq.store.buffers[bufferIndex].lanes[i];
      bank.activeSlot = INVALID_PATTERN_SLOT;

      for (uint8_t slot = 0; slot < PATTERNS_PER_LANE; ++slot) {
        bank.slots[slot].occupied = false;
        bank.slots[slot].pattern = LanePattern{};
      }
    }
  }
  seq.numLanes = MAX_LANES;
}

void clearSequencerLaneEvents(SequencerLaneEvents& events) {
  for (uint8_t lane = 0; lane < MAX_TRACKS; ++lane)
    events.lanes[lane].count = 0;
}

// =================
// Processing
// =================
void runSequencer(SequencerState& state, SequencerBlockWindow block, SequencerLaneEvents& evts) {
  const PatternSnapshot& snapshot = getReadBuffer(state);

  for (uint8_t laneIndex = 0; laneIndex < state.numLanes; ++laneIndex) {
    const PatternBank& bank = snapshot.lanes[laneIndex];
    const LanePattern* pattern = resolveActivePatternForPlayback(bank);
    if (!pattern)
      continue;

    LaneState& laneState = state.lanes[laneIndex];
    LaneEvents& laneOut = evts.lanes[laneIndex];
    const LaneContext& ctx = state.laneCtxs[laneIndex];

    if (block.stoppedThisBlock) {
      fireAllPendingNoteOffs(laneState, laneOut);
      fireAllPendingUnlocks(laneState, laneOut);
      laneState.lastStep = -1;
      laneState.lastStepCycle = -1;
      continue;
    }

    if (pattern->numSteps == 0 || pattern->stepsPerBeat == 0)
      continue;

    const double stepLengthBeats = 1.0 / pattern->stepsPerBeat;
    const double patternLengthBeats = pattern->numSteps * stepLengthBeats;

    if (block.endBeat <= block.startBeat)
      continue;

    const int64_t firstCycle = firstPatternCycleInBlock(block, patternLengthBeats);
    const int64_t lastCycle = lastPatternCycleInBlock(block, patternLengthBeats);

    for (int64_t cycle = firstCycle; cycle <= lastCycle; ++cycle) {
      const double cycleStartBeat = static_cast<double>(cycle) * patternLengthBeats;

      for (uint32_t stepIndex = 0; stepIndex < pattern->numSteps; ++stepIndex) {
        const double stepBeat = static_cast<double>(stepIndex) * stepLengthBeats;
        StepOccurrence occurrence{};
        occurrence.stepIndex = stepIndex;
        occurrence.cycleIndex = cycle;
        occurrence.absoluteBeat = cycleStartBeat + stepBeat;

        if (!beatIsInsideBlock(occurrence.absoluteBeat, block))
          continue;

        if (shouldSkipStepOccurrence(laneState, occurrence))
          continue;

        fireStep(stepIndex,
                 pattern,
                 laneState,
                 laneOut,
                 ctx,
                 occurrence.absoluteBeat,
                 stepLengthBeats,
                 block);
        markStepOccurrenceFired(laneState, occurrence);
      }
    }
    fireRemainingPendingNoteOffsInBlock(laneState, laneOut, block);
  }
}

// =====================
// Snapshot
// =====================
VoidResult validatePatternSnapshot(const PatternSnapshot& snapshot) {
  for (uint8_t lane = 0; lane < MAX_LANES; ++lane) {
    CHECK_RESULT(checkLaneBounds(lane));
    CHECK_RESULT(validatePatternBank(snapshot.lanes[lane]));
  }
  return {true, nullptr};
}

VoidResult prepareSequencerSnapshotSwap(SequencerState& state, const PatternSnapshot& snapshot) {
  CHECK_RESULT(validatePatternSnapshot(snapshot));
  CHECK_RESULT(beginPatternEdit(state, false));
  getWriteBuffer(state) = snapshot;
  return {true, nullptr};
}

VoidResult commitSequencerSnapshotSwap(SequencerState& state) {
  CHECK_RESULT(checkIsEditing(state));
  return {true, nullptr};
}

void abortSequencerSnapshotSwap(SequencerState& state) {
  if (state.isEditing.load(std::memory_order_acquire))
    abortPatternEdit(state);
}

void publishPendingSequencerSnapshotIfReady(SequencerState& state) {
  if (!state.isEditing.load(std::memory_order_acquire))
    return;
  commitPattern(state);
}

void resetStepEvent(StepEvent* step) {
  for (uint8_t i = 0; i < MAX_LOCKS_PER_STEP; ++i)
    step->locks[i] = ParamLock{};

  *step = StepEvent{};
}

void resetLanePattern(LanePattern* pattern) {
  for (uint8_t step = 0; step < MAX_PATTERN_STEPS; ++step)
    resetStepEvent(&pattern->steps[step]);

  pattern->numSteps = DEFAULT_PATTERN_STEPS;
  pattern->stepsPerBeat = DEFAULT_STEPS_PER_BEAT;
}

void resetPatternBank(PatternBank* bank) {
  for (uint8_t slot = 0; slot < PATTERNS_PER_LANE; ++slot) {
    bank->slots[slot].occupied = false;
    resetLanePattern(&bank->slots[slot].pattern);
  }

  bank->activeSlot = INVALID_PATTERN_SLOT;
}

void resetPatternSnapshot(PatternSnapshot* snapshot) {
  for (uint8_t lane = 0; lane < MAX_LANES; ++lane)
    resetPatternBank(&snapshot->lanes[lane]);
}

const PatternSnapshot& getPatternSnapshot(const SequencerState& state) {
  return getReadBuffer(state);
}
} // namespace app::sequencer
