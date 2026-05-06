#pragma once

#include "app/Constants.h"
#include "app/Track.h"
#include "app/Transport.h"
#include "app/Types.h"
#include "app/sessions/AudioSession.h"

#include "synth/events/Events.h"

#include <cstdint>

namespace app::sequencer {
using synth::events::EngineEvent;
using synth::events::MIDIEvent;
using synth::events::ParamEvent;
using synth::events::ScheduledEvent;

using audio::DEFAULT_FRAMES;
using audio::DEFAULT_SAMPLE_RATE;
using transport::DEFAULT_BPM;

inline constexpr uint8_t PATTERNS_PER_LANE = 8;
inline constexpr uint8_t INVALID_PATTERN_SLOT = 0xFF;

inline constexpr uint8_t MAX_LANES = MAX_TRACKS;
inline constexpr double MIN_GATE_BEAT =
    static_cast<double>(DEFAULT_FRAMES) / (DEFAULT_SAMPLE_RATE * (DEFAULT_BPM / 60.0));

// "beat" == Quarter Note
inline constexpr uint8_t MAX_PATTERN_STEPS = 64;
inline constexpr uint8_t DEFAULT_PATTERN_STEPS = 16;

inline constexpr uint8_t MAX_STEPS_PER_BEAT = 48;
inline constexpr uint8_t DEFAULT_STEPS_PER_BEAT = 4;

inline constexpr uint8_t MAX_LOCKS_PER_STEP = 4;
inline constexpr uint32_t MAX_PENDING_UNLOCKS = MAX_LOCKS_PER_STEP * MAX_PATTERN_STEPS;

using ParamCallback = float (*)(uint8_t id, void* ctx);

// ==================
// P-Lock
// ==================

struct ParamLock {
  uint8_t paramID = 0;
  float value = 0.0f;
};

struct ParamUnlock {
  uint8_t paramID = 0;
  float restoreValue = 0.0f;
  float lockedValue = 0.0f;
  bool pending = false;
};

struct StepLocks {
  const ParamLock* locks = nullptr;
  uint8_t numLocks = 0;
};

struct PendingUnlocks {
  ParamUnlock entries[MAX_PENDING_UNLOCKS]{};
};

// ==================
// Step / Pattern
// ==================

struct StepEvent {
  ParamLock locks[MAX_LOCKS_PER_STEP]{};
  uint8_t numLocks = 0;

  bool active = false;
  bool noteOn = false;

  bool legato = false; // aka "tie": ignores gate
  float gate = 0.5f;

  uint8_t note = 0;
  uint8_t velocity = 0;
};

struct LanePattern {
  StepEvent steps[MAX_PATTERN_STEPS]{};
  uint8_t numSteps = DEFAULT_PATTERN_STEPS;
  uint8_t stepsPerBeat = DEFAULT_STEPS_PER_BEAT;
};

struct PatternBankSlot {
  bool occupied = false;
  LanePattern pattern{};
};

struct PatternBank {
  PatternBankSlot slots[PATTERNS_PER_LANE]{};
  uint8_t activeSlot = INVALID_PATTERN_SLOT;
};

struct PatternSnapshot {
  PatternBank lanes[MAX_LANES]{};
};

struct PatternStore {
  PatternSnapshot buffers[2]{};
  std::atomic<uint32_t> readIndex{0};

  void setReadIndex(uint32_t i) { readIndex.store(i, std::memory_order_relaxed); }
};

// ==================
// Lane State
// ==================

using GetParamCallback = float (*)(uint8_t id, void* ctx);

// lane-specific or target-derived data required by sequencer
struct LaneContext {
  GetParamCallback getParamCallback = nullptr;
  void* getParamCtx = nullptr;
};

struct LaneState {
  PendingUnlocks unlocks{};
  double noteOffBeat = -1.0;
  int32_t lastStep = -1;
  int64_t lastStepCycle = -1;
  bool noteActive = false;
  uint8_t activeNote = 0;
};

struct PatternConfig {
  uint8_t numSteps = 0;
  uint8_t stepsPerBeat = 0;
};

// =================
// Sequencer State
// =================
struct SequencerState {
  PatternStore store{};

  LaneState lanes[MAX_TRACKS]{};
  LaneContext laneCtxs[MAX_TRACKS]{};
  uint8_t numLanes = 0;

  bool isEditing = false;
};

// ===============
// Processing
// ===============

//  Per step:
//  - MAX_PATTERN_STEPS * (
//  - (max lock + unlock events) + (
//  - 1 cut noteOff +
//  - 1 noteOn +
//  - 1 same-block gate noteOff )) +
//  - 1 pending gate noteOff
inline constexpr uint16_t MAX_LANE_EVENTS_PER_BLOCK =
    MAX_PATTERN_STEPS * ((2 * MAX_LOCKS_PER_STEP) + 3) + 1;

struct LaneEvents {
  ScheduledEvent events[MAX_LANE_EVENTS_PER_BLOCK];
  uint16_t count = 0;

  bool push(const ScheduledEvent& e) {
    if (count >= MAX_LANE_EVENTS_PER_BLOCK)
      return false;
    events[count++] = e;
    return true;
  }
};

struct SequencerLaneEvents {
  LaneEvents lanes[MAX_TRACKS]{};
};

struct SequencerBlockWindow {
  double startBeat = 0.0;
  double endBeat = 0.0;
  uint32_t numFrames = 0;
  bool stoppedThisBlock = false;
};

struct InitSequencerContext {
  track::TrackState* tracksArr;
  size_t numTracks;
  ParamCallback callback;
};

void initSequencer(SequencerState& seq, InitSequencerContext);
void runSequencer(SequencerState& seq, SequencerBlockWindow block, SequencerLaneEvents& evts);

// =====================
// Pattern Editing
// =====================

VoidResult beginPatternEdit(SequencerState& state, bool copy);
VoidResult commitPattern(SequencerState& state);
VoidResult abortPatternEdit(SequencerState& state);

// ==== Pattern-level ====
VoidResult setPatternNumSteps(SequencerState& state, uint8_t lane, uint8_t numSteps);
VoidResult setPatternStepsPerBeat(SequencerState& state, uint8_t lane, uint8_t stepsPerBeat);

VoidResult clearPattern(SequencerState& state, uint8_t lane);

// ==== Step field setters ====
VoidResult setStep(SequencerState& state, uint8_t lane, uint8_t step, const StepEvent& evt);
VoidResult setStepActive(SequencerState& state, uint8_t lane, uint8_t step, bool active);
VoidResult setStepNote(SequencerState& state, uint8_t lane, uint8_t step, uint8_t note);
VoidResult setStepNoteOn(SequencerState& state, uint8_t lane, uint8_t step, bool noteOn);
VoidResult setStepVelocity(SequencerState& state, uint8_t lane, uint8_t step, uint8_t velocity);
VoidResult setStepGate(SequencerState& state, uint8_t lane, uint8_t step, float gate);
VoidResult setStepLegato(SequencerState& state, uint8_t lane, uint8_t step, bool legato);

VoidResult clearStep(SequencerState& state, uint8_t lane, uint8_t step);

// ==== P-lock editing ====
VoidResult
setStepLock(SequencerState& state, uint8_t lane, uint8_t step, uint8_t paramID, float value);
VoidResult clearStepLock(SequencerState& state, uint8_t lane, uint8_t step, uint8_t paramID);
VoidResult clearStepLocks(SequencerState& state, uint8_t lane, uint8_t step);

// ==== Bulk edit — table length must equal numSteps exactly, otherwise error ====
VoidResult
setActivePattern(SequencerState& state, uint8_t lane, const uint8_t* values, uint8_t count);
VoidResult
setNotePattern(SequencerState& state, uint8_t lane, const uint8_t* values, uint8_t count);
VoidResult
setVelocityPattern(SequencerState& state, uint8_t lane, const uint8_t* values, uint8_t count);

VoidResult replacePatternBank(SequencerState& state, uint8_t lane, const PatternBank& bank);
VoidResult
replacePattern(SequencerState& state, uint8_t lane, uint8_t slot, const LanePattern& pattern);

VoidResult clearPatternBankSlot(SequencerState& state, uint8_t lane, uint8_t slot);
VoidResult setActivePatternSlot(SequencerState& state, uint8_t lane, uint8_t slot);

// phase-1b
DEFINE_VALUE_RESULT(const StepEvent*, nullptr, GetStep);
DEFINE_VALUE_RESULT(const LanePattern*, nullptr, GetPattern);
DEFINE_VALUE_RESULT(StepLocks, StepLocks{}, GetStepLocks);

DEFINE_VALUE_RESULT(PatternConfig, PatternConfig{}, GetPatternConfig);
DEFINE_VALUE_RESULT(const PatternBank*, nullptr, GetPatternBank);
DEFINE_VALUE_RESULT(const PatternBankSlot*, nullptr, GetPatternBankSlot);

GetStepResult getStep(const SequencerState& state, uint8_t lane, uint8_t step);
GetPatternResult getOrCreatePendingPattern(SequencerState& state, uint8_t lane);
GetPatternResult getPendingPattern(const SequencerState& state, uint8_t lane);
GetPatternResult getActivePattern(const SequencerState& state, uint8_t lane);
GetPatternConfigResult getPatternConfig(const SequencerState& state, uint8_t lane);

GetPatternBankResult getPendingPatternBank(const SequencerState& state, uint8_t lane);
GetPatternBankResult getPatternBank(const SequencerState& state, uint8_t lane);
GetPatternBankSlotResult getPatternBankSlot(const SequencerState& state,
                                            uint8_t lane,
                                            uint8_t slot);

GetStepLocksResult getStepLocks(const SequencerState& state, uint8_t lane, uint8_t step);

} // namespace app::sequencer
