#include "TestHelpers.h"
#include "TestRunner.h"

#include "app/doc/DocTypes.h"
#include "app/doc/metadata/DocMetadata.h"

#include <cstdio>

namespace {

using test::getParseTestWorkspace;
using test::hasDiagnostic;
using test::parseWS;

} // namespace

static void test_populated_patterns_with_explicit_active_slot() {
  TEST("populated_patterns_with_explicit_active_slot");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS(
      "track(1, TrackSettings { patterns = { [1] = { numSteps = 1, stepsPerBeat = 4, "
      "steps = { { active = true, notes = { { note = 60, velocity = 100, gate = 0.8 } } } } } }, "
      "activeSlot = 1 })",
      ws);
  CHECK("ok", r.ok);
  CHECK("hasTrackState[0]", ws->model.sequencer.hasTrackState[0]);
  const auto& track = ws->model.sequencer.tracks[0];
  const auto& pattern = track.patternSlots[0].pattern;
  const auto& step = pattern.steps[0];
  CHECK("has sequencer patch", track.hasSequencerPatch);
  CHECK("slot patch present", track.hasPatternSlot[0]);
  CHECK("pattern op patch", track.patternSlots[0].op == app::PatchObjectOp::Patch);
  CHECK("has numSteps", pattern.hasNumSteps);
  CHECK("numSteps == 1", pattern.numSteps == 1);
  CHECK("has stepsPerBeat", pattern.hasStepsPerBeat);
  CHECK("stepsPerBeat == 4", pattern.stepsPerBeat == 4);
  CHECK("step patch present", pattern.hasStep[0]);
  CHECK("step active", step.hasActive && step.active);
  CHECK("step has note count", step.hasNoteCount);
  CHECK("step note count", step.noteCount == 1);
  CHECK("step note patch", step.hasNotePatch[0]);
  CHECK("step note", step.notes[0].hasNote && step.notes[0].note == 60);
  CHECK("step velocity", step.notes[0].hasVelocity && step.notes[0].velocity == 100);
  CHECK("step gate", step.notes[0].hasGate && step.notes[0].gate == 0.8f);
  CHECK("has activeSlot", track.hasActiveSlot);
  CHECK("activeSlot == 0", track.activeSlot == 0);
  CHECK("activeSlotSource == Explicit",
        track.activeSlotSource == app::doc::ActivePatternSlotSource::Explicit);
  CHECK("no diagnostics", r.diagnostics.empty());
}

static void test_empty_track_settings_is_not_sequencer_state() {
  TEST("empty_track_settings_is_not_sequencer_state");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings {})", ws);
  CHECK("ok", r.ok);
  CHECK("track sequencer absent", !ws->model.sequencer.hasTrackState[0]);
  CHECK("no sequencer patch", !ws->model.sequencer.tracks[0].hasSequencerPatch);
  CHECK("no diagnostics", r.diagnostics.empty());
}

static void test_patterns_false_clears_pattern_bank() {
  TEST("patterns_false_clears_pattern_bank");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = false })", ws);
  CHECK("ok", r.ok);
  CHECK("track sequencer present", ws->model.sequencer.hasTrackState[0]);
  CHECK("has sequencer patch", ws->model.sequencer.tracks[0].hasSequencerPatch);
  CHECK("bank clear", ws->model.sequencer.tracks[0].patternBankOp == app::PatchObjectOp::Clear);
  CHECK("no diagnostics", r.diagnostics.empty());
}

static void test_empty_patterns_table_has_no_patch_edits() {
  TEST("empty_patterns_table_has_no_patch_edits");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = {} })", ws);
  CHECK("ok", r.ok);
  CHECK("track sequencer absent", !ws->model.sequencer.hasTrackState[0]);
  CHECK("no sequencer patch", !ws->model.sequencer.tracks[0].hasSequencerPatch);
  CHECK("no slot patch", !ws->model.sequencer.tracks[0].hasPatternSlot[0]);
  CHECK("bank op is patch container",
        ws->model.sequencer.tracks[0].patternBankOp == app::PatchObjectOp::Patch);
  CHECK("no diagnostics", r.diagnostics.empty());
}

static void test_invalid_patterns_shape_rejects_revision() {
  TEST("invalid_patterns_shape_rejects_revision");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = 123 })", ws);
  CHECK("not ok", !r.ok);
  CHECK("diagnostic sequencer.patterns.invalid_shape",
        hasDiagnostic(r.diagnostics, "sequencer.patterns.invalid_shape"));
  CHECK("no sequencer patch", !ws->model.sequencer.tracks[0].hasSequencerPatch);
}

static void test_omitted_active_slot_is_not_inferred() {
  TEST("omitted_active_slot_is_not_inferred");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [3] = { numSteps = 1, stepsPerBeat = 4, "
                   "steps = { { active = true } } } } })",
                   ws);
  CHECK("ok", r.ok);
  CHECK("track present", ws->model.sequencer.hasTrackState[0]);
  CHECK("slot 3 present", ws->model.sequencer.tracks[0].hasPatternSlot[2]);
  CHECK("no activeSlot patch", !ws->model.sequencer.tracks[0].hasActiveSlot);
  CHECK("activeSlot == INVALID",
        ws->model.sequencer.tracks[0].activeSlot == app::sequencer::INVALID_PATTERN_SLOT);
  CHECK("activeSlotSource == Unset",
        ws->model.sequencer.tracks[0].activeSlotSource == app::doc::ActivePatternSlotSource::Unset);
}

static void test_active_slot_without_patterns_rejects_revision() {
  TEST("active_slot_without_patterns_rejects_revision");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { activeSlot = 1 })", ws);
  CHECK("not ok", !r.ok);
  CHECK("diagnostic sequencer.active_slot.missing_patterns",
        hasDiagnostic(r.diagnostics, "sequencer.active_slot.missing_patterns"));
}

static void test_active_slot_empty_slot_is_deferred_to_admission() {
  TEST("active_slot_empty_slot_is_deferred_to_admission");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [2] = { numSteps = 1, stepsPerBeat = 4, "
                   "steps = { { active = true } } } }, activeSlot = 1 })",
                   ws);
  CHECK("ok", r.ok);
  CHECK("slot 2 present", ws->model.sequencer.tracks[0].hasPatternSlot[1]);
  CHECK("activeSlot patch present", ws->model.sequencer.tracks[0].hasActiveSlot);
  CHECK("activeSlot == 0", ws->model.sequencer.tracks[0].activeSlot == 0);
  CHECK("no diagnostics", r.diagnostics.empty());
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

static void test_sparse_steps_table_allows_omitted_steps() {
  TEST("sparse_steps_table_allows_omitted_steps");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [1] = { numSteps = 2, stepsPerBeat = 4, "
                   "steps = { { active = true } } } } })",
                   ws);
  CHECK("ok", r.ok);
  const auto& pattern = ws->model.sequencer.tracks[0].patternSlots[0].pattern;
  CHECK("numSteps patch", pattern.hasNumSteps && pattern.numSteps == 2);
  CHECK("step 1 patch", pattern.hasStep[0]);
  CHECK("step 2 omitted", !pattern.hasStep[1]);
  CHECK("no diagnostics", r.diagnostics.empty());
}

static void test_sparse_step_one_note_array_sets_note_patch() {
  TEST("sparse_step_one_note_array_sets_note_patch");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { "
                   "notes = { { note = 64, velocity = 90, gate = 0.75 } } } } } } })",
                   ws);

  const auto& track = ws->model.sequencer.tracks[0];
  const auto& pattern = track.patternSlots[0].pattern;
  const auto& step = pattern.steps[0];

  CHECK("ok", r.ok);
  CHECK("track present", ws->model.sequencer.hasTrackState[0]);
  CHECK("slot present", track.hasPatternSlot[0]);
  CHECK("step present", pattern.hasStep[0]);
  CHECK("has note count", step.hasNoteCount);
  CHECK("note count", step.noteCount == 1);
  CHECK("has note patch", step.hasNotePatch[0]);
  CHECK("has note", step.notes[0].hasNote);
  CHECK("note value", step.notes[0].note == 64);
  CHECK("has velocity", step.notes[0].hasVelocity);
  CHECK("velocity value", step.notes[0].velocity == 90);
  CHECK("has gate", step.notes[0].hasGate);
  CHECK("gate value", step.notes[0].gate == 0.75f);
  CHECK("active omitted", !step.hasActive);
  CHECK("locks omitted", step.locks.op == app::PatchObjectOp::None);
  CHECK("no diagnostics", r.diagnostics.empty());
}

static void test_sparse_step_note_array_sets_tie_patch() {
  TEST("sparse_step_note_array_sets_tie_patch");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { "
                   "notes = { { note = 64, tie = true } } } } } } })",
                   ws);

  const auto& step = ws->model.sequencer.tracks[0].patternSlots[0].pattern.steps[0];

  CHECK("ok", r.ok);
  CHECK("has note count", step.hasNoteCount);
  CHECK("note count", step.noteCount == 1);
  CHECK("has note patch", step.hasNotePatch[0]);
  CHECK("has tie", step.notes[0].hasTie);
  CHECK("tie value", step.notes[0].tie);
  CHECK("no diagnostics", r.diagnostics.empty());
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
  CHECK("final track sequencer absent", !ws->model.sequencer.hasTrackState[0]);
  CHECK("final no sequencer patch", !ws->model.sequencer.tracks[0].hasSequencerPatch);
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
  CHECK("slot patch present", ws->model.sequencer.tracks[0].hasPatternSlot[0]);
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
  CHECK("track global callable", r.diagnostics.empty());
  CHECK("empty track has no sequencer state", !ws->model.sequencer.hasTrackState[0]);
}

static void test_pattern_slot_false_clears_slot() {
  TEST("pattern_slot_false_clears_slot");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [1] = false } })", ws);
  CHECK("ok", r.ok);
  CHECK("track present", ws->model.sequencer.hasTrackState[0]);
  CHECK("slot present", ws->model.sequencer.tracks[0].hasPatternSlot[0]);
  CHECK("slot clear",
        ws->model.sequencer.tracks[0].patternSlots[0].op == app::PatchObjectOp::Clear);
}

static void test_step_false_clears_step() {
  TEST("step_false_clears_step");
  auto* ws = getParseTestWorkspace();
  auto r =
      parseWS("track(1, TrackSettings { patterns = { [1] = { steps = { [1] = false } } } })", ws);
  CHECK("ok", r.ok);
  const auto& step = ws->model.sequencer.tracks[0].patternSlots[0].pattern.steps[0];
  CHECK("track present", ws->model.sequencer.hasTrackState[0]);
  CHECK("slot present", ws->model.sequencer.tracks[0].hasPatternSlot[0]);
  CHECK("step present", ws->model.sequencer.tracks[0].patternSlots[0].pattern.hasStep[0]);
  CHECK("step clear", step.op == app::PatchObjectOp::Clear);
}

static void test_empty_pattern_slot_table_has_no_slot_patch() {
  TEST("empty_pattern_slot_table_has_no_slot_patch");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [1] = {} } })", ws);
  CHECK("ok", r.ok);
  CHECK("track sequencer absent", !ws->model.sequencer.hasTrackState[0]);
  CHECK("slot omitted", !ws->model.sequencer.tracks[0].hasPatternSlot[0]);
  CHECK("no diagnostics", r.diagnostics.empty());
}

static void test_empty_step_table_has_no_step_patch() {
  TEST("empty_step_table_has_no_step_patch");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [1] = { steps = { [1] = {} } } } })", ws);
  CHECK("ok", r.ok);
  CHECK("track sequencer absent", !ws->model.sequencer.hasTrackState[0]);
  CHECK("slot omitted", !ws->model.sequencer.tracks[0].hasPatternSlot[0]);
  CHECK("no diagnostics", r.diagnostics.empty());
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

static void test_sparse_step_multi_note_array_sets_note_patches() {
  TEST("sparse_step_multi_note_array_sets_note_patches");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { "
                   "notes = { "
                   "{ note = 60, velocity = 100, gate = 1.0 }, "
                   "{ note = 64, velocity = 96, gate = 0.5 }, "
                   "{ note = 67, velocity = 92, gate = 1.5 } "
                   "} } } } } })",
                   ws);

  const auto& step = ws->model.sequencer.tracks[0].patternSlots[0].pattern.steps[0];

  CHECK("ok", r.ok);
  CHECK("has note count", step.hasNoteCount);
  CHECK("note count", step.noteCount == 3);
  CHECK("note 0 patch", step.hasNotePatch[0]);
  CHECK("note 1 patch", step.hasNotePatch[1]);
  CHECK("note 2 patch", step.hasNotePatch[2]);
  CHECK("note 0", step.notes[0].note == 60);
  CHECK("note 1", step.notes[1].note == 64);
  CHECK("note 2", step.notes[2].note == 67);
  CHECK("velocity 1", step.notes[1].velocity == 96);
  CHECK("gate 2", step.notes[2].gate == 1.5f);
  CHECK("no diagnostics", r.diagnostics.empty());
}

static void test_step_level_scalar_note_fields_reject_revision() {
  TEST("step_level_scalar_note_fields_reject_revision");

  const char* docs[] = {
      "track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { noteOn = true } } } } })",
      "track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { note = 60 } } } } })",
      "track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { velocity = 100 } } } } })",
      "track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { gate = 1.0 } } } } })",
      "track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { legato = true } } } } })",
      "track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { tie = true } } } } })",
  };

  for (const char* doc : docs) {
    auto* ws = getParseTestWorkspace();
    auto r = parseWS(doc, ws);
    CHECK("not ok", !r.ok);
    CHECK("diagnostic sequencer.pattern.invalid_shape",
          hasDiagnostic(r.diagnostics, app::doc::docdiag::SequencerPatternInvalidShape));
  }
}

static void test_step_notes_must_be_array() {
  TEST("step_notes_must_be_array");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { "
                   "notes = { note = 60, velocity = 100, gate = 1.0 } } } } } })",
                   ws);
  CHECK("not ok", !r.ok);
  CHECK("diagnostic sequencer.pattern.invalid_shape",
        hasDiagnostic(r.diagnostics, app::doc::docdiag::SequencerPatternInvalidShape));
}

static void test_step_notes_entry_must_be_table() {
  TEST("step_notes_entry_must_be_table");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { "
                   "notes = { 60 } } } } } })",
                   ws);
  CHECK("not ok", !r.ok);
  CHECK("diagnostic sequencer.pattern.invalid_shape",
        hasDiagnostic(r.diagnostics, app::doc::docdiag::SequencerPatternInvalidShape));
}

static void test_step_notes_invalid_values_reject_revision() {
  TEST("step_notes_invalid_values_reject_revision");

  const char* docs[] = {
      "track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { notes = { { note = 200 } } "
      "} } } } })",
      "track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { notes = { { velocity = 200 "
      "} } } } } } })",
      "track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { notes = { { gate = -1.0 } } "
      "} } } } })",
      "track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { notes = { { noteOn = 1 } } "
      "} } } } })",
      "track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { notes = { { tie = 1 } } "
      "} } } } })",
  };

  for (const char* doc : docs) {
    auto* ws = getParseTestWorkspace();
    auto r = parseWS(doc, ws);
    CHECK("not ok", !r.ok);
    CHECK("diagnostic sequencer.pattern.invalid_shape",
          hasDiagnostic(r.diagnostics, app::doc::docdiag::SequencerPatternInvalidShape));
  }
}

static void test_step_notes_capacity_rejects_revision() {
  TEST("step_notes_capacity_rejects_revision");
  auto* ws = getParseTestWorkspace();
  auto r = parseWS("track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { notes = { "
                   "{ note = 60 }, { note = 61 }, { note = 62 }, { note = 63 }, "
                   "{ note = 64 }, { note = 65 }, { note = 66 }, { note = 67 }, "
                   "{ note = 68 } "
                   "} } } } } })",
                   ws);
  CHECK("not ok", !r.ok);
  CHECK("diagnostic sequencer.pattern.invalid_shape",
        hasDiagnostic(r.diagnostics, app::doc::docdiag::SequencerPatternInvalidShape));
}

void runDocSequencerParserTests() {
  SUITE("DocSequencerParser");
  test_populated_patterns_with_explicit_active_slot();
  test_empty_track_settings_is_not_sequencer_state();
  test_patterns_false_clears_pattern_bank();
  test_empty_patterns_table_has_no_patch_edits();
  test_invalid_patterns_shape_rejects_revision();
  test_omitted_active_slot_is_not_inferred();
  test_active_slot_without_patterns_rejects_revision();
  test_active_slot_empty_slot_is_deferred_to_admission();
  test_pattern_slot_key_must_be_integer();
  test_pattern_slot_key_must_be_in_range();
  test_sparse_steps_table_allows_omitted_steps();
  test_sparse_step_one_note_array_sets_note_patch();
  test_sparse_step_note_array_sets_tie_patch();
  test_malformed_step_rejects_pattern();
  test_track_index_must_be_integer();
  test_active_slot_must_be_integer();
  test_track_settings_must_be_table();
  test_duplicate_track_last_block_wins_but_diagnostics_remain();
  test_constructors_preserve_table_arguments();
  test_metadata_authored_constructors_are_registered();
  test_metadata_document_track_global_is_registered();
  test_pattern_slot_false_clears_slot();
  test_step_false_clears_step();
  test_empty_pattern_slot_table_has_no_slot_patch();
  test_empty_step_table_has_no_step_patch();
  test_runtime_apply_file_is_not_registered_in_authored_parser();
  test_runtime_apply_file_snake_case_is_not_registered_in_authored_parser();
  test_sparse_step_multi_note_array_sets_note_patches();
  test_step_level_scalar_note_fields_reject_revision();
  test_step_notes_must_be_array();
  test_step_notes_entry_must_be_table();
  test_step_notes_invalid_values_reject_revision();
  test_step_notes_capacity_rejects_revision();
}
