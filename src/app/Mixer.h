#pragma once

#include "app/Constants.h"

#include "dsp/Buffers.h"
#include "dsp/Dynamics.h"
#include "dsp/Math.h"

#include <atomic>

namespace app::mixer {
using dsp::buffers::StereoBufferView;
using dsp::dynamics::PeakLimiter;

struct TrackMixState {
  bool enabled = true;
  float gain = 1.0f;
  float pan = 0.0f; // [-1.0, 1.0];
};

struct MixerSnapshot {
  TrackMixState tracks[MAX_TRACKS]{};
  float masterGain = 1.0f;
  float limiterThreshold = dsp::math::dBToLinear(-1.0f);
};

struct MixerSnapshotSwapResult {
  bool ok = true;
  const char* err = nullptr;
};

struct MixerState {
  MixerSnapshot current{};
  MixerSnapshot pending{};

  std::atomic<bool> pendingReady{false};
  std::atomic<bool> writeInFlight{false};
};

struct MasterBusState {
  StereoBufferView busBuffer{};
  uint32_t busBufferSlot = 0;

  PeakLimiter limiter{};
};

MixerSnapshotSwapResult prepareMixerSnapshotSwap(MixerState& mixer,
                                                 const MixerSnapshot& targetSnapshot);
MixerSnapshotSwapResult commitMixerSnapshotSwap(MixerState& mixer);
void abortMixerSnapshotSwap(MixerState& mixer);
void publishPendingMixerSnapshotIfReady(MixerState& mixer);

} // namespace app::mixer
