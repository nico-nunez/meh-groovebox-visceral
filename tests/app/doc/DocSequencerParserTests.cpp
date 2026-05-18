#include "TestHelpers.h"
#include "TestRunner.h"

#include "app/Sequencer.h"
#include "app/doc/DocMetadata.h"
#include "app/doc/DocTypes.h"

#include <cstdio>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace {

using test::getParseTestWorkspace;
using test::hasDiagnostic;
using test::parseWS;

} // namespace

// ---------------------------------------------------------------------------
// Test cases
// ---------------------------------------------------------------------------

static void test_populated_patterns_with_explicit_active_slot() {
  TEST("populated_patterns_with_explicit_active_slot");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [1] = { numSteps = 1, stepsPerBeat = 4, "
                   "steps = { { active = true, note = 60, velocity = 100, gate = 0.8 } } } }, "
                   "activeSlot = 1 })",
                   ws);
  CHECK("ok", r.ok);
  CHECK("hasTrackState[0]", ws->model.sequencer.hasTrackState[0]);
  CHECK("patterns[0].occupied", ws->model.sequencer.tracks[0].patterns[0].occupied);
  CHECK("activeSlot == 0", ws->model.sequencer.tracks[0].activeSlot == 0);
  CHECK("activeSlotSource == Explicit",
        ws->model.sequencer.tracks[0].activeSlotSource ==
            app::doc::ActivePatternSlotSource::Explicit);
  CHECK("no diagnostics", r.diagnostics.empty());
}

static void test_omitted_patterns_is_explicit_empty() {
  TEST("omitted_patterns_is_explicit_empty");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings {})", ws);
  CHECK("ok", r.ok);
  CHECK("hasTrackState[0]", ws->model.sequencer.hasTrackState[0]);
  CHECK("explicitlyAuthoredEmpty", ws->model.sequencer.tracks[0].explicitlyAuthoredEmpty);
  CHECK("activeSlot == INVALID",
        ws->model.sequencer.tracks[0].activeSlot == app::sequencer::INVALID_PATTERN_SLOT);
  CHECK("no diagnostics", r.diagnostics.empty());
}

static void test_empty_patterns_table_is_explicit_empty() {
  TEST("empty_patterns_table_is_explicit_empty");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = {} })", ws);
  CHECK("ok", r.ok);
  CHECK("explicitlyAuthoredEmpty", ws->model.sequencer.tracks[0].explicitlyAuthoredEmpty);
  CHECK("activeSlot == INVALID",
        ws->model.sequencer.tracks[0].activeSlot == app::sequencer::INVALID_PATTERN_SLOT);
  CHECK("no diagnostics", r.diagnostics.empty());
}

static void test_invalid_patterns_shape_rejects_revision() {
  TEST("invalid_patterns_shape_rejects_revision");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = 123 })", ws);
  CHECK("not ok", !r.ok);
  CHECK("diagnostic sequencer.patterns.invalid_shape",
        hasDiagnostic(r.diagnostics, "sequencer.patterns.invalid_shape"));
  CHECK("explicitlyAuthoredEmpty == false", !ws->model.sequencer.tracks[0].explicitlyAuthoredEmpty);
}

static void test_omitted_active_slot_is_inferred() {
  TEST("omitted_active_slot_is_inferred");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [3] = { numSteps = 1, stepsPerBeat = 4, "
                   "steps = { { active = true } } } } })",
                   ws);
  CHECK("ok", r.ok);
  CHECK("activeSlot == 2", ws->model.sequencer.tracks[0].activeSlot == 2);
  CHECK("activeSlotSource == Inferred",
        ws->model.sequencer.tracks[0].activeSlotSource ==
            app::doc::ActivePatternSlotSource::Inferred);
}

static void test_active_slot_without_patterns_rejects_revision() {
  TEST("active_slot_without_patterns_rejects_revision");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { activeSlot = 1 })", ws);
  CHECK("not ok", !r.ok);
  CHECK("diagnostic sequencer.active_slot.missing_patterns",
        hasDiagnostic(r.diagnostics, "sequencer.active_slot.missing_patterns"));
}

static void test_active_slot_empty_slot_rejects_revision() {
  TEST("active_slot_empty_slot_rejects_revision");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [2] = { numSteps = 1, stepsPerBeat = 4, "
                   "steps = { { active = true } } } }, activeSlot = 1 })",
                   ws);
  CHECK("not ok", !r.ok);
  CHECK("diagnostic sequencer.active_slot.empty_slot",
        hasDiagnostic(r.diagnostics, "sequencer.active_slot.empty_slot"));
}

static void test_pattern_slot_key_must_be_integer() {
  TEST("pattern_slot_key_must_be_integer");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { bad = { numSteps = 1, stepsPerBeat = 4, "
                   "steps = { { active = true } } } } })",
                   ws);
  CHECK("not ok", !r.ok);
  CHECK("diagnostic sequencer.pattern.slot_invalid_key",
        hasDiagnostic(r.diagnostics, "sequencer.pattern.slot_invalid_key"));
}

static void test_pattern_slot_key_must_be_in_range() {
  TEST("pattern_slot_key_must_be_in_range");
  auto* ws = getParseTestWorkspace();
  auto r =
      parseWS("track(1, TrackSettings { patterns = { [999] = { numSteps = 1, stepsPerBeat = 4, "
              "steps = { { active = true } } } } })",
              ws);
  CHECK("not ok", !r.ok);
  CHECK("diagnostic sequencer.pattern.slot_out_of_range",
        hasDiagnostic(r.diagnostics, "sequencer.pattern.slot_out_of_range"));
}

static void test_short_steps_table_rejects_pattern() {
  TEST("short_steps_table_rejects_pattern");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [1] = { numSteps = 2, stepsPerBeat = 4, "
                   "steps = { { active = true } } } } })",
                   ws);
  CHECK("not ok", !r.ok);
  CHECK("diagnostic sequencer.pattern.invalid_shape",
        hasDiagnostic(r.diagnostics, "sequencer.pattern.invalid_shape"));
}

static void test_malformed_step_rejects_pattern() {
  TEST("malformed_step_rejects_pattern");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [1] = { numSteps = 1, stepsPerBeat = 4, "
                   "steps = { { active = 'yes' } } } } })",
                   ws);
  CHECK("not ok", !r.ok);
  CHECK("diagnostic sequencer.pattern.invalid_shape",
        hasDiagnostic(r.diagnostics, "sequencer.pattern.invalid_shape"));
}

static void test_track_index_must_be_integer() {
  TEST("track_index_must_be_integer");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track('one', TrackSettings {})", ws);
  CHECK("not ok", !r.ok);
  CHECK("diagnostic sequencer.track.invalid_index",
        hasDiagnostic(r.diagnostics, "sequencer.track.invalid_index"));
}

static void test_active_slot_must_be_integer() {
  TEST("active_slot_must_be_integer");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = {}, activeSlot = 'one' })", ws);
  CHECK("not ok", !r.ok);
  CHECK("diagnostic sequencer.active_slot.invalid_type",
        hasDiagnostic(r.diagnostics, "sequencer.active_slot.invalid_type"));
}

static void test_track_settings_must_be_table() {
  TEST("track_settings_must_be_table");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, 123)", ws);
  CHECK("not ok", !r.ok);
  CHECK("diagnostic sequencer.track.invalid_settings",
        hasDiagnostic(r.diagnostics, "sequencer.track.invalid_settings"));
}

static void test_duplicate_track_last_block_wins_but_diagnostics_remain() {
  TEST("duplicate_track_last_block_wins_but_diagnostics_remain");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = 123 }) "
                   "track(1, TrackSettings {})",
                   ws);
  CHECK("not ok", !r.ok);
  CHECK("diagnostic sequencer.patterns.invalid_shape",
        hasDiagnostic(r.diagnostics, "sequencer.patterns.invalid_shape"));
  CHECK("final explicitlyAuthoredEmpty == true",
        ws->model.sequencer.tracks[0].explicitlyAuthoredEmpty);
}

static void test_constructors_preserve_table_arguments() {
  TEST("constructors_preserve_table_arguments");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [1] = { numSteps = 1, stepsPerBeat = 4, "
                   "steps = { { active = true } } } } }) "
                   "SynthSettings { ignored = true } "
                   "MixerSettings { ignored = true }",
                   ws);
  CHECK("ok", r.ok);
  CHECK("patterns[0].occupied", ws->model.sequencer.tracks[0].patterns[0].occupied);
  CHECK("no diagnostics", r.diagnostics.empty());
}

static void test_metadata_authored_constructors_are_registered() {
  TEST("metadata_authored_constructors_are_registered");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("TrackSettings {} SynthSettings {} MixerSettings {}", ws);
  CHECK("ok", r.ok);
  CHECK("no diagnostics", r.diagnostics.empty());
}

static void test_metadata_document_track_global_is_registered() {
  TEST("metadata_document_track_global_is_registered");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings {})", ws);
  CHECK("ok", r.ok);
  CHECK("track 0 present", ws->model.sequencer.hasTrackState[0]);
}

static void test_runtime_apply_file_is_not_registered_in_authored_parser() {
  TEST("runtime_apply_file_is_not_registered_in_authored_parser");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("applyFile('song.lua')", ws);
  CHECK("not ok", !r.ok);
  CHECK("diagnostic document.lua_eval_failed",
        hasDiagnostic(r.diagnostics, app::doc::docdiag::DocumentLuaEvalFailed));
}

static void test_runtime_apply_file_snake_case_is_not_registered_in_authored_parser() {
  TEST("runtime_apply_file_snake_case_is_not_registered_in_authored_parser");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("apply_file('song.lua')", ws);
  CHECK("not ok", !r.ok);
  CHECK("diagnostic document.lua_eval_failed",
        hasDiagnostic(r.diagnostics, app::doc::docdiag::DocumentLuaEvalFailed));
}
// ---------------------------------------------------------------------------
// Entry point (called from tests/main.cpp)
// ---------------------------------------------------------------------------

void runDocSequencerParserTests() {
  SUITE("DocSequencerParser");
  test_populated_patterns_with_explicit_active_slot();
  test_omitted_patterns_is_explicit_empty();
  test_empty_patterns_table_is_explicit_empty();
  test_invalid_patterns_shape_rejects_revision();
  test_omitted_active_slot_is_inferred();
  test_active_slot_without_patterns_rejects_revision();
  test_active_slot_empty_slot_rejects_revision();
  test_pattern_slot_key_must_be_integer();
  test_pattern_slot_key_must_be_in_range();
  test_short_steps_table_rejects_pattern();
  test_malformed_step_rejects_pattern();
  test_track_index_must_be_integer();
  test_active_slot_must_be_integer();
  test_track_settings_must_be_table();
  test_duplicate_track_last_block_wins_but_diagnostics_remain();
  test_constructors_preserve_table_arguments();
  test_metadata_authored_constructors_are_registered();
  test_metadata_document_track_global_is_registered();
  test_runtime_apply_file_is_not_registered_in_authored_parser();
  test_runtime_apply_file_snake_case_is_not_registered_in_authored_parser();
}
