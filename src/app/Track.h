#pragma once

#include "dsp/Buffers.h"
#include "synth/Engine.h"
#include "synth/events/EventQueues.h"

#include <cstdint>

namespace app::track {
using dsp::buffers::StereoBufferView;

using synth::Engine;
using synth::events::EngineEventQueue;
using synth::events::MIDIEventQueue;
using synth::events::ParamEventQueue;
using synth::preset::Preset;

struct TrackQueues {
  MIDIEventQueue midi{};
  ParamEventQueue param{};
  EngineEventQueue engine{};
};

struct TrackState {
  Engine engine{};
  TrackQueues queues{};

  Preset preset{};
  bool presetValid = false;

  StereoBufferView outputBuffer{};
  uint32_t outputSlot = 0;
};

} // namespace app::track
