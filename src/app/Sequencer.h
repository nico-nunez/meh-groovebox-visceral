#pragma once

#include "app/Constants.h"

#include "synth/events/Events.h"

#include <cstdint>

namespace app::sequencer {
using synth::events::EngineEvent;
using synth::events::MIDIEvent;
using synth::events::ParamEvent;

// ==================
// P-Lock
// ==================
inline constexpr uint32_t MAX_LOCKS_PER_STEP = 4;
inline constexpr uint32_t MAX_STEPS_PER_BLOCK = 2; // theortical and not actually enforced
inline constexpr uint32_t MAX_PENDING_UNLOCKS = MAX_LOCKS_PER_STEP * MAX_STEPS_PER_BLOCK;

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

inline constexpr uint32_t MAX_PATTERN_STEPS = 16;

struct StepEvent {
  ParamLock locks[MAX_LOCKS_PER_STEP]{};
  uint8_t numLocks = 0;

  bool active = false;
  bool noteOn = false;
  uint8_t note = 0;
  uint8_t velocity = 0;
};

struct LanePattern {
  StepEvent steps[MAX_PATTERN_STEPS]{};
  uint32_t numSteps = MAX_PATTERN_STEPS;
  uint8_t stepsPerBeat = 4;
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
};

// ===============
// Processing
// ===============

struct SequencerEvent {
  enum class Kind : uint8_t { MIDI, Param, Engine };
  Kind kind;
  union {
    synth::events::MIDIEvent midi;
    synth::events::ParamEvent param;
    synth::events::EngineEvent engine;
  } data;
};

// "+ 2" is for note on/off
inline constexpr uint8_t MAX_LANE_EVENTS_PER_BLOCK = MAX_PATTERN_STEPS * (MAX_LOCKS_PER_STEP + 2);

struct LaneEvents {
  SequencerEvent events[MAX_LANE_EVENTS_PER_BLOCK];
  uint8_t count = 0;

  bool push(const SequencerEvent& e) {
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
  bool stoppedThisBlock = false;
};

void runSequencer(SequencerState& state, SequencerBlockWindow block, SequencerLaneEvents& evts);

} // namespace app::sequencer
