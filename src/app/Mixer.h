#pragma once

#include "app/Constants.h"

#include "dsp/Buffers.h"
#include "dsp/Dynamics.h"
#include "dsp/Math.h"

namespace app::mixer {
using dsp::buffers::StereoBufferView;
using dsp::dynamics::PeakLimiter;

struct TrackMixState {
  bool enabled = true;
  float gain = 1.0f;
};

struct MixerState {
  TrackMixState tracks[MAX_TRACKS]{};
  float masterGain = 1.0f;
  float limiterThreshold = dsp::math::dBToLinear(-1.0f);
};

struct MasterBusState {
  StereoBufferView busBuffer{};
  uint32_t busBufferSlot = 0;

  PeakLimiter limiter{};
};

} // namespace app::mixer
