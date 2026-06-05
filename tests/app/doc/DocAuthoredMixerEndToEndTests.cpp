#include "TestHelpers.h"
#include "TestRunner.h"

#include "app/AppContext.h"
#include "app/GrooveboxEditSession.h"
#include "app/Sequencer.h"
#include "app/doc/DocAuthoredModel.h"
#include "app/doc/DocAuthoringService.h"
#include "app/doc/metadata/DocMetadata.h"
#include "app/sessions/AudioSession.h"
#include "synth/params/ParamDefs.h"

#include <algorithm>
#include <fstream>
#include <string>

namespace {
const char* kNonEmptyTrack1 =
    "track(1, TrackSettings { patterns = { [1] = { numSteps = 1, stepsPerBeat = 4, "
    "steps = { { active = true, notes = { { note = 60, velocity = 100 } } } } } }, activeSlot = 1 "
    "})";

} // namespace

static void test_mixer_e2e_mixer_call_applies() {
  TEST("mixer_e2e_mixer_call_applies");
  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);

  auto result =
      app::doc::submitAuthoredDocRevision(app->documents.authoring,
                                          *app,
                                          1,
                                          "mixer(1, MixerSettings { gain = 0.6, pan = -0.1 })");
  CHECK("ok", result.ok);
  CHECK("no diags", result.diagnostics.empty());
  test::publishPending(app);
  CHECK("gain published", app->mixer.current.tracks[0].gain == 0.6f);
  CHECK("pan published", app->mixer.current.tracks[0].pan == -0.1f);
  test::destroyAppContext(app);
}

static void test_mixer_e2e_track_settings_mixer_applies() {
  TEST("mixer_e2e_track_settings_mixer_applies");
  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);

  auto result = app::doc::submitAuthoredDocRevision(
      app->documents.authoring,
      *app,
      1,
      "track(1, TrackSettings { mixer = MixerSettings { gain = 0.9 } })");
  CHECK("ok", result.ok);
  test::publishPending(app);
  CHECK("gain published", app->mixer.current.tracks[0].gain == 0.9f);
  test::destroyAppContext(app);
}

static void test_mixer_e2e_all_three_track_params() {
  TEST("mixer_e2e_all_three_track_params");
  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);

  auto result = app::doc::submitAuthoredDocRevision(
      app->documents.authoring,
      *app,
      1,
      "mixer(1, MixerSettings { gain = 0.8, pan = 0.2, mute = false })");
  CHECK("ok", result.ok);
  test::publishPending(app);
  CHECK("gain published", app->mixer.current.tracks[0].gain == 0.8f);
  CHECK("pan published", app->mixer.current.tracks[0].pan == 0.2f);
  CHECK("mute published", app->mixer.current.tracks[0].enabled);
  test::destroyAppContext(app);
}

static void test_mixer_e2e_invalid_deferred_field_rejected() {
  TEST("mixer_e2e_invalid_deferred_field_rejected");
  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);

  auto result = app::doc::submitAuthoredDocRevision(app->documents.authoring,
                                                    *app,
                                                    1,
                                                    "mixer(1, MixerSettings { masterGain = 0.9 })");
  CHECK("not ok", !result.ok);
  const bool hasDiag =
      std::any_of(result.diagnostics.begin(), result.diagnostics.end(), [](const auto& d) {
        return d.code == app::doc::docdiag::MixerParamUnknown;
      });
  CHECK("diag", hasDiag);
  test::destroyAppContext(app);
}

static void test_mixer_e2e_invalid_type_rejected() {
  TEST("mixer_e2e_invalid_type_rejected");
  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);

  auto result = app::doc::submitAuthoredDocRevision(app->documents.authoring,
                                                    *app,
                                                    1,
                                                    "mixer(1, MixerSettings { mute = 1 })");
  CHECK("not ok", !result.ok);
  test::destroyAppContext(app);
}

static void test_mixer_e2e_out_of_range_rejected() {
  TEST("mixer_e2e_out_of_range_rejected");
  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);

  auto result = app::doc::submitAuthoredDocRevision(app->documents.authoring,
                                                    *app,
                                                    1,
                                                    "mixer(1, MixerSettings { gain = 2.0 })");
  CHECK("not ok", !result.ok);
  test::destroyAppContext(app);
}

static void test_mixer_e2e_valid_mixed_doc_applies() {
  TEST("mixer_e2e_valid_mixed_doc_applies");
  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);

  std::string doc = "mixer(1, MixerSettings { gain = 0.8 })\n"
                    "synth(1, SynthSettings { osc1 = { mixLevel = 0.5 } })\n";
  doc += kNonEmptyTrack1;
  auto result = app::doc::submitAuthoredDocRevision(app->documents.authoring, *app, 1, doc.c_str());
  CHECK("ok", result.ok);
  test::publishPending(app);
  CHECK("mixer published", app->mixer.current.tracks[0].gain == 0.8f);
  CHECK("synth published", app->tracks[0].engine.params[synth::param::OSC1_MIX_LEVEL] == 0.5f);
  test::destroyAppContext(app);
}

static void test_mixer_e2e_admitted_model_records_multiple_tracks() {
  TEST("mixer_e2e_admitted_model_records_multiple_tracks");
  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);

  app::doc::submitAuthoredDocRevision(app->documents.authoring,
                                      *app,
                                      1,
                                      "mixer(1, MixerSettings { gain = 0.8 })\n"
                                      "mixer(2, MixerSettings { mute = true })\n"
                                      "mixer(3, MixerSettings { pan = -0.5 })");

  CHECK("track1", app->documents.authoring.apply.lastAdmittedDocModel.hasMixerState[0]);
  CHECK("track2", app->documents.authoring.apply.lastAdmittedDocModel.hasMixerState[1]);
  CHECK("track3", app->documents.authoring.apply.lastAdmittedDocModel.hasMixerState[2]);
  test::destroyAppContext(app);
}

static void test_mixer_e2e_mixer_sequencer_publish_together() {
  TEST("mixer_e2e_mixer_sequencer_publish_together");
  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);

  std::string doc = "mixer(1, MixerSettings { gain = 0.7 })\n";
  doc += kNonEmptyTrack1;
  auto result = app::doc::submitAuthoredDocRevision(app->documents.authoring, *app, 1, doc.c_str());

  CHECK("ok", result.ok);
  CHECK("mixer old", app->mixer.current.tracks[0].gain == 1.0f);
  auto pre = app::sequencer::getPatternBank(app->sequencer, 0);
  CHECK("seq old", pre.ok && !pre.value->slots[0].occupied);

  test::publishPending(app);

  CHECK("mixer published", app->mixer.current.tracks[0].gain == 0.7f);
  auto post = app::sequencer::getPatternBank(app->sequencer, 0);
  CHECK("seq published", post.ok && post.value->slots[0].occupied);
  test::destroyAppContext(app);
}

static void test_gain_patch_preserves_live_pan() {
  TEST("gain_patch_preserves_live_pan");
  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);

  app->mixer.current.tracks[0].pan = -0.5f;

  auto result = app::doc::submitAuthoredDocRevision(app->documents.authoring,
                                                    *app,
                                                    1,
                                                    "mixer(1, MixerSettings { gain = 0.7 })");
  CHECK("ok", result.ok);
  test::publishPending(app);

  CHECK("gain changed", app->mixer.current.tracks[0].gain == 0.7f);
  CHECK("pan preserved", app->mixer.current.tracks[0].pan == -0.5f);
  test::destroyAppContext(app);
}

static void test_mixer_patch_preserves_sequencer() {
  TEST("mixer_patch_preserves_sequencer");
  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);

  auto seed =
      app::doc::submitAuthoredDocRevision(app->documents.authoring, *app, 1, kNonEmptyTrack1);
  CHECK("seed ok", seed.ok);
  test::publishPending(app);

  auto mixer = app::doc::submitAuthoredDocRevision(app->documents.authoring,
                                                   *app,
                                                   2,
                                                   "mixer(1, MixerSettings { gain = 0.7 })");
  CHECK("mixer ok", mixer.ok);
  test::publishPending(app);

  auto bank = app::sequencer::getPatternBank(app->sequencer, 0);
  CHECK("slot preserved", bank.ok && bank.value->slots[0].occupied);
  CHECK("note preserved", bank.value->slots[0].pattern.steps[0].notes[0].note == 60);
  test::destroyAppContext(app);
}

static void test_sequencer_patch_preserves_mixer() {
  TEST("sequencer_patch_preserves_mixer");
  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);

  app->mixer.current.tracks[0].gain = 0.25f;

  auto result =
      app::doc::submitAuthoredDocRevision(app->documents.authoring,
                                          *app,
                                          1,
                                          "track(1, TrackSettings { patterns = { [1] = { steps = { "
                                          "[1] = { notes = { { note = 64 } } } } } } })");
  CHECK("apply ok", result.ok);
  test::publishPending(app);

  CHECK("mixer preserved", app->mixer.current.tracks[0].gain == 0.25f);
  test::destroyAppContext(app);
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
  test_mixer_e2e_mixer_sequencer_publish_together();
  test_gain_patch_preserves_live_pan();
  test_mixer_patch_preserves_sequencer();
  test_sequencer_patch_preserves_mixer();
  test_valid_mixer_doc_lua_fixture_exists();
}
