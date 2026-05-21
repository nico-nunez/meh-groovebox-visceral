#include "app/display/DisplayState.h"

#include "app/AppContext.h"
#include "app/Sequencer.h"
#include "app/display/SynthDisplayState.h"

#include "synth/events/Events.h"

namespace app::display {
namespace {

DocumentStatusSnapshot
makeDocumentStatusSnapshot(const app::editor::AuthoredDocEditorState& editor) {
  DocumentStatusSnapshot snapshot{};
  snapshot.dirty = editor.buffer.dirty;
  snapshot.applyRevision = editor.buffer.applyRevision;
  snapshot.lastAppliedRevision = editor.buffer.lastAppliedRevision;
  snapshot.backendDiagnosticCount = static_cast<uint32_t>(editor.backendDiagnostics.size());
  snapshot.lualsDiagnosticCount = static_cast<uint32_t>(editor.luals.diagnostics.size());
  snapshot.lualsStatus = editor.luals.status;
  return snapshot;
}

MIDIRoutingSnapshot makeMIDIRoutingSnapshot(const AppContext& app) {
  MIDIRoutingSnapshot snapshot{};
  snapshot.stickyTrack = app.midiStickyTrack;
  snapshot.followsCurrentTrack = app.midiStickyTrack == MIDI_CHANNEL_UNASSIGNED;
  for (uint8_t ch = 0; ch < MAX_MIDI_CHANNELS; ++ch)
    snapshot.channelMap[ch] = app.midiChannelMap[ch];
  return snapshot;
}

TrackSnapshot makeTrackSnapshot(const AppContext& app) {
  TrackSnapshot snapshot{};
  snapshot.selectedTrack = app.currentTrack;
  snapshot.midiStickyTrack = app.midiStickyTrack;
  snapshot.midiFollowsCurrentTrack = app.midiStickyTrack == MIDI_CHANNEL_UNASSIGNED;
  return snapshot;
}

uint32_t inactiveIndex(const DisplayPublication& publication) {
  const uint32_t current = publication.readIndex.load(std::memory_order_relaxed);
  return 1u - current;
}

void fillTransportTelemetry(TransportTelemetry& out,
                            const app::transport::TransportState& transport) {
  out.mode = transport.mode;
  out.bpm = transport.bpm;
  out.beatPosition = transport.beatPosition;
  out.samplePosition = transport.samplePosition;
}

void fillMixerTelemetry(MixerRuntimeTelemetry& out, const app::mixer::TrackMixState& mixer) {
  out.enabled = mixer.enabled;
  out.gain = mixer.gain;
  out.pan = mixer.pan;
}

} // namespace

void publishDisplayRuntimeTelemetry(DisplayPublication& publication,
                                    const DisplayRuntimeTelemetry& telemetry) {
  const uint32_t writeIndex = inactiveIndex(publication);
  publication.buffers[writeIndex] = telemetry;
  publication.readIndex.store(writeIndex, std::memory_order_release);
}

DisplayRuntimeTelemetry readDisplayRuntimeTelemetry(const DisplayPublication& publication) {
  const uint32_t index = publication.readIndex.load(std::memory_order_acquire);
  return publication.buffers[index];
}

DisplayRuntimeTelemetry makeDisplayRuntimeTelemetry(const AppContext& app) {
  DisplayRuntimeTelemetry telemetry = readDisplayRuntimeTelemetry(app.displayPublication);
  telemetry.sequence += 1;

  fillTransportTelemetry(telemetry.transport, app.transport);

  for (uint8_t track = 0; track < MAX_TRACKS; ++track) {
    fillSynthRuntimeTelemetry(telemetry.tracks[track].synth, app.tracks[track].engine);
    fillMixerTelemetry(telemetry.tracks[track].mixer, app.mixer.current.tracks[track]);
  }

  return telemetry;
}

void recordDisplayMIDIEvent(DisplayPublication& publication,
                            uint8_t track,
                            const synth::MIDIEvent& evt) {
  DisplayRuntimeTelemetry telemetry = readDisplayRuntimeTelemetry(publication);
  if (track >= MAX_TRACKS)
    return;

  MIDIRuntimeTelemetry& midi = telemetry.tracks[track].midi;
  midi.eventCounter += 1;
  midi.lastChannel = evt.channel;
  midi.lastType = static_cast<uint8_t>(evt.type);
  midi.hasLastEvent = true;

  switch (evt.type) {
  case synth::MIDIEvent::Type::NoteOn:
    midi.lastNote = evt.data.noteOn.note;
    midi.lastVelocity = evt.data.noteOn.velocity;
    break;
  case synth::MIDIEvent::Type::NoteOff:
    midi.lastNote = evt.data.noteOff.note;
    midi.lastVelocity = evt.data.noteOff.velocity;
    break;
  default:
    break;
  }

  publishDisplayRuntimeTelemetry(publication, telemetry);
}

DisplayControlSnapshot makeDisplayControlSnapshot(const AppContext& app) {
  DisplayControlSnapshot snapshot{};
  snapshot.track = makeTrackSnapshot(app);
  snapshot.sequencer = makeSequencerPatternSnapshot(app.sequencer, snapshot.track.selectedTrack);
  snapshot.document = makeDocumentStatusSnapshot(app.editor.authoredEditor);
  snapshot.midi = makeMIDIRoutingSnapshot(app);
  return snapshot;
}

SequencerPatternSnapshot makeSequencerPatternSnapshot(const app::sequencer::SequencerState& seq,
                                                      uint8_t lane) {
  SequencerPatternSnapshot snapshot{};

  const auto bankResult = app::sequencer::getPatternBank(seq, lane);
  if (!bankResult.ok || !bankResult.value)
    return snapshot;

  const app::sequencer::PatternBank& bank = *bankResult.value;
  snapshot.activeSlot = bank.activeSlot;

  for (uint8_t slot = 0; slot < app::sequencer::PATTERNS_PER_LANE; ++slot)
    snapshot.occupiedSlots[slot] = bank.slots[slot].occupied;

  if (bank.activeSlot == app::sequencer::INVALID_PATTERN_SLOT)
    return snapshot;
  if (bank.activeSlot >= app::sequencer::PATTERNS_PER_LANE)
    return snapshot;
  if (!bank.slots[bank.activeSlot].occupied)
    return snapshot;

  const app::sequencer::LanePattern& pattern = bank.slots[bank.activeSlot].pattern;
  snapshot.hasActivePattern = true;
  snapshot.numSteps = pattern.numSteps;
  snapshot.stepsPerBeat = pattern.stepsPerBeat;

  for (uint8_t step = 0; step < pattern.numSteps; ++step) {
    const app::sequencer::StepEvent& event = pattern.steps[step];
    if (event.active)
      snapshot.activeStepCount += 1;
    if (event.noteOn)
      snapshot.noteOnStepCount += 1;
    snapshot.paramLockCount += event.numLocks;
  }

  return snapshot;
}

DisplayDashboardSnapshot mergeDisplaySnapshot(const DisplayControlSnapshot& control,
                                              const DisplayRuntimeTelemetry& runtime) {
  DisplayDashboardSnapshot snapshot{};
  snapshot.track = control.track;
  snapshot.sequencer = control.sequencer;
  snapshot.document = control.document;
  snapshot.midi = control.midi;

  uint8_t selectedTrack = control.track.selectedTrack;
  if (selectedTrack >= MAX_TRACKS)
    selectedTrack = 0;

  snapshot.transport.mode = runtime.transport.mode;
  snapshot.transport.bpm = runtime.transport.bpm;
  snapshot.transport.beatPosition = runtime.transport.beatPosition;
  snapshot.transport.samplePosition = runtime.transport.samplePosition;
  snapshot.transport.musical =
      app::transport::formatMusicalPosition(runtime.transport.beatPosition);

  snapshot.mixer.enabled = runtime.tracks[selectedTrack].mixer.enabled;
  snapshot.mixer.gain = runtime.tracks[selectedTrack].mixer.gain;
  snapshot.mixer.pan = runtime.tracks[selectedTrack].mixer.pan;

  snapshot.synth = makeSynthSummarySnapshot(runtime.tracks[selectedTrack].synth);
  snapshot.midi.selectedTrackActivity = runtime.tracks[selectedTrack].midi;

  return snapshot;
}

DisplayDashboardSnapshot makeDisplayDashboardSnapshot(const AppContext& app) {
  const DisplayControlSnapshot control = makeDisplayControlSnapshot(app);
  const DisplayRuntimeTelemetry runtime = readDisplayRuntimeTelemetry(app.displayPublication);
  return mergeDisplaySnapshot(control, runtime);
}

} // namespace app::display
