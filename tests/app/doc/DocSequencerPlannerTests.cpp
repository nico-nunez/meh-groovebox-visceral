#include "TestRunner.h"

#include "app/doc/DocSequencerPlanner.h"
#include "app/Sequencer.h"

#include <cstdio>

namespace {

app::sequencer::LanePattern oneStepPattern(uint8_t note) {
  app::sequencer::LanePattern pattern{};
  pattern.numSteps = 1;
  pattern.stepsPerBeat = 4;
  pattern.steps[0].active = true;
  pattern.steps[0].noteOn = true;
  pattern.steps[0].note = note;
  pattern.steps[0].velocity = 100;
  return pattern;
}

app::doc::AuthoredTrackSeqModel trackWithSlot(uint8_t trackIndex, uint8_t slot, uint8_t note) {
  app::doc::AuthoredTrackSeqModel track{};
  track.trackIndex = trackIndex;
  track.patterns[slot].occupied = true;
  track.patterns[slot].pattern = oneStepPattern(note);
  track.activeSlot = slot;
  track.activeSlotSource = app::doc::ActivePatternSlotSource::Explicit;
  return track;
}

} // namespace

static void test_unchanged_track_emits_no_op() {
  TEST("unchanged_track_emits_no_op");
  app::doc::AuthoredSeqDocModel previous{};
  previous.hasTrackState[0] = true;
  previous.tracks[0] = trackWithSlot(0, 0, 60);

  app::doc::AuthoredSeqDocModel next{};
  next.hasTrackState[0] = true;
  next.tracks[0] = trackWithSlot(0, 0, 60);

  auto plan = app::doc::planSequencerApply(next, &previous);
  CHECK("ok", plan.ok);
  CHECK("no ops", plan.trackOps.empty());
}

static void test_changed_track_emits_replace_bank() {
  TEST("changed_track_emits_replace_bank");
  app::doc::AuthoredSeqDocModel previous{};
  previous.hasTrackState[0] = true;
  previous.tracks[0] = trackWithSlot(0, 0, 60);

  app::doc::AuthoredSeqDocModel next{};
  next.hasTrackState[0] = true;
  next.tracks[0] = trackWithSlot(0, 0, 61);

  auto plan = app::doc::planSequencerApply(next, &previous);
  CHECK("ok", plan.ok);
  CHECK("one op", plan.trackOps.size() == 1);
  CHECK("trackIndex == 0", plan.trackOps[0].trackIndex == 0);
  CHECK("slot 0 occupied", plan.trackOps[0].bank.slots[0].occupied);
  CHECK("note == 61", plan.trackOps[0].bank.slots[0].pattern.steps[0].note == 61);
}

static void test_explicit_empty_track_emits_empty_bank() {
  TEST("explicit_empty_track_emits_empty_bank");
  app::doc::AuthoredSeqDocModel previous{};
  previous.hasTrackState[0] = true;
  previous.tracks[0] = trackWithSlot(0, 0, 60);

  app::doc::AuthoredSeqDocModel next{};
  next.hasTrackState[0] = true;
  next.tracks[0] = app::doc::AuthoredTrackSeqModel{};
  next.tracks[0].trackIndex = 0;
  next.tracks[0].explicitlyAuthoredEmpty = true;

  auto plan = app::doc::planSequencerApply(next, &previous);
  CHECK("ok", plan.ok);
  CHECK("one op", plan.trackOps.size() == 1);
  CHECK("activeSlot == INVALID",
        plan.trackOps[0].bank.activeSlot == app::sequencer::INVALID_PATTERN_SLOT);

  bool allEmpty = true;
  for (uint8_t slot = 0; slot < app::sequencer::PATTERNS_PER_LANE; ++slot) {
    if (plan.trackOps[0].bank.slots[slot].occupied) {
      allEmpty = false;
      break;
    }
  }
  CHECK("all slots empty", allEmpty);
}

static void test_absent_next_track_emits_no_op() {
  TEST("absent_next_track_emits_no_op");
  app::doc::AuthoredSeqDocModel previous{};
  previous.hasTrackState[0] = true;
  previous.tracks[0] = trackWithSlot(0, 0, 60);

  app::doc::AuthoredSeqDocModel next{};
  next.hasTrackState[0] = false;

  auto plan = app::doc::planSequencerApply(next, &previous);
  CHECK("ok", plan.ok);
  CHECK("no ops", plan.trackOps.empty());
}

static void test_source_spans_do_not_affect_equality() {
  TEST("source_spans_do_not_affect_equality");
  app::doc::AuthoredSeqDocModel previous{};
  previous.hasTrackState[0] = true;
  previous.tracks[0] = trackWithSlot(0, 0, 60);
  previous.tracks[0].trackSpan     = {1,  0, 1,  0};
  previous.tracks[0].patternsSpan  = {2,  0, 2,  0};
  previous.tracks[0].activeSlotSpan = {3, 0, 3,  0};

  app::doc::AuthoredSeqDocModel next{};
  next.hasTrackState[0] = true;
  next.tracks[0] = trackWithSlot(0, 0, 60);
  next.tracks[0].trackSpan      = {10, 0, 10, 0};
  next.tracks[0].patternsSpan   = {20, 0, 20, 0};
  next.tracks[0].activeSlotSpan = {30, 0, 30, 0};

  auto plan = app::doc::planSequencerApply(next, &previous);
  CHECK("ok", plan.ok);
  CHECK("no ops", plan.trackOps.empty());
}

void runDocSequencerPlannerTests() {
  SUITE("DocSequencerPlanner");
  test_unchanged_track_emits_no_op();
  test_changed_track_emits_replace_bank();
  test_explicit_empty_track_emits_empty_bank();
  test_absent_next_track_emits_no_op();
  test_source_spans_do_not_affect_equality();
}
