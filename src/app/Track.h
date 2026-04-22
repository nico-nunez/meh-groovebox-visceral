#pragma once

#include "dsp/Buffers.h"
#include "synth/Engine.h"
#include "synth/events/EventQueues.h"
#include "synth/events/Events.h"

#include <cstdint>

namespace app::track {
using dsp::buffers::StereoBufferView;

using synth::Engine;
using synth::events::EngineEventQueue;
using synth::events::MIDIEventQueue;
using synth::events::ParamEventQueue;
using synth::events::ScheduledEvent;
using synth::preset::Preset;

inline constexpr uint32_t MAX_EVENTS_PER_TRACK = 512;

struct ScheduledEventBuffer {
  ScheduledEvent buffer[MAX_EVENTS_PER_TRACK]{};
  uint32_t count = 0;
  uint32_t droppedEvents = 0;

  void clear() { count = 0; }

  bool push(const ScheduledEvent& evt) {
    if (count >= MAX_EVENTS_PER_TRACK) {
      ++droppedEvents;
      return false;
    }

    buffer[count++] = evt;
    return true;
  }
};

struct TrackQueues {
  MIDIEventQueue midi{};
  ParamEventQueue param{};
  EngineEventQueue engine{};
};

struct TrackState {
  Engine engine{};
  TrackQueues queues{};
  ScheduledEventBuffer events{};

  Preset preset{};
  bool presetValid = false;

  StereoBufferView outputBuffer{};
  uint32_t outputSlot = 0;
};

// ======================
// Track Binding Helpers
// ======================

} // namespace app::track
