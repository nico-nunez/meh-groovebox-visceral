#include "TestHelpers.h"
#include "TestRunner.h"

#include "app/AppContext.h"
#include "app/GrooveboxEditSession.h"
#include "app/doc/DocAuthoringService.h"
#include "app/doc/metadata/DocMetadata.h"
#include "app/sessions/AudioSession.h"
#include "synth/WavetableBanks.h"
#include "synth/params/ParamDefs.h"
#include "synth/params/ParamSync.h"

namespace {

void initSynthParserGlobals() {
  synth::wavetable::banks::initFactoryBanks();
}

using test::getParseTestWorkspace;
using test::hasDiagnostic;
using test::parseWS;

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

  auto* ws = getParseTestWorkspace();
  auto parsed = parseWS(doc, ws);
  CHECK("parser ok", parsed.ok);
  CHECK("track 0 synth parsed", ws->model.hasSynthState[0]);
  CHECK("track 1 synth parsed", ws->model.hasSynthState[1]);

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);
  auto result = app::doc::submitAuthoredDocRevision(app->documents.authoring, *app, 1, doc);

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

  auto* ws = getParseTestWorkspace();
  auto parsed = parseWS(doc, ws);
  CHECK("parser ok", parsed.ok);
  CHECK("seq parsed", ws->model.sequencer.hasTrackState[0]);
  CHECK("synth parsed", ws->model.hasSynthState[0]);

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);
  auto result = app::doc::submitAuthoredDocRevision(app->documents.authoring, *app, 1, doc);

  CHECK("apply ok", result.ok);
  CHECK("completed", app->documents.authoring.apply.status == app::doc::ApplyStatus::Completed);
  CHECK("admitted doc model", app->documents.authoring.apply.hasLastAdmittedDocModel);
  CHECK("admitted seq",
        app->documents.authoring.apply.lastAdmittedDocModel.sequencer.hasTrackState[0]);
  CHECK("admitted synth", app->documents.authoring.apply.lastAdmittedDocModel.hasSynthState[0]);
  test::publishPending(app);
  CHECK("synth published", app->tracks[0].engine.params[synth::param::OSC1_MIX_LEVEL] == 0.6f);
  app::destroyAppContext(app);
}

static void test_deferred_synth_fields_fail_without_queueing_events() {
  TEST("deferred_synth_fields_fail_without_queueing_events");

  initSynthParserGlobals();

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);
  auto result =
      app::doc::submitAuthoredDocRevision(app->documents.authoring,
                                          *app,
                                          1,
                                          "synth(1, SynthSettings { lfo1 = { rate = 2 } })");

  CHECK("apply failed", !result.ok);
  CHECK("unknown param diagnostic",
        hasDiagnostic(result.diagnostics, app::doc::docdiag::SynthParamUnknown));
  CHECK("no pending apply", !app->documents.pendingApply.ready.load());
  app::destroyAppContext(app);
}

static void test_valid_synth_document_never_emits_apply_not_implemented() {
  TEST("valid_synth_document_never_emits_apply_not_implemented");

  initSynthParserGlobals();

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);
  auto result = app::doc::submitAuthoredDocRevision(
      app->documents.authoring,
      *app,
      1,
      "synth(1, SynthSettings { osc1 = { mix = 0.25 }, "
      "                         svf = { enabled = true, cutoff = 800 } })");

  CHECK("apply ok", result.ok);
  app::destroyAppContext(app);
}

static void test_synth_param_patch_preserves_other_live_params() {
  TEST("synth_param_patch_preserves_other_live_params");

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);
  app->tracks[0].controlProgram.paramValues[synth::param::OSC2_MIX_LEVEL] = 0.8f;
  app->tracks[0].controlProgramValid = true;
  synth::param::sync::setParamDeferred(app->tracks[0].engine, synth::param::OSC2_MIX_LEVEL, 0.8f);

  auto result =
      app::doc::submitAuthoredDocRevision(app->documents.authoring,
                                          *app,
                                          1,
                                          "synth(1, SynthSettings { osc1 = { mix = 0.5 } })");
  CHECK("apply ok", result.ok);
  test::publishPending(app);

  CHECK("osc1 changed", app->tracks[0].engine.params[synth::param::OSC1_MIX_LEVEL] == 0.5f);
  CHECK("osc2 preserved", app->tracks[0].engine.params[synth::param::OSC2_MIX_LEVEL] == 0.8f);
  app::destroyAppContext(app);
}

static void test_sequencer_patch_preserves_synth() {
  TEST("sequencer_patch_preserves_synth");

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);
  app->tracks[0].controlProgram.paramValues[synth::param::OSC1_MIX_LEVEL] = 0.9f;
  app->tracks[0].controlProgramValid = true;
  synth::param::sync::setParamDeferred(app->tracks[0].engine, synth::param::OSC1_MIX_LEVEL, 0.9f);

  auto result = app::doc::submitAuthoredDocRevision(
      app->documents.authoring,
      *app,
      1,
      "track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { note = 64 } } } } })");
  CHECK("apply ok", result.ok);
  test::publishPending(app);

  CHECK("synth preserved", app->tracks[0].engine.params[synth::param::OSC1_MIX_LEVEL] == 0.9f);
  app::destroyAppContext(app);
}

void runDocAuthoredSynthEndToEndTests() {
  SUITE("DocAuthoredSynthEndToEnd");
  test_luals_advertised_synth_shape_parses_and_applies();
  test_mixed_synth_and_sequencer_document_applies();
  test_deferred_synth_fields_fail_without_queueing_events();
  test_valid_synth_document_never_emits_apply_not_implemented();
  test_synth_param_patch_preserves_other_live_params();
  test_sequencer_patch_preserves_synth();
}
