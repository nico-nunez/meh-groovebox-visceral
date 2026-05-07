#include "TestRunner.h"

#include "app/AppContext.h"
#include "app/doc/DocAuthoringService.h"
#include "app/doc/DocMetadata.h"
#include "app/doc/DocSequencerParser.h"
#include "synth/WavetableBanks.h"
#include "synth/params/ParamDefs.h"

namespace {

app::doc::AuthoredDocumentNormalizeResult parseDoc(const char* text) {
  synth::wavetable::banks::initFactoryBanks();
  return app::doc::parseAndNormalizeAuthoredDocument(1, 7, text);
}

void initSynthParserGlobals() {
  synth::wavetable::banks::initFactoryBanks();
}

bool hasDiagnostic(const app::doc::DocDiagnostics& diagnostics, const char* code) {
  for (const auto& diagnostic : diagnostics) {
    if (diagnostic.code == code)
      return true;
  }
  return false;
}

bool popParam(app::AppContext& app, uint8_t trackIndex, synth::ParamEvent& out) {
  return app.tracks[trackIndex].queues.param.pop(out);
}

bool hasParamEvent(app::AppContext& app,
                   uint8_t trackIndex,
                   synth::param::ParamID id,
                   float value) {
  synth::ParamEvent event{};
  while (popParam(app, trackIndex, event)) {
    if (event.id == id && event.value == value)
      return true;
  }
  return false;
}

bool anyParamEvent(app::AppContext& app, uint8_t trackIndex) {
  synth::ParamEvent event{};
  return popParam(app, trackIndex, event);
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

  app::doc::DocAuthoringService service{};
  app::AppContext app{};
  auto result = app::doc::applySequencerRevision(service, app, 1, doc);

  CHECK("apply ok", result.ok);
  CHECK("track 0 event", hasParamEvent(app, 0, synth::param::OSC1_MIX_LEVEL, 0.8f));
  CHECK("track 1 event", hasParamEvent(app, 1, synth::param::MASTER_GAIN, 0.9f));
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

  app::doc::DocAuthoringService service{};
  app::AppContext app{};
  auto result = app::doc::applySequencerRevision(service, app, 1, doc);

  CHECK("apply ok", result.ok);
  CHECK("completed", service.apply.status == app::doc::ApplyStatus::Completed);
  CHECK("admitted doc model", service.apply.hasLastAdmittedDocModel);
  CHECK("admitted seq", service.apply.lastAdmittedDocModel.sequencer.hasTrackState[0]);
  CHECK("admitted synth", service.apply.lastAdmittedDocModel.hasSynthState[0]);
  CHECK("synth event", hasParamEvent(app, 0, synth::param::OSC1_MIX_LEVEL, 0.6f));
}

static void test_deferred_synth_fields_fail_without_queueing_events() {
  TEST("deferred_synth_fields_fail_without_queueing_events");

  initSynthParserGlobals();

  app::doc::DocAuthoringService service{};
  app::AppContext app{};
  auto result = app::doc::applySequencerRevision(service,
                                                 app,
                                                 1,
                                                 "synth(1, SynthSettings { lfo1 = { rate = 2 } })");

  CHECK("apply failed", !result.ok);
  CHECK("unknown param diagnostic",
        hasDiagnostic(result.diagnostics, app::doc::docdiag::SynthParamUnknown));
  CHECK("no queued event", !anyParamEvent(app, 0));
}

static void test_valid_synth_document_never_emits_apply_not_implemented() {
  TEST("valid_synth_document_never_emits_apply_not_implemented");

  initSynthParserGlobals();

  app::doc::DocAuthoringService service{};
  app::AppContext app{};
  auto result = app::doc::applySequencerRevision(
      service,
      app,
      1,
      "synth(1, SynthSettings { osc1 = { mix = 0.25 }, "
      "                         svf = { enabled = true, cutoff = 800 } })");

  CHECK("apply ok", result.ok);
}

void runDocAuthoredSynthEndToEndTests() {
  SUITE("DocAuthoredSynthEndToEnd");
  test_luals_advertised_synth_shape_parses_and_applies();
  test_mixed_synth_and_sequencer_document_applies();
  test_deferred_synth_fields_fail_without_queueing_events();
  test_valid_synth_document_never_emits_apply_not_implemented();
}
