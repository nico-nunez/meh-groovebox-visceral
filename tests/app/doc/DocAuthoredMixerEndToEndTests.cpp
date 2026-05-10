#include "TestRunner.h"

#include "app/AppContext.h"
#include "app/doc/DocAuthoredModel.h"
#include "app/doc/DocAuthoringService.h"
#include "app/doc/DocMetadata.h"

#include <algorithm>
#include <fstream>
#include <string>

namespace {
const char* kNonEmptyTrack1 =
    "track(1, TrackSettings { patterns = { [1] = { numSteps = 1, stepsPerBeat = 4, "
    "steps = { { active = true, note = 60, velocity = 100 } } } }, activeSlot = 1 })";
} // namespace

static void test_mixer_e2e_mixer_call_applies() {
  TEST("mixer_e2e_mixer_call_applies");
  app::doc::DocAuthoringService service{};
  app::doc::initDocAuthoringService(service);
  app::AppContext app{};

  auto result =
      app::doc::applySequencerRevision(service,
                                       app,
                                       1,
                                       "mixer(1, MixerSettings { gain = 0.6, pan = -0.1 })");
  CHECK("ok", result.ok);
  CHECK("no diags", result.diagnostics.empty());
}

static void test_mixer_e2e_track_settings_mixer_applies() {
  TEST("mixer_e2e_track_settings_mixer_applies");
  app::doc::DocAuthoringService service{};
  app::doc::initDocAuthoringService(service);
  app::AppContext app{};

  auto result = app::doc::applySequencerRevision(
      service,
      app,
      1,
      "track(1, TrackSettings { mixer = MixerSettings { gain = 0.9 } })");
  CHECK("ok", result.ok);
}

static void test_mixer_e2e_all_three_track_params() {
  TEST("mixer_e2e_all_three_track_params");
  app::doc::DocAuthoringService service{};
  app::doc::initDocAuthoringService(service);
  app::AppContext app{};

  auto result = app::doc::applySequencerRevision(
      service,
      app,
      1,
      "mixer(1, MixerSettings { gain = 0.8, pan = 0.2, mute = false })");
  CHECK("ok", result.ok);
}

static void test_mixer_e2e_invalid_deferred_field_rejected() {
  TEST("mixer_e2e_invalid_deferred_field_rejected");
  app::doc::DocAuthoringService service{};
  app::doc::initDocAuthoringService(service);
  app::AppContext app{};

  auto result = app::doc::applySequencerRevision(service,
                                                 app,
                                                 1,
                                                 "mixer(1, MixerSettings { masterGain = 0.9 })");
  CHECK("not ok", !result.ok);
  const bool hasDiag =
      std::any_of(result.diagnostics.begin(), result.diagnostics.end(), [](const auto& d) {
        return d.code == app::doc::docdiag::MixerParamUnknown;
      });
  CHECK("diag", hasDiag);
}

static void test_mixer_e2e_invalid_type_rejected() {
  TEST("mixer_e2e_invalid_type_rejected");
  app::doc::DocAuthoringService service{};
  app::doc::initDocAuthoringService(service);
  app::AppContext app{};

  auto result =
      app::doc::applySequencerRevision(service, app, 1, "mixer(1, MixerSettings { mute = 1 })");
  CHECK("not ok", !result.ok);
}

static void test_mixer_e2e_out_of_range_rejected() {
  TEST("mixer_e2e_out_of_range_rejected");
  app::doc::DocAuthoringService service{};
  app::doc::initDocAuthoringService(service);
  app::AppContext app{};

  auto result =
      app::doc::applySequencerRevision(service, app, 1, "mixer(1, MixerSettings { gain = 2.0 })");
  CHECK("not ok", !result.ok);
}

static void test_mixer_e2e_valid_mixed_doc_applies() {
  TEST("mixer_e2e_valid_mixed_doc_applies");
  app::doc::DocAuthoringService service{};
  app::doc::initDocAuthoringService(service);
  app::AppContext app{};

  std::string doc = "mixer(1, MixerSettings { gain = 0.8 })\n"
                    "synth(1, SynthSettings { osc1 = { mix = 0.5 } })\n";
  doc += kNonEmptyTrack1;
  auto result = app::doc::applySequencerRevision(service, app, 1, doc.c_str());
  CHECK("ok", result.ok);
}

static void test_mixer_e2e_admitted_model_records_multiple_tracks() {
  TEST("mixer_e2e_admitted_model_records_multiple_tracks");
  app::doc::DocAuthoringService service{};
  app::doc::initDocAuthoringService(service);
  app::AppContext app{};

  app::doc::applySequencerRevision(service,
                                   app,
                                   1,
                                   "mixer(1, MixerSettings { gain = 0.8 })\n"
                                   "mixer(2, MixerSettings { mute = true })\n"
                                   "mixer(3, MixerSettings { pan = -0.5 })");

  CHECK("track1", service.apply.lastAdmittedDocModel.hasMixerState[0]);
  CHECK("track2", service.apply.lastAdmittedDocModel.hasMixerState[1]);
  CHECK("track3", service.apply.lastAdmittedDocModel.hasMixerState[2]);
}

static void test_mixer_e2e_admitted_synth_state_preserved_across_mixer_sequencer_apply() {
  TEST("mixer_e2e_admitted_synth_state_preserved_across_mixer_sequencer_apply");
  app::doc::DocAuthoringService service{};
  app::doc::initDocAuthoringService(service);
  app::AppContext app{};

  app::doc::applySequencerRevision(service,
                                   app,
                                   1,
                                   "synth(1, SynthSettings { osc1 = { mix = 0.5 } })");

  std::string doc = "mixer(1, MixerSettings { gain = 0.7 })\n";
  doc += kNonEmptyTrack1;
  app::doc::applySequencerRevision(service, app, 2, doc.c_str());

  CHECK("synth preserved", service.apply.lastAdmittedDocModel.hasSynthState[0]);
  CHECK("mixer added", service.apply.lastAdmittedDocModel.hasMixerState[0]);
  CHECK("seq added", service.apply.lastAdmittedDocModel.sequencer.hasTrackState[0]);
}

static void test_valid_mixer_doc_lua_fixture_exists() {
  TEST("valid_mixer_doc_lua_fixture_exists");
  std::ifstream f("tests/luals/authored_document/valid_mixer_doc.lua");
  CHECK("exists", f.good());
}

void runDocAuthoredMixerEndToEndTests() {
  SUITE("DocAuthoredMixerEndToEnd");
  test_mixer_e2e_mixer_call_applies();
  test_mixer_e2e_track_settings_mixer_applies();
  test_mixer_e2e_all_three_track_params();
  test_mixer_e2e_invalid_deferred_field_rejected();
  test_mixer_e2e_invalid_type_rejected();
  test_mixer_e2e_out_of_range_rejected();
  test_mixer_e2e_valid_mixed_doc_applies();
  test_mixer_e2e_admitted_model_records_multiple_tracks();
  test_mixer_e2e_admitted_synth_state_preserved_across_mixer_sequencer_apply();
  test_valid_mixer_doc_lua_fixture_exists();
}
