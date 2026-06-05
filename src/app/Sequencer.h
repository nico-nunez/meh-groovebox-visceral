#pragma once

#include "app/Constants.h"
#include "app/Track.h"
#include "app/Types.h"

#include "synth/events/Events.h"

#include <atomic>
#include <cassert>
#include <cstdint>

namespace app::sequencer {
using synth::events::EngineEvent;
using synth::events::MIDIEvent;
using synth::events::ParamEvent;
using synth::events::ScheduledEvent;

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

struct StepNote {
  bool noteOn = true;
  bool tie = false;
  uint8_t note = 0;
  uint8_t velocity = 0;
  float gate = 0.5f;
};

struct PendingNoteOff {
  bool pending = false;
  uint8_t note = 0;
  double beat = -1.0;
};

struct StepEvent {
  ParamLock locks[MAX_LOCKS_PER_STEP]{};
  uint8_t numLocks = 0;

  bool active = false;

  StepNote notes[MAX_NOTES_PER_STEP]{};
  uint8_t noteCount = 0;
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
  PendingNoteOff noteOffs[MAX_PENDING_NOTE_OFFS]{};
  int32_t lastStep = -1;
  int64_t lastStepCycle = -1;
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

  std::atomic<bool> isEditing{false};
};

// ===============
// Processing
// ===============

struct LaneEvents {
  ScheduledEvent events[MAX_LANE_EVENTS_PER_BLOCK];
  uint16_t count = 0;
  uint32_t droppedEvents = 0;

  bool push(const ScheduledEvent& e) {
    if (count >= MAX_LANE_EVENTS_PER_BLOCK) {
      droppedEvents++;
      assert(false && "sequencer lane event buffer overflow");
      return false;
    }
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

void clearSequencerLaneEvents(SequencerLaneEvents& events);

// =====================
// Pattern Editing
// =====================

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

VoidResult validatePatternSnapshot(const PatternSnapshot& snapshot);
VoidResult prepareSequencerSnapshotSwap(SequencerState& state, const PatternSnapshot& snapshot);
VoidResult commitSequencerSnapshotSwap(SequencerState& state);
void abortSequencerSnapshotSwap(SequencerState& state);
void publishPendingSequencerSnapshotIfReady(SequencerState& state);

void resetStepEvent(StepEvent* step);
void resetLanePattern(LanePattern* pattern);
void resetPatternBank(PatternBank* bank);
void resetPatternSnapshot(PatternSnapshot* snapshot);

const PatternSnapshot& getPatternSnapshot(const SequencerState& state);

} // namespace app::sequencer
