#include "TestHelpers.h"
#include "TestRunner.h"

#include "app/AppContext.h"
#include "app/GrooveboxEditSession.h"
#include "app/doc/DocAuthoringService.h"
#include "app/doc/DocMetadata.h"
#include "app/sessions/AudioSession.h"
#include "synth/WavetableBanks.h"
#include "synth/params/ParamDefs.h"

namespace {

void initSynthParserGlobals() {
  synth::wavetable::banks::initFactoryBanks();
}

using test::hasDiagnostic;
using test::parseDoc;

app::AppContext* makeContext() {
  app::audio::DeviceInfo device{};
  device.sampleRate = 48000;
  device.bufferFrameSize = 64;
  device.numChannels = 2;
  return app::createAppContext(device);
}

} // namespace

static void test_luals_advertised_synth_shape_parses_and_applies() {
  TEST("luals_advertised_synth_shape_parses_and_applies");

  const char* doc =
      "local bass = TrackSettings()\n"
      "bass.synth = SynthSettings {\n"
      "  osc1 = { enabled = true, bank = 'saw', mix = 0.8, octave = -1, scan = 0.1 },\n"
      "  ampEnv = { attack = 5, decay = 120, sustain = 0.7, release = 180 },\n"
      "  svf = { enabled = true, mode = 'lp', cutoff = 1200, resonance = 0.2 },\n"
      "  fx = { delay = { enabled = true, mix = 0.25 }, "
      "         reverb = { enabled = true, mix = 0.15 } },\n"
      "}\n"
      "track(1, bass)\n"
      "synth(2, SynthSettings { master = { gain = 0.9 }, "
      "                         unison = { enabled = true, voices = 3 } })\n";

  auto parsed = parseDoc(doc);
  CHECK("parser ok", parsed.ok);
  CHECK("track 0 synth parsed", parsed.model.hasSynthState[0]);
  CHECK("track 1 synth parsed", parsed.model.hasSynthState[1]);

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);
  auto result = app::doc::applySequencerRevision(app->docAuthoring, *app, 1, doc);

  CHECK("apply ok", result.ok);
  test::publishPending(app);
  CHECK("track 0 published", app->tracks[0].engine.params[synth::param::OSC1_MIX_LEVEL] == 0.8f);
  CHECK("track 1 published", app->tracks[1].engine.params[synth::param::MASTER_GAIN] == 0.9f);
  app::destroyAppContext(app);
}

static void test_mixed_synth_and_sequencer_document_applies() {
  TEST("mixed_synth_and_sequencer_document_applies");

  const char* doc =
      "track(1, TrackSettings {\n"
      "  synth = SynthSettings { osc1 = { bank = 'saw', mix = 0.6 } },\n"
      "  patterns = { [1] = { numSteps = 1, stepsPerBeat = 4, "
      "                       steps = { { active = true, note = 48, velocity = 100 } } } },\n"
      "  activeSlot = 1,\n"
      "})\n";

  auto parsed = parseDoc(doc);
  CHECK("parser ok", parsed.ok);
  CHECK("seq parsed", parsed.model.sequencer.hasTrackState[0]);
  CHECK("synth parsed", parsed.model.hasSynthState[0]);

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);
  auto result = app::doc::applySequencerRevision(app->docAuthoring, *app, 1, doc);

  CHECK("apply ok", result.ok);
  CHECK("completed", app->docAuthoring.apply.status == app::doc::ApplyStatus::Completed);
  CHECK("admitted doc model", app->docAuthoring.apply.hasLastAdmittedDocModel);
  CHECK("admitted seq", app->docAuthoring.apply.lastAdmittedDocModel.sequencer.hasTrackState[0]);
  CHECK("admitted synth", app->docAuthoring.apply.lastAdmittedDocModel.hasSynthState[0]);
  test::publishPending(app);
  CHECK("synth published", app->tracks[0].engine.params[synth::param::OSC1_MIX_LEVEL] == 0.6f);
  app::destroyAppContext(app);
}

static void test_deferred_synth_fields_fail_without_queueing_events() {
  TEST("deferred_synth_fields_fail_without_queueing_events");

  initSynthParserGlobals();

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);
  auto result = app::doc::applySequencerRevision(app->docAuthoring,
                                                 *app,
                                                 1,
                                                 "synth(1, SynthSettings { lfo1 = { rate = 2 } })");

  CHECK("apply failed", !result.ok);
  CHECK("unknown param diagnostic",
        hasDiagnostic(result.diagnostics, app::doc::docdiag::SynthParamUnknown));
  CHECK("no pending apply", !app->pendingGrooveboxApply.ready.load());
  app::destroyAppContext(app);
}

static void test_valid_synth_document_never_emits_apply_not_implemented() {
  TEST("valid_synth_document_never_emits_apply_not_implemented");

  initSynthParserGlobals();

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);
  auto result = app::doc::applySequencerRevision(
      app->docAuthoring,
      *app,
      1,
      "synth(1, SynthSettings { osc1 = { mix = 0.25 }, "
      "                         svf = { enabled = true, cutoff = 800 } })");

  CHECK("apply ok", result.ok);
  app::destroyAppContext(app);
}

void runDocAuthoredSynthEndToEndTests() {
  SUITE("DocAuthoredSynthEndToEnd");
  test_luals_advertised_synth_shape_parses_and_applies();
  test_mixed_synth_and_sequencer_document_applies();
  test_deferred_synth_fields_fail_without_queueing_events();
  test_valid_synth_document_never_emits_apply_not_implemented();
}
