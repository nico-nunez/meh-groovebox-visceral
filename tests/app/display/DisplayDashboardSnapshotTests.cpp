#include "TestRunner.h"

#include "app/AppContext.h"
#include "app/Sequencer.h"
#include "app/display/DisplayState.h"
#include "app/display/SynthDisplayState.h"

#include "synth/params/ParamDefs.h"

namespace {

void setParam(synth::Engine& engine, synth::param::ParamID id, float value) {
  engine.params[static_cast<int>(id)] = value;
}

void createCommittedPattern(app::sequencer::SequencerState& seq) {
  CHECK("begin edit", app::sequencer::beginPatternEdit(seq, false).ok);
  CHECK("steps", app::sequencer::setPatternNumSteps(seq, 0, 8).ok);
  CHECK("spb", app::sequencer::setPatternStepsPerBeat(seq, 0, 4).ok);

  app::sequencer::StepEvent step0{};
  step0.active = true;
  step0.noteOn = true;
  step0.note = 60;
  step0.velocity = 100;
  step0.numLocks = 1;
  step0.locks[0] = {static_cast<uint8_t>(synth::param::OSC1_MIX_LEVEL), 0.5f};
  CHECK("set step 0", app::sequencer::setStep(seq, 0, 0, step0).ok);

  app::sequencer::StepEvent step4{};
  step4.active = true;
  step4.noteOn = false;
  CHECK("set step 4", app::sequencer::setStep(seq, 0, 4, step4).ok);

  CHECK("commit", app::sequencer::commitPattern(seq).ok);
}

} // namespace

static void test_control_snapshot_copies_track_midi_and_document_state() {
  TEST("control_snapshot_copies_track_midi_and_document_state");

  app::AppContext app{};
  app.currentTrack = 2;
  app.midiStickyTrack = 1;
  for (uint8_t ch = 0; ch < app::MAX_MIDI_CHANNELS; ++ch)
    app.midiChannelMap[ch] = app::MIDI_CHANNEL_UNASSIGNED;
  app.midiChannelMap[3] = 2;

  app.authoredEditor.buffer.dirty = true;
  app.authoredEditor.buffer.applyRevision = 7;
  app.authoredEditor.buffer.lastAppliedRevision = 5;
  app.authoredEditor.backendDiagnostics.push_back(app::doc::DocDiagnostic{});
  app.authoredEditor.luals.diagnostics.push_back(app::editor::LuaLSDiagnostic{});
  app.authoredEditor.luals.status = app::editor::LanguageServiceStatus::Succeeded;

  const auto snapshot = app::display::makeDisplayControlSnapshot(app);

  CHECK("selected track", snapshot.track.selectedTrack == 2);
  CHECK("sticky", snapshot.track.midiStickyTrack == 1);
  CHECK("does not follow", !snapshot.track.midiFollowsCurrentTrack);
  CHECK("mapped channel", snapshot.midi.channelMap[3] == 2);
  CHECK("dirty", snapshot.document.dirty);
  CHECK("apply rev", snapshot.document.applyRevision == 7);
  CHECK("last applied", snapshot.document.lastAppliedRevision == 5);
  CHECK("backend diag count", snapshot.document.backendDiagnosticCount == 1);
  CHECK("luals diag count", snapshot.document.lualsDiagnosticCount == 1);
  CHECK("luals status",
        snapshot.document.lualsStatus == app::editor::LanguageServiceStatus::Succeeded);
}

static void test_sequencer_pattern_snapshot_summarizes_active_pattern() {
  TEST("sequencer_pattern_snapshot_summarizes_active_pattern");

  app::sequencer::SequencerState seq{};
  createCommittedPattern(seq);

  const auto snapshot = app::display::makeSequencerPatternSnapshot(seq, 0);

  CHECK("has active", snapshot.hasActivePattern);
  CHECK("active slot", snapshot.activeSlot == 0);
  CHECK("slot occupied", snapshot.occupiedSlots[0]);
  CHECK("num steps", snapshot.numSteps == 8);
  CHECK("steps per beat", snapshot.stepsPerBeat == 4);
  CHECK("active count", snapshot.activeStepCount == 2);
  CHECK("note on count", snapshot.noteOnStepCount == 1);
  CHECK("lock count", snapshot.paramLockCount == 1);
}

static void test_sequencer_pattern_snapshot_handles_missing_pattern() {
  TEST("sequencer_pattern_snapshot_handles_missing_pattern");

  app::sequencer::SequencerState seq{};
  const auto snapshot = app::display::makeSequencerPatternSnapshot(seq, 0);

  CHECK("no active", !snapshot.hasActivePattern);
  CHECK("invalid slot", snapshot.activeSlot == app::sequencer::INVALID_PATTERN_SLOT);
  CHECK("no steps", snapshot.numSteps == 0);
}

static void test_runtime_merge_uses_runtime_transport_mixer_synth_and_midi() {
  TEST("runtime_merge_uses_runtime_transport_mixer_synth_and_midi");

  app::display::DisplayControlSnapshot control{};
  control.track.selectedTrack = 1;
  control.track.midiFollowsCurrentTrack = true;
  control.document.dirty = true;

  app::display::DisplayRuntimeTelemetry runtime{};
  runtime.transport.mode = app::transport::TransportMode::Playing;
  runtime.transport.bpm = 130.0f;
  runtime.transport.beatPosition = 4.25;
  runtime.transport.samplePosition = 512;
  runtime.tracks[1].mixer.enabled = false;
  runtime.tracks[1].mixer.gain = 0.4f;
  runtime.tracks[1].mixer.pan = 0.25f;
  runtime.tracks[1].synth.noteCount = 6;
  runtime.tracks[1].synth.paramCount = 1;
  runtime.tracks[1].synth.params[0].id = synth::param::MASTER_GAIN;
  runtime.tracks[1].synth.params[0].value = 1.25f;
  runtime.tracks[1].midi.eventCounter = 3;
  runtime.tracks[1].midi.lastNote = 65;
  runtime.tracks[1].midi.hasLastEvent = true;

  const auto snapshot = app::display::mergeDisplaySnapshot(control, runtime);
  const auto* master = app::display::findSynthParam(snapshot.synth, synth::param::MASTER_GAIN);

  CHECK("transport mode", snapshot.transport.mode == app::transport::TransportMode::Playing);
  CHECK("transport bpm", snapshot.transport.bpm == 130.0f);
  CHECK("bar", snapshot.transport.musical.bar == 2);
  CHECK("beat", snapshot.transport.musical.beat == 1);
  CHECK("mixer enabled", !snapshot.mixer.enabled);
  CHECK("mixer gain", snapshot.mixer.gain == 0.4f);
  CHECK("synth note count", snapshot.synth.noteCount == 6);
  CHECK("master found", master != nullptr);
  CHECK("master value", master && master->value == 1.25f);
  CHECK("midi counter", snapshot.midi.selectedTrackActivity.eventCounter == 3);
  CHECK("midi note", snapshot.midi.selectedTrackActivity.lastNote == 65);
}

static void test_runtime_merge_clamps_invalid_selected_track_to_zero() {
  TEST("runtime_merge_clamps_invalid_selected_track_to_zero");

  app::display::DisplayControlSnapshot control{};
  control.track.selectedTrack = app::MAX_TRACKS;

  app::display::DisplayRuntimeTelemetry runtime{};
  runtime.tracks[0].mixer.gain = 0.75f;

  const auto snapshot = app::display::mergeDisplaySnapshot(control, runtime);

  CHECK("uses track zero", snapshot.mixer.gain == 0.75f);
}

static void test_dashboard_snapshot_reads_latest_publication_and_control_state() {
  TEST("dashboard_snapshot_reads_latest_publication_and_control_state");

  app::AppContext app{};
  app.currentTrack = 0;
  app.authoredEditor.buffer.dirty = true;
  setParam(app.tracks[0].engine, synth::param::MASTER_GAIN, 1.1f);

  app::display::DisplayRuntimeTelemetry runtime = app::display::makeDisplayRuntimeTelemetry(app);
  runtime.transport.bpm = 140.0f;
  app::display::publishDisplayRuntimeTelemetry(app.displayPublication, runtime);

  const auto snapshot = app::display::makeDisplayDashboardSnapshot(app);
  const auto* master = app::display::findSynthParam(snapshot.synth, synth::param::MASTER_GAIN);

  CHECK("dirty", snapshot.document.dirty);
  CHECK("bpm from publication", snapshot.transport.bpm == 140.0f);
  CHECK("master found", master != nullptr);
  CHECK("master value", master && master->value == 1.1f);
}

void runDisplayDashboardSnapshotTests() {
  SUITE("DisplayDashboardSnapshot");
  test_control_snapshot_copies_track_midi_and_document_state();
  test_sequencer_pattern_snapshot_summarizes_active_pattern();
  test_sequencer_pattern_snapshot_handles_missing_pattern();
  test_runtime_merge_uses_runtime_transport_mixer_synth_and_midi();
  test_runtime_merge_clamps_invalid_selected_track_to_zero();
  test_dashboard_snapshot_reads_latest_publication_and_control_state();
}
