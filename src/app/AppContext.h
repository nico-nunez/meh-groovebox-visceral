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

struct TrackSession {
  MIDIEventQueue midiQueue{};
  ParamEventQueue paramQueue{};
  EngineEventQueue engineQueue{};
};

struct TrackContext {
  TrackSession session{};
  Preset* preset = nullptr;
  Engine* engine = nullptr;
};

struct AppContext {
  TransportRuntime transport{};
  TransportEventQueue transportQueue{};

  Engine engines[MAX_TRACKS];
  TrackContext tracks[MAX_TRACKS]{};
  PresetStore presetStore;

  uint8_t midiChannelMap[16];
  uint8_t currentTrack = 0;
};

inline TrackSession* getTrackSession(AppContext* ctx, uint8_t track = MAX_TRACKS) {
  if (track >= MAX_TRACKS)
    track = ctx->currentTrack;

  return &ctx->tracks[track].session;
}

inline Engine* getTrackEngine(AppContext* ctx, uint8_t track = MAX_TRACKS) {
  if (track >= MAX_TRACKS)
    track = ctx->currentTrack;

  return ctx->tracks[track].engine;
}

AppContext* createAppContext(audio::DeviceInfo deviceInfo);
void destroyAppContext(AppContext* ctx);

// ==== Event push helpers ====
inline bool pushTransportAction(AppContext* ctx, transport::TransportEvent evt) {
  return ctx->transportQueue.push(evt);
}

inline bool pushMIDIEvent(AppContext* ctx, synth::MIDIEvent evt) {
  uint8_t track = ctx->midiChannelMap[evt.channel];
  if (track == app::MIDI_CHANNEL_UNASSIGNED)
    track = ctx->currentTrack;

  return ctx->tracks[track].session.midiQueue.push(evt);
}

inline bool pushParamEvent(AppContext* ctx, synth::ParamEvent evt, uint8_t track = MAX_TRACKS) {
  if (track >= MAX_TRACKS)
    track = ctx->currentTrack;

  return ctx->tracks[track].session.paramQueue.push(evt);
}

inline bool pushEngineEvent(AppContext* ctx, synth::EngineEvent evt, uint8_t track = MAX_TRACKS) {
  if (track >= MAX_TRACKS)
    track = ctx->currentTrack;

  return ctx->tracks[track].session.engineQueue.push(evt);
}
} // namespace app
