#include "TestHelpers.h"
#include "TestRunner.h"

#include "app/AppContext.h"
#include "app/AppParams.h"
#include "app/doc/DocAuthoredModel.h"
#include "app/doc/DocAuthoringService.h"
#include "app/doc/metadata/DocMetadata.h"

#include <algorithm>

namespace {
using test::getParseTestWorkspace;
using test::parseWorkspace;

const char* kNonEmptyTrack1 =
    "track(1, TrackSettings { patterns = { [1] = { numSteps = 1, stepsPerBeat = 4, "
    "steps = { { active = true, notes = { { note = 60, velocity = 100 } } } } } }, activeSlot = 1 })";

} // namespace

static void test_mixer_parser_sets_has_mixer_state_from_call() {
  TEST("mixer_parser_sets_has_mixer_state_from_call");
  auto* ws = getParseTestWorkspace();
  auto result = parseWorkspace(1, 1, "mixer(1, MixerSettings { gain = 0.8 })", ws);
  CHECK("ok", result.ok);
  CHECK("hasMixerState", ws->model.hasMixerState[0]);
}

static void test_mixer_parser_sets_has_mixer_state_from_track_settings() {
  TEST("mixer_parser_sets_has_mixer_state_from_track_settings");
  auto* ws = getParseTestWorkspace();
  auto result =
      parseWorkspace(1, 1, "track(1, TrackSettings { mixer = MixerSettings { pan = -0.5 } })", ws);
  CHECK("ok", result.ok);
  CHECK("hasMixerState", ws->model.hasMixerState[0]);
}

static void test_mixer_parser_gain_write() {
  TEST("mixer_parser_gain_write");
  auto* ws = getParseTestWorkspace();
  auto result = parseWorkspace(1, 1, "mixer(1, MixerSettings { gain = 0.75 })", ws);
  CHECK("ok", result.ok);
  const auto& patch = ws->model.mixerTracks[0];
  CHECK("has write", patch.writes.size() == 1);
  CHECK("paramID",
        patch.writes.size() > 0 && patch.writes[0].paramID == app::params::AppParamID::TrackGain);
  CHECK("value", patch.writes.size() > 0 && patch.writes[0].value == 0.75f);
}

static void test_mixer_parser_pan_write() {
  TEST("mixer_parser_pan_write");
  auto* ws = getParseTestWorkspace();
  auto result = parseWorkspace(1, 1, "mixer(1, MixerSettings { pan = -0.3 })", ws);
  CHECK("ok", result.ok);
  const auto& patch = ws->model.mixerTracks[0];
  CHECK("has write", patch.writes.size() == 1);
  CHECK("paramID",
        patch.writes.size() > 0 && patch.writes[0].paramID == app::params::AppParamID::TrackPan);
  CHECK("value", patch.writes.size() > 0 && std::abs(patch.writes[0].value - (-0.3f)) < 1e-5f);
}

static void test_mixer_parser_mute_true() {
  TEST("mixer_parser_mute_true");
  auto* ws = getParseTestWorkspace();
  auto result = parseWorkspace(1, 1, "mixer(1, MixerSettings { mute = true })", ws);
  CHECK("ok", result.ok);
  const auto& patch = ws->model.mixerTracks[0];
  CHECK("has write", patch.writes.size() == 1);
  CHECK("paramID",
        patch.writes.size() > 0 && patch.writes[0].paramID == app::params::AppParamID::TrackMute);
  CHECK("value", patch.writes.size() > 0 && patch.writes[0].value == 1.0f);
}

static void test_mixer_parser_mute_false() {
  TEST("mixer_parser_mute_false");
  auto* ws = getParseTestWorkspace();
  auto result = parseWorkspace(1, 1, "mixer(1, MixerSettings { mute = false })", ws);
  CHECK("ok", result.ok);
  CHECK("value",
        ws->model.mixerTracks[0].writes.size() > 0 &&
            ws->model.mixerTracks[0].writes[0].value == 0.0f);
}

static void test_mixer_parser_multiple_fields_in_one_call() {
  TEST("mixer_parser_multiple_fields_in_one_call");
  auto* ws = getParseTestWorkspace();
  auto result =
      parseWorkspace(1, 1, "mixer(1, MixerSettings { gain = 0.8, pan = -0.2, mute = false })", ws);
  CHECK("ok", result.ok);
  CHECK("write count", ws->model.mixerTracks[0].writes.size() == 3);
}

static void test_mixer_parser_different_tracks_independent() {
  TEST("mixer_parser_different_tracks_independent");
  auto* ws = getParseTestWorkspace();
  auto result = parseWorkspace(1,
                               1,
                               "mixer(1, MixerSettings { gain = 0.8 })\n"
                               "mixer(2, MixerSettings { mute = true })",
                               ws);
  CHECK("ok", result.ok);
  CHECK("track1", ws->model.hasMixerState[0]);
  CHECK("track2", ws->model.hasMixerState[1]);
}

static void test_mixer_parser_empty_mixer_settings_is_valid_no_op() {
  TEST("mixer_parser_empty_mixer_settings_is_valid_no_op");
  auto* ws = getParseTestWorkspace();
  auto result = parseWorkspace(1, 1, "mixer(1, MixerSettings {})", ws);
  CHECK("ok", result.ok);
  CHECK("hasMixerState", ws->model.hasMixerState[0]);
  CHECK("no writes", ws->model.mixerTracks[0].writes.empty());
}

static void test_mixer_parser_invalid_track_index() {
  TEST("mixer_parser_invalid_track_index");
  auto* ws = getParseTestWorkspace();
  auto result = parseWorkspace(1, 1, "mixer(99, MixerSettings { gain = 0.5 })", ws);
  CHECK("not ok", !result.ok);
  const bool hasDiag =
      std::any_of(result.diagnostics.begin(), result.diagnostics.end(), [](const auto& d) {
        return d.code == app::doc::docdiag::MixerTrackInvalidIndex;
      });
  CHECK("diag", hasDiag);
}

static void test_mixer_parser_settings_not_a_table() {
  TEST("mixer_parser_settings_not_a_table");
  auto* ws = getParseTestWorkspace();
  auto result = parseWorkspace(1, 1, "mixer(1, 42)", ws);
  CHECK("not ok", !result.ok);
  const bool hasDiag =
      std::any_of(result.diagnostics.begin(), result.diagnostics.end(), [](const auto& d) {
        return d.code == app::doc::docdiag::MixerSettingsInvalidShape;
      });
  CHECK("diag", hasDiag);
}

static void test_mixer_parser_unknown_field() {
  TEST("mixer_parser_unknown_field");
  auto* ws = getParseTestWorkspace();
  auto result = parseWorkspace(1, 1, "mixer(1, MixerSettings { masterGain = 0.9 })", ws);
  CHECK("not ok", !result.ok);
  const bool hasDiag =
      std::any_of(result.diagnostics.begin(), result.diagnostics.end(), [](const auto& d) {
        return d.code == app::doc::docdiag::MixerParamUnknown;
      });
  CHECK("diag", hasDiag);
}

static void test_mixer_parser_type_mismatch_gain_as_string() {
  TEST("mixer_parser_type_mismatch_gain_as_string");
  auto* ws = getParseTestWorkspace();
  auto result = parseWorkspace(1, 1, "mixer(1, MixerSettings { gain = 'loud' })", ws);
  CHECK("not ok", !result.ok);
  const bool hasDiag =
      std::any_of(result.diagnostics.begin(), result.diagnostics.end(), [](const auto& d) {
        return d.code == app::doc::docdiag::MixerParamTypeMismatch;
      });
  CHECK("diag", hasDiag);
}

static void test_mixer_parser_type_mismatch_mute_as_number() {
  TEST("mixer_parser_type_mismatch_mute_as_number");
  auto* ws = getParseTestWorkspace();
  auto result = parseWorkspace(1, 1, "mixer(1, MixerSettings { mute = 1 })", ws);
  CHECK("not ok", !result.ok);
  const bool hasDiag =
      std::any_of(result.diagnostics.begin(), result.diagnostics.end(), [](const auto& d) {
        return d.code == app::doc::docdiag::MixerParamTypeMismatch;
      });
  CHECK("diag", hasDiag);
}

static void test_mixer_parser_gain_out_of_range_high() {
  TEST("mixer_parser_gain_out_of_range_high");
  auto* ws = getParseTestWorkspace();
  auto result = parseWorkspace(1, 1, "mixer(1, MixerSettings { gain = 2.0 })", ws);
  CHECK("not ok", !result.ok);
  const bool hasDiag =
      std::any_of(result.diagnostics.begin(), result.diagnostics.end(), [](const auto& d) {
        return d.code == app::doc::docdiag::MixerParamOutOfRange;
      });
  CHECK("diag", hasDiag);
}

static void test_mixer_parser_pan_out_of_range() {
  TEST("mixer_parser_pan_out_of_range");
  auto* ws = getParseTestWorkspace();
  auto result = parseWorkspace(1, 1, "mixer(1, MixerSettings { pan = 2.0 })", ws);
  CHECK("not ok", !result.ok);
  const bool hasDiag =
      std::any_of(result.diagnostics.begin(), result.diagnostics.end(), [](const auto& d) {
        return d.code == app::doc::docdiag::MixerParamOutOfRange;
      });
  CHECK("diag", hasDiag);
}

static void test_mixer_parser_duplicate_write_same_value_is_ok() {
  TEST("mixer_parser_duplicate_write_same_value_is_ok");
  auto* ws = getParseTestWorkspace();
  auto result = parseWorkspace(1,
                               1,
                               "mixer(1, MixerSettings { gain = 0.8 })\n"
                               "mixer(1, MixerSettings { gain = 0.8 })",
                               ws);
  CHECK("ok", result.ok);
}

static void test_mixer_parser_duplicate_write_different_value_fails() {
  TEST("mixer_parser_duplicate_write_different_value_fails");
  auto* ws = getParseTestWorkspace();
  auto result = parseWorkspace(1,
                               1,
                               "mixer(1, MixerSettings { gain = 0.8 })\n"
                               "mixer(1, MixerSettings { gain = 0.5 })",
                               ws);
  CHECK("not ok", !result.ok);
  const bool hasDiag =
      std::any_of(result.diagnostics.begin(), result.diagnostics.end(), [](const auto& d) {
        return d.code == app::doc::docdiag::MixerParamDuplicateWrite;
      });
  CHECK("diag", hasDiag);
}

static void test_sequencer_only_document_still_applies_after_phase2() {
  TEST("sequencer_only_document_still_applies_after_phase2");
  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);

  auto result =
      app::doc::submitAuthoredDocRevision(app->documents.authoring, *app, 1, kNonEmptyTrack1);
  CHECK("ok", result.ok);
  test::destroyAppContext(app);
}

static void test_synth_only_document_still_applies_after_phase2() {
  TEST("synth_only_document_still_applies_after_phase2");
  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);

  auto result =
      app::doc::submitAuthoredDocRevision(app->documents.authoring,
                                          *app,
                                          1,
                                          "synth(1, SynthSettings { osc1 = { mixLevel = 0.5 } })");
  CHECK("ok", result.ok);
  test::destroyAppContext(app);
}

void runDocMixerSettingsParserTests() {
  SUITE("DocMixerSettingsParser");
  test_mixer_parser_sets_has_mixer_state_from_call();
  test_mixer_parser_sets_has_mixer_state_from_track_settings();
  test_mixer_parser_gain_write();
  test_mixer_parser_pan_write();
  test_mixer_parser_mute_true();
  test_mixer_parser_mute_false();
  test_mixer_parser_multiple_fields_in_one_call();
  test_mixer_parser_different_tracks_independent();
  test_mixer_parser_empty_mixer_settings_is_valid_no_op();
  test_mixer_parser_invalid_track_index();
  test_mixer_parser_settings_not_a_table();
  test_mixer_parser_unknown_field();
  test_mixer_parser_type_mismatch_gain_as_string();
  test_mixer_parser_type_mismatch_mute_as_number();
  test_mixer_parser_gain_out_of_range_high();
  test_mixer_parser_pan_out_of_range();
  test_mixer_parser_duplicate_write_same_value_is_ok();
  test_mixer_parser_duplicate_write_different_value_fails();
  test_sequencer_only_document_still_applies_after_phase2();
  test_synth_only_document_still_applies_after_phase2();
}
