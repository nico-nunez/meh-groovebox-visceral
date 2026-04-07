#pragma once

#include "app/SynthSession.h"

#include "synth/Engine.h"
#include "synth/preset/Preset.h"

#include <cstdint>

namespace app {
using session::hSynthSession;

using synth::Engine;
using synth::preset::Preset;

inline constexpr uint8_t MAX_TRACKS = 1;

struct PresetStore {
  Preset slots[MAX_TRACKS]{};
  bool valid[MAX_TRACKS] = {};
};

struct TrackContext {
  Preset* preset = nullptr;
  Engine* engine = nullptr;
  hSynthSession session = nullptr;
};

struct AppContext {
  TrackContext tracks[MAX_TRACKS]{};
  PresetStore presetStore;
  uint8_t currentTrack = 0;
};

inline hSynthSession getTrackSession(AppContext* ctx, uint8_t track = MAX_TRACKS) {
  if (track >= MAX_TRACKS)
    track = ctx->currentTrack;

  return ctx->tracks[track].session;
}

inline Engine* getTrackEngine(AppContext* ctx, uint8_t track = MAX_TRACKS) {
  if (track >= MAX_TRACKS)
    track = ctx->currentTrack;

  return ctx->tracks[track].engine;
}

} // namespace app
