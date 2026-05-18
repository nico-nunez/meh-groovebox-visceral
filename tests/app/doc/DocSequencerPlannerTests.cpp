#include "TestRunner.h"

#include "app/Sequencer.h"
#include "app/doc/DocSequencerPlanner.h"

namespace {
using app::doc::AuthoredSeqDocModel;
using app::doc::AuthoredTrackSeqModel;
using app::sequencer::LanePattern;

void oneStepPattern(uint8_t note, LanePattern* pattern) {
  pattern->numSteps = 1;
  pattern->stepsPerBeat = 4;
  pattern->steps[0].active = true;
  pattern->steps[0].noteOn = true;
  pattern->steps[0].note = note;
  pattern->steps[0].velocity = 100;
}

void trackWithSlot(uint8_t trackIndex, uint8_t slot, uint8_t note, AuthoredTrackSeqModel* track) {
  static LanePattern patternStorage[32]{};
  static uint8_t nextPattern = 0;

  *track = AuthoredTrackSeqModel{};
  track->trackIndex = trackIndex;
  track->activeSlot = slot;
  track->activeSlotSource = app::doc::ActivePatternSlotSource::Explicit;
  track->patterns[slot].occupied = true;
  track->patterns[slot].pattern = &patternStorage[nextPattern++ % 32];
  oneStepPattern(note, track->patterns[slot].pattern);
}

} // namespace

static void test_build_sequencer_target_snapshot_defaults_omitted_state() {
  TEST("build_sequencer_target_snapshot_defaults_omitted_state");

  AuthoredSeqDocModel model{};
  app::sequencer::PatternSnapshot snapshot{};
  auto result = app::doc::buildSequencerTargetSnapshot(&model, 1, 7, &snapshot);

  CHECK("target ok", result.ok);
  CHECK("lane 1 inactive",
        snapshot.lanes[0].activeSlot == app::sequencer::INVALID_PATTERN_SLOT);
  CHECK("lane 1 slot empty", !snapshot.lanes[0].slots[0].occupied);
}

static void test_build_sequencer_target_snapshot_applies_authored_bank() {
  TEST("build_sequencer_target_snapshot_applies_authored_bank");

  AuthoredSeqDocModel model{};
  model.hasTrackState[0] = true;
  trackWithSlot(0, 0, 61, &model.tracks[0]);

  app::sequencer::PatternSnapshot snapshot{};
  auto result = app::doc::buildSequencerTargetSnapshot(&model, 1, 7, &snapshot);

  CHECK("target ok", result.ok);
  CHECK("active slot", snapshot.lanes[0].activeSlot == 0);
  CHECK("slot occupied", snapshot.lanes[0].slots[0].occupied);
  CHECK("note copied", snapshot.lanes[0].slots[0].pattern.steps[0].note == 61);
}

static void test_build_sequencer_target_snapshot_uses_replacement_defaults() {
  TEST("build_sequencer_target_snapshot_uses_replacement_defaults");

  AuthoredSeqDocModel model{};
  model.hasTrackState[1] = true;
  trackWithSlot(1, 0, 64, &model.tracks[1]);

  app::sequencer::PatternSnapshot snapshot{};
  auto result = app::doc::buildSequencerTargetSnapshot(&model, 1, 7, &snapshot);

  CHECK("target ok", result.ok);
  CHECK("lane 2 authored", snapshot.lanes[1].slots[0].pattern.steps[0].note == 64);
  CHECK("lane 1 default inactive",
        snapshot.lanes[0].activeSlot == app::sequencer::INVALID_PATTERN_SLOT);
  CHECK("lane 1 default empty", !snapshot.lanes[0].slots[0].occupied);
}

void runDocSequencerPlannerTests() {
  SUITE("DocSequencerPlanner");
  test_build_sequencer_target_snapshot_defaults_omitted_state();
  test_build_sequencer_target_snapshot_applies_authored_bank();
  test_build_sequencer_target_snapshot_uses_replacement_defaults();
}
