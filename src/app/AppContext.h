#pragma once

#include "app/BlockScheduler.h"
#include "app/Constants.h"
#include "app/ControlEvents.h"
#include "app/FileWatchApply.h"
#include "app/GrooveboxEditSession.h"
#include "app/GrooveboxPaths.h"
#include "app/Mixer.h"
#include "app/Sequencer.h"
#include "app/Track.h"
#include "app/Transport.h"
#include "app/display/DisplayState.h"
#include "app/doc/DocAuthoringService.h"
#include "app/editor/AuthoredDocEditor.h"

#include "dsp/Buffers.h"
#include "synth/midi/MIDIParamMapping.h"
#include "synth/params/ParamDefs.h"

#include <cstdint>

namespace app {

namespace audio {
struct DeviceInfo;
}
using display::DisplayPublication;

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

using doc::DocAuthoringService;
using editor::AuthoredDocEditorState;

struct DocumentRuntime {
  DocAuthoringService authoring{};
  PendingGrooveboxApply pendingApply{};
  ExternalFileApplyState externalFileApply{};
};

struct EditorRuntime {
  AuthoredDocEditorState authoredEditor{};
  bool internalEditor = true;
};

struct AppContext {
  TransportState transport{};
  ControlEventQueue controlQueue{};

  TrackState tracks[MAX_TRACKS]{};
  SequencerState sequencer{};
  BlockSchedulerWorkspace blockScheduler{};

  uint8_t midiChannelMap[16];
  uint8_t currentTrack = 0;
  uint8_t midiStickyTrack = MIDI_CHANNEL_UNASSIGNED;

  StereoBufferPool renderBufferPool{};
  MixerState mixer{};
  MasterBusState masterBus{};

  DocumentRuntime documents{};
  EditorRuntime editor{};
  GrooveboxPaths grooveboxPaths{};

  DisplayPublication displayPublication{};
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

inline void updateTrackControlProgramParam(track::TrackState* track,
                                           synth::param::ParamID id,
                                           float value) {
  if (!track || id == synth::param::PARAM_UNKNOWN || id >= synth::param::PARAM_COUNT)
    return;

  track->controlProgram.paramValues[static_cast<int>(id)] = synth::param::clampParam(id, value);
  track->controlProgramValid = true;
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

  const bool ok = ctx->tracks[track].queues.midi.push(evt);
  if (ok && evt.type == synth::MIDIEvent::Type::ControlChange) {
    synth::param::ParamID id = synth::param::PARAM_UNKNOWN;
    float value = 0.0f;
    if (synth::midi::ccToPersistentParamValue(evt.data.cc.number, evt.data.cc.value, &id, &value))
      updateTrackControlProgramParam(&ctx->tracks[track], id, value);
  }
  if (ok)
    app::display::recordDisplayMIDIEvent(ctx->displayPublication, track, evt);
  return ok;
}

inline bool pushParamEvent(AppContext* ctx, synth::ParamEvent evt, uint8_t track = MAX_TRACKS) {
  if (track >= MAX_TRACKS)
    track = ctx->currentTrack;

  const auto id = static_cast<synth::param::ParamID>(evt.id);
  const bool ok = ctx->tracks[track].queues.param.push(evt);
  if (ok)
    updateTrackControlProgramParam(&ctx->tracks[track], id, evt.value);
  return ok;
}

inline bool pushEngineEvent(AppContext* ctx, synth::EngineEvent evt, uint8_t track = MAX_TRACKS) {
  if (track >= MAX_TRACKS)
    track = ctx->currentTrack;

  return ctx->tracks[track].queues.engine.push(evt);
}

} // namespace app
