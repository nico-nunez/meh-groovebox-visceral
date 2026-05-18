#include "TestHelpers.h"
#include "TestRunner.h"

#include "app/doc/DocMetadata.h"
#include "synth/params/ParamDefs.h"
#include "synth/params/ParamUtils.h"

#include <string>

namespace {

using test::getParseTestWorkspace;
using test::hasDiagnostic;
using test::parseWS;

const app::doc::AuthoredSynthParamWrite* findWrite(const app::doc::AuthoredTrackSynthPatch& patch,
                                                   synth::param::ParamID id) {
  for (const auto& write : patch.writes) {
    if (write.paramID == id)
      return &write;
  }
  return nullptr;
}

float enumValue(synth::param::ParamType type, const char* token) {
  auto parsed = synth::param::utils::parseEnum(type, token);
  CHECK((std::string("parse enum ") + token).c_str(), parsed.ok);
  return static_cast<float>(parsed.value);
}

} // namespace

static void test_top_level_synth_parses_patch_writes() {
  TEST("top_level_synth_parses_patch_writes");

  auto* ws = getParseTestWorkspace();
  auto r = parseWS("synth(1, SynthSettings { osc1 = { bank = 'saw', mix = 0.8 }, "
                   "svf = { cutoff = 1200, enabled = true } })",
                   ws);

  CHECK("ok", r.ok);
  CHECK("track 0 synth present", ws->model.hasSynthState[0]);
  const auto& patch = ws->model.synthTracks[0];
  CHECK("has patch", patch.hasPatch);
  CHECK("track index", patch.trackIndex == 0);
  CHECK("write count", patch.writes.size() == 4);

  const auto* bank = findWrite(patch, synth::param::OSC1_BANK_ID);
  const auto* mix = findWrite(patch, synth::param::OSC1_MIX_LEVEL);
  const auto* cutoff = findWrite(patch, synth::param::SVF_CUTOFF);
  const auto* enabled = findWrite(patch, synth::param::SVF_ENABLED);

  CHECK("bank write", bank != nullptr);
  CHECK("bank enum value",
        bank && bank->value == enumValue(synth::param::ParamType::OscBankID, "saw"));
  CHECK("mix write", mix && mix->value == 0.8f);
  CHECK("cutoff write", cutoff && cutoff->value == 1200.0f);
  CHECK("enabled write", enabled && enabled->value == 1.0f);
}

static void test_track_settings_synth_parses_patch_writes() {
  TEST("track_settings_synth_parses_patch_writes");

  auto* ws = getParseTestWorkspace();
  auto r = parseWS("local t = TrackSettings() "
                   "t.synth = SynthSettings { ampEnv = { attack = 5, release = 120 }, "
                   "unison = { voices = 3 } } "
                   "track(2, t)",
                   ws);

  CHECK("ok", r.ok);
  CHECK("track 1 synth present", ws->model.hasSynthState[1]);
  const auto& patch = ws->model.synthTracks[1];
  CHECK("attack", findWrite(patch, synth::param::AMP_ENV_ATTACK) != nullptr);
  CHECK("release", findWrite(patch, synth::param::AMP_ENV_RELEASE) != nullptr);
  const auto* voices = findWrite(patch, synth::param::UNISON_VOICES);
  CHECK("voices", voices && voices->value == 3.0f);
}

static void test_synth_and_sequencer_can_parse_together() {
  TEST("synth_and_sequencer_can_parse_together");

  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { "
                   "synth = SynthSettings { master = { gain = 0.7 } }, "
                   "patterns = { [1] = { numSteps = 1, stepsPerBeat = 4, "
                   "steps = { { active = true, note = 60 } } } }, activeSlot = 1 })",
                   ws);

  CHECK("ok", r.ok);
  CHECK("seq track present", ws->model.sequencer.hasTrackState[0]);
  CHECK("seq pattern occupied", ws->model.sequencer.tracks[0].patterns[0].occupied);
  CHECK("synth present", ws->model.hasSynthState[0]);
  CHECK("master gain", findWrite(ws->model.synthTracks[0], synth::param::MASTER_GAIN) != nullptr);
}

static void test_empty_synth_settings_is_valid_noop_patch() {
  TEST("empty_synth_settings_is_valid_noop_patch");

  auto* ws = getParseTestWorkspace();
  auto r = parseWS("synth(1, SynthSettings {})", ws);

  CHECK("ok", r.ok);
  CHECK("has synth state", ws->model.hasSynthState[0]);
  CHECK("no writes", ws->model.synthTracks[0].writes.empty());
}

static void test_synth_track_index_must_be_integer() {
  TEST("synth_track_index_must_be_integer");

  auto* ws = getParseTestWorkspace();
  auto r = parseWS("synth('one', SynthSettings {})", ws);

  CHECK("not ok", !r.ok);
  CHECK("diagnostic", hasDiagnostic(r.diagnostics, app::doc::docdiag::SynthTrackInvalidIndex));
}

static void test_synth_track_index_must_be_in_range() {
  TEST("synth_track_index_must_be_in_range");

  auto* ws = getParseTestWorkspace();
  auto r = parseWS("synth(999, SynthSettings {})", ws);

  CHECK("not ok", !r.ok);
  CHECK("diagnostic", hasDiagnostic(r.diagnostics, app::doc::docdiag::SynthTrackInvalidIndex));
}

static void test_synth_settings_must_be_table() {
  TEST("synth_settings_must_be_table");

  auto* ws = getParseTestWorkspace();
  auto r = parseWS("synth(1, 123)", ws);

  CHECK("not ok", !r.ok);
  CHECK("diagnostic", hasDiagnostic(r.diagnostics, app::doc::docdiag::SynthSettingsInvalidShape));
}

static void test_track_settings_synth_must_be_table() {
  TEST("track_settings_synth_must_be_table");

  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { synth = 123 })", ws);

  CHECK("not ok", !r.ok);
  CHECK("diagnostic", hasDiagnostic(r.diagnostics, app::doc::docdiag::SynthSettingsInvalidShape));
}

static void test_unknown_synth_field_is_diagnostic() {
  TEST("unknown_synth_field_is_diagnostic");

  auto* ws = getParseTestWorkspace();
  auto r = parseWS("synth(1, SynthSettings { osc1 = { nope = 1 } })", ws);

  CHECK("not ok", !r.ok);
  CHECK("diagnostic", hasDiagnostic(r.diagnostics, app::doc::docdiag::SynthParamUnknown));
}

static void test_deferred_synth_field_is_diagnostic() {
  TEST("deferred_synth_field_is_diagnostic");

  auto* ws = getParseTestWorkspace();
  auto r = parseWS("synth(1, SynthSettings { lfo1 = { rate = 2 } })", ws);

  CHECK("not ok", !r.ok);
  CHECK("diagnostic", hasDiagnostic(r.diagnostics, app::doc::docdiag::SynthParamUnknown));
}

static void test_bool_param_rejects_number() {
  TEST("bool_param_rejects_number");

  auto* ws = getParseTestWorkspace();
  auto r = parseWS("synth(1, SynthSettings { osc1 = { enabled = 1 } })", ws);

  CHECK("not ok", !r.ok);
  CHECK("diagnostic", hasDiagnostic(r.diagnostics, app::doc::docdiag::SynthParamTypeMismatch));
}

static void test_integer_param_rejects_float() {
  TEST("integer_param_rejects_float");

  auto* ws = getParseTestWorkspace();
  auto r = parseWS("synth(1, SynthSettings { unison = { voices = 3.5 } })", ws);

  CHECK("not ok", !r.ok);
  CHECK("diagnostic", hasDiagnostic(r.diagnostics, app::doc::docdiag::SynthParamTypeMismatch));
}

static void test_enum_param_rejects_unknown_string() {
  TEST("enum_param_rejects_unknown_string");

  auto* ws = getParseTestWorkspace();
  auto r = parseWS("synth(1, SynthSettings { osc1 = { bank = 'not_a_bank' } })", ws);

  CHECK("not ok", !r.ok);
  CHECK("diagnostic", hasDiagnostic(r.diagnostics, app::doc::docdiag::SynthParamEnumUnknown));
}

static void test_number_param_rejects_out_of_range() {
  TEST("number_param_rejects_out_of_range");

  auto* ws = getParseTestWorkspace();
  auto r = parseWS("synth(1, SynthSettings { svf = { cutoff = 999999 } })", ws);

  CHECK("not ok", !r.ok);
  CHECK("diagnostic", hasDiagnostic(r.diagnostics, app::doc::docdiag::SynthParamOutOfRange));
}

static void test_duplicate_identical_write_is_allowed_once() {
  TEST("duplicate_identical_write_is_allowed_once");

  auto* ws = getParseTestWorkspace();
  auto r = parseWS("synth(1, SynthSettings { osc1 = { mix = 0.5 } }) "
                   "local t = TrackSettings() "
                   "t.synth = SynthSettings { osc1 = { mix = 0.5 } } "
                   "track(1, t)",
                   ws);

  CHECK("ok", r.ok);
  CHECK("one write", ws->model.synthTracks[0].writes.size() == 1);
}

static void test_duplicate_conflicting_write_is_diagnostic() {
  TEST("duplicate_conflicting_write_is_diagnostic");

  auto* ws = getParseTestWorkspace();
  auto r = parseWS("synth(1, SynthSettings { osc1 = { mix = 0.5 } }) "
                   "local t = TrackSettings() "
                   "t.synth = SynthSettings { osc1 = { mix = 0.8 } } "
                   "track(1, t)",
                   ws);

  CHECK("not ok", !r.ok);
  CHECK("diagnostic", hasDiagnostic(r.diagnostics, app::doc::docdiag::SynthParamDuplicateWrite));
}

static void test_existing_sequencer_parser_api_still_returns_seq_model() {
  TEST("existing_sequencer_parser_api_still_returns_seq_model");

  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [1] = { numSteps = 1, stepsPerBeat = 4, "
                   "steps = { { active = true } } } }, activeSlot = 1 })",
                   ws);

  CHECK("ok", r.ok);
  CHECK("seq track present", ws->model.sequencer.hasTrackState[0]);
  CHECK("seq pattern occupied", ws->model.sequencer.tracks[0].patterns[0].occupied);
}

void runDocSynthSettingsParserTests() {
  SUITE("DocSynthSettingsParser");
  test_top_level_synth_parses_patch_writes();
  test_track_settings_synth_parses_patch_writes();
  test_synth_and_sequencer_can_parse_together();
  test_empty_synth_settings_is_valid_noop_patch();
  test_synth_track_index_must_be_integer();
  test_synth_track_index_must_be_in_range();
  test_synth_settings_must_be_table();
  test_track_settings_synth_must_be_table();
  test_unknown_synth_field_is_diagnostic();
  test_deferred_synth_field_is_diagnostic();
  test_bool_param_rejects_number();
  test_integer_param_rejects_float();
  test_enum_param_rejects_unknown_string();
  test_number_param_rejects_out_of_range();
  test_duplicate_identical_write_is_allowed_once();
  test_duplicate_conflicting_write_is_diagnostic();
  test_existing_sequencer_parser_api_still_returns_seq_model();
}
