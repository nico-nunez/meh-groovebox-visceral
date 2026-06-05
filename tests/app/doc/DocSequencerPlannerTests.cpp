#include "TestRunner.h"

#include "TestHelpers.h"

#include "app/GrooveboxPatch.h"
#include "app/doc/DocSequencerPlanner.h"

namespace {
using app::doc::AuthoredSeqDocModel;
using app::doc::AuthoredTrackSeqModel;
using test::getParseTestWorkspace;
using test::parseWS;

void oneStepPatternPatch(uint8_t note, app::doc::AuthoredPatternPatch* pattern) {
  pattern->op = app::PatchObjectOp::Patch;
  pattern->hasNumSteps = true;
  pattern->numSteps = 1;
  pattern->hasStepsPerBeat = true;
  pattern->stepsPerBeat = 4;
  pattern->hasStep[0] = true;
  pattern->steps[0].op = app::PatchObjectOp::Patch;
  pattern->steps[0].hasActive = true;
  pattern->steps[0].active = true;
  pattern->steps[0].hasNoteCount = true;
  pattern->steps[0].noteCount = 1;
  pattern->steps[0].hasNotePatch[0] = true;
  pattern->steps[0].notes[0].hasNoteOn = true;
  pattern->steps[0].notes[0].noteOn = true;
  pattern->steps[0].notes[0].hasNote = true;
  pattern->steps[0].notes[0].note = note;
  pattern->steps[0].notes[0].hasVelocity = true;
  pattern->steps[0].notes[0].velocity = 100;
}

void trackWithSlotPatch(uint8_t trackIndex,
                        uint8_t slot,
                        uint8_t note,
                        AuthoredTrackSeqModel* track) {
  *track = AuthoredTrackSeqModel{};
  track->trackIndex = trackIndex;
  track->hasSequencerPatch = true;
  track->patternBankOp = app::PatchObjectOp::Patch;
  track->hasPatternSlot[slot] = true;
  track->patternSlots[slot].op = app::PatchObjectOp::Patch;
  oneStepPatternPatch(note, &track->patternSlots[slot].pattern);
  track->hasActiveSlot = true;
  track->activeSlot = slot;
  track->activeSlotSource = app::doc::ActivePatternSlotSource::Explicit;
}

} // namespace

static void test_build_sequencer_patch_omits_absent_state() {
  TEST("build_sequencer_patch_omits_absent_state");

  AuthoredSeqDocModel model{};
  app::SequencerPatch patch{};
  auto result = app::doc::buildSequencerPatch(&model, 1, 7, &patch);

  CHECK("target ok", result.ok);
  CHECK("lane 1 omitted", !patch.hasTrack[0]);
  CHECK("lane 1 activeSlot not patched", !patch.tracks[0].hasActiveSlot);
  CHECK("lane 1 slot omitted", !patch.tracks[0].hasSlot[0]);
}

static void test_build_sequencer_patch_copies_authored_slot_patch() {
  TEST("build_sequencer_patch_copies_authored_slot_patch");

  AuthoredSeqDocModel model{};
  model.hasTrackState[0] = true;
  trackWithSlotPatch(0, 0, 61, &model.tracks[0]);

  app::SequencerPatch patch{};
  auto result = app::doc::buildSequencerPatch(&model, 1, 7, &patch);

  CHECK("target ok", result.ok);
  CHECK("track patched", patch.hasTrack[0]);
  CHECK("bank patch", patch.tracks[0].bankOp == app::PatchObjectOp::Patch);
  CHECK("active slot patched", patch.tracks[0].hasActiveSlot);
  CHECK("active slot", patch.tracks[0].activeSlot == 0);
  CHECK("slot patched", patch.tracks[0].hasSlot[0]);
  CHECK("slot op", patch.tracks[0].slots[0].op == app::PatchObjectOp::Patch);
  CHECK("pattern op", patch.tracks[0].slots[0].pattern.op == app::PatchObjectOp::Patch);
  CHECK("num steps copied",
        patch.tracks[0].slots[0].pattern.hasNumSteps &&
            patch.tracks[0].slots[0].pattern.numSteps == 1);
  CHECK("steps per beat copied",
        patch.tracks[0].slots[0].pattern.hasStepsPerBeat &&
            patch.tracks[0].slots[0].pattern.stepsPerBeat == 4);
  CHECK("step copied", patch.tracks[0].slots[0].pattern.hasStep[0]);
  CHECK("note count copied",
        patch.tracks[0].slots[0].pattern.steps[0].hasNoteCount &&
            patch.tracks[0].slots[0].pattern.steps[0].noteCount == 1);
  CHECK("note patch copied", patch.tracks[0].slots[0].pattern.steps[0].hasNotePatch[0]);
  CHECK("note copied",
        patch.tracks[0].slots[0].pattern.steps[0].notes[0].hasNote &&
            patch.tracks[0].slots[0].pattern.steps[0].notes[0].note == 61);
  CHECK("velocity copied",
        patch.tracks[0].slots[0].pattern.steps[0].notes[0].hasVelocity &&
            patch.tracks[0].slots[0].pattern.steps[0].notes[0].velocity == 100);
}

static void test_build_sequencer_patch_preserves_sparse_tracks() {
  TEST("build_sequencer_patch_preserves_sparse_tracks");

  AuthoredSeqDocModel model{};
  model.hasTrackState[1] = true;
  trackWithSlotPatch(1, 0, 64, &model.tracks[1]);

  app::SequencerPatch patch{};
  auto result = app::doc::buildSequencerPatch(&model, 1, 7, &patch);

  CHECK("target ok", result.ok);
  CHECK("lane 2 patched", patch.hasTrack[1]);
  CHECK("lane 2 note", patch.tracks[1].slots[0].pattern.steps[0].notes[0].note == 64);
  CHECK("lane 1 omitted", !patch.hasTrack[0]);
  CHECK("lane 1 default inactive",
        patch.tracks[0].activeSlot == app::sequencer::INVALID_PATTERN_SLOT);
  CHECK("lane 1 slot omitted", !patch.tracks[0].hasSlot[0]);
}

static void test_build_sequencer_patch_copies_clear_ops() {
  TEST("build_sequencer_patch_copies_clear_ops");

  AuthoredSeqDocModel model{};
  model.hasTrackState[0] = true;
  model.tracks[0].trackIndex = 0;
  model.tracks[0].hasSequencerPatch = true;
  model.tracks[0].patternBankOp = app::PatchObjectOp::Clear;
  model.tracks[0].hasPatternSlot[1] = true;
  model.tracks[0].patternSlots[1].op = app::PatchObjectOp::Clear;
  model.tracks[0].patternSlots[1].pattern.hasStep[0] = true;
  model.tracks[0].patternSlots[1].pattern.steps[0].op = app::PatchObjectOp::Clear;

  app::SequencerPatch patch{};
  auto result = app::doc::buildSequencerPatch(&model, 1, 7, &patch);

  CHECK("target ok", result.ok);
  CHECK("track patched", patch.hasTrack[0]);
  CHECK("bank clear", patch.tracks[0].bankOp == app::PatchObjectOp::Clear);
  CHECK("slot clear",
        patch.tracks[0].hasSlot[1] && patch.tracks[0].slots[1].op == app::PatchObjectOp::Clear);
  CHECK("step clear",
        patch.tracks[0].slots[1].pattern.hasStep[0] &&
            patch.tracks[0].slots[1].pattern.steps[0].op == app::PatchObjectOp::Clear);
}

static void test_empty_track_settings_produces_no_sequencer_patch() {
  TEST("empty_track_settings_produces_no_sequencer_patch");

  auto* ws = getParseTestWorkspace();
  auto parsed = parseWS("track(1, TrackSettings {})", ws);
  CHECK("parse ok", parsed.ok);

  app::SequencerPatch patch{};
  auto result = app::doc::buildSequencerPatch(&ws->model.sequencer, 1, 7, &patch);

  CHECK("target ok", result.ok);
  CHECK("track absent", !patch.hasTrack[0]);
}

static void test_sparse_step_note_copies_to_patch() {
  TEST("sparse_step_note_copies_to_patch");

  auto* ws = getParseTestWorkspace();
  auto parsed = parseWS("track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { "
                        "notes = { { note = 64 } } } } } } })",
                        ws);
  CHECK("parse ok", parsed.ok);

  app::SequencerPatch patch{};
  auto result = app::doc::buildSequencerPatch(&ws->model.sequencer, 1, 7, &patch);

  const auto& step = patch.tracks[0].slots[0].pattern.steps[0];
  CHECK("target ok", result.ok);
  CHECK("track present", patch.hasTrack[0]);
  CHECK("slot present", patch.tracks[0].hasSlot[0]);
  CHECK("step present", patch.tracks[0].slots[0].pattern.hasStep[0]);
  CHECK("has note count", step.hasNoteCount);
  CHECK("note count", step.noteCount == 1);
  CHECK("has note patch", step.hasNotePatch[0]);
  CHECK("has note", step.notes[0].hasNote);
  CHECK("note copied", step.notes[0].note == 64);
  CHECK("velocity absent", !step.notes[0].hasVelocity);
}

void runDocSequencerPlannerTests() {
  SUITE("DocSequencerPlanner");
  test_build_sequencer_patch_omits_absent_state();
  test_build_sequencer_patch_copies_authored_slot_patch();
  test_build_sequencer_patch_preserves_sparse_tracks();
  test_build_sequencer_patch_copies_clear_ops();
  test_empty_track_settings_produces_no_sequencer_patch();
  test_sparse_step_note_copies_to_patch();
}
