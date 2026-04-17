#pragma once

#include "app/Constants.h"
#include "app/Sequencer.h"
#include "app/Track.h"
#include "app/Transport.h"

#include <cstdint>

namespace app {

namespace audio {
struct DeviceInfo;
}

using transport::TransportEventQueue;
using transport::TransportState;

using sequencer::PatternStore;
using sequencer::SequencerState;

using track::TrackQueues;
using track::TrackState;

using synth::Engine;

struct AppContext {
  TransportState transport{};
  TransportEventQueue transportQueue{};

  TrackState tracks[MAX_TRACKS]{};

  SequencerState sequencer{};

  uint8_t midiChannelMap[16];
  uint8_t currentTrack = 0;
};

AppContext* createAppContext(audio::DeviceInfo deviceInfo);
void destroyAppContext(AppContext* ctx);

// ==================
// Getters
// ==================
inline TrackState* getTrack(AppContext* ctx, uint8_t track = MAX_TRACKS) {
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
