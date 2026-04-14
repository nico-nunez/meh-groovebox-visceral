#pragma once

#include "app/Transport.h"

#include "synth/Engine.h"
#include "synth/events/EventQueues.h"
#include "synth/preset/Preset.h"

#include <cstdint>

namespace app {

namespace audio {
struct DeviceInfo;
}

using transport::TransportEventQueue;
using transport::TransportRuntime;

using synth::Engine;

using synth::preset::Preset;

using synth::events::EngineEventQueue;
using synth::events::MIDIEventQueue;
using synth::events::ParamEventQueue;

inline constexpr uint8_t MAX_TRACKS = 1;
inline constexpr uint8_t MIDI_CHANNEL_UNASSIGNED = 0xFF;

struct PresetStore {
  Preset slots[MAX_TRACKS]{};
  bool valid[MAX_TRACKS] = {};
};

struct TrackQueues {
  MIDIEventQueue midi{};
  ParamEventQueue param{};
  EngineEventQueue engine{};
};

struct TrackRuntime {
  Engine engine{};
  TrackQueues queues{};

  Preset preset{};
  bool presetValid = false;
};

struct AppContext {
  TransportRuntime transport{};
  TransportEventQueue transportQueue{};

  TrackRuntime tracks[MAX_TRACKS]{};

  uint8_t midiChannelMap[16];
  uint8_t currentTrack = 0;
};

AppContext* createAppContext(audio::DeviceInfo deviceInfo);
void destroyAppContext(AppContext* ctx);

// ==================
// Getters
// ==================
inline TrackRuntime* getTrackRuntime(AppContext* ctx, uint8_t track = MAX_TRACKS) {
  if (track >= MAX_TRACKS)
    track = ctx->currentTrack;

  return &ctx->tracks[track];
}

inline TrackQueues* getTrackQueues(AppContext* ctx, uint8_t track = MAX_TRACKS) {
  if (track >= MAX_TRACKS)
    track = ctx->currentTrack;

  return &ctx->tracks[track].queues;
}

inline Engine* getTrackEngine(AppContext* ctx, uint8_t track = MAX_TRACKS) {
  if (track >= MAX_TRACKS)
    track = ctx->currentTrack;

  return &ctx->tracks[track].engine;
}

// ==================
// Event Helpers
// ==================
inline bool pushTransportAction(AppContext* ctx, transport::TransportEvent evt) {
  return ctx->transportQueue.push(evt);
}

inline bool pushMIDIEvent(AppContext* ctx, synth::MIDIEvent evt) {
  uint8_t track = ctx->midiChannelMap[evt.channel];
  if (track == app::MIDI_CHANNEL_UNASSIGNED)
    track = ctx->currentTrack;

  return ctx->tracks[track].queues.midi.push(evt);
}

inline bool pushParamEvent(AppContext* ctx, synth::ParamEvent evt, uint8_t track = MAX_TRACKS) {
  if (track >= MAX_TRACKS)
    track = ctx->currentTrack;

  return ctx->tracks[track].queues.param.push(evt);
}

inline bool pushEngineEvent(AppContext* ctx, synth::EngineEvent evt, uint8_t track = MAX_TRACKS) {
  if (track >= MAX_TRACKS)
    track = ctx->currentTrack;

  return ctx->tracks[track].queues.engine.push(evt);
}
} // namespace app
