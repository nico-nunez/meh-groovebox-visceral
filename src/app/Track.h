#pragma once

#include "synth/Engine.h"
#include "synth/events/EventQueues.h"

namespace app::track {
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
};

} // namespace app::track
