#pragma once

#include "app/Constants.h"
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

inline constexpr uint8_t MAX_LANES = MAX_TRACKS;
inline constexpr double MIN_GATE_BEAT =
    static_cast<double>(DEFAULT_FRAMES) / (DEFAULT_SAMPLE_RATE * (DEFAULT_BPM / 60.0));

// "beat" == Quarter Note
inline constexpr uint32_t MAX_PATTERN_STEPS = 64;
inline constexpr uint32_t DEFAULT_PATTERN_STEPS = 16;
inline constexpr uint32_t DEFAULT_STEPS_PER_BEAT = 4;

inline constexpr uint32_t MAX_LOCKS_PER_STEP = 4;
inline constexpr uint32_t MAX_PENDING_UNLOCKS = MAX_LOCKS_PER_STEP * MAX_PATTERN_STEPS;

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
  uint32_t numSteps = DEFAULT_PATTERN_STEPS;
  uint8_t stepsPerBeat = DEFAULT_STEPS_PER_BEAT;
};

struct PatternSnapshot {
  LanePattern lanes[MAX_TRACKS]{};
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
  bool noteActive = false;
  uint8_t activeNote = 0;
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

void runSequencer(SequencerState& state, SequencerBlockWindow block, SequencerLaneEvents& evts);

// =====================
// Pattern Editing
// =====================

Result beginPatternEdit(SequencerState& state, bool copy);
Result commitPattern(SequencerState& state);

// Per-step (set all fields at once)
Result setStep(SequencerState& state, uint8_t lane, uint8_t step, const StepEvent& evt);

// Per-field
Result setStepActive(SequencerState& state, uint8_t lane, uint8_t step, bool active);
Result setStepNote(SequencerState& state, uint8_t lane, uint8_t step, uint8_t note);
Result setStepVelocity(SequencerState& state, uint8_t lane, uint8_t step, uint8_t velocity);
Result setStepNoteOn(SequencerState& state, uint8_t lane, uint8_t step, bool noteOn);

// P-lock editing
Result setStepLock(SequencerState& state, uint8_t lane, uint8_t step, uint8_t paramID, float value);
Result clearStepLock(SequencerState& state, uint8_t lane, uint8_t step, uint8_t paramID);
Result clearStepLocks(SequencerState& state, uint8_t lane, uint8_t step);

// Bulk — table length must equal numSteps exactly, otherwise error
Result setActivePattern(SequencerState& state, uint8_t lane, const uint8_t* values, uint8_t count);
Result setNotePattern(SequencerState& state, uint8_t lane, const uint8_t* values, uint8_t count);
Result
setVelocityPattern(SequencerState& state, uint8_t lane, const uint8_t* values, uint8_t count);

} // namespace app::sequencer
