#pragma once

#include "app/Constants.h"
#include "app/ControlEvents.h"
#include "app/Mixer.h"
#include "app/Sequencer.h"
#include "app/Track.h"
#include "app/Transport.h"

#include "dsp/Buffers.h"

#include <cstdint>

namespace app {

namespace audio {
struct DeviceInfo;
}

using events::ControlEvent;
using events::ControlEventQueue;

using dsp::buffers::StereoBufferPool;

using mixer::MasterBusState;
using mixer::MixerState;

using transport::TransportState;

using sequencer::PatternStore;
using sequencer::SequencerState;

using track::TrackQueues;
using track::TrackState;

using synth::Engine;

struct AppContext {
  TransportState transport{};
  ControlEventQueue controlQueue{};

  TrackState tracks[MAX_TRACKS]{};
  SequencerState sequencer{};

  uint8_t midiChannelMap[16];
  uint8_t currentTrack = 0;
  uint8_t midiStickyTrack = MIDI_CHANNEL_UNASSIGNED;

  StereoBufferPool renderBufferPool{};
  MixerState mixer{};
  MasterBusState masterBus{};
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
inline VoidResult pushControlEvent(AppContext* ctx, ControlEvent evt) {
  if (ctx->controlQueue.push(evt))
    return {true, nullptr};
  return {false, "control queue full"};
}

inline bool pushMIDIEvent(AppContext* ctx, synth::MIDIEvent evt) {
  uint8_t track = ctx->currentTrack;

  if (ctx->midiStickyTrack != MIDI_CHANNEL_UNASSIGNED)
    track = ctx->midiStickyTrack;

  if (evt.channel < MAX_MIDI_CHANNELS) {
    uint8_t mappedTrack = ctx->midiChannelMap[evt.channel];
    if (mappedTrack != MIDI_CHANNEL_UNASSIGNED)
      track = mappedTrack;
  }

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
