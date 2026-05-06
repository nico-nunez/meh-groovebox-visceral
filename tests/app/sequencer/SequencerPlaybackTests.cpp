#include "TestRunner.h"

#include "app/Sequencer.h"

namespace {

float getParamValue(uint8_t, void*) {
  return 0.0f;
}

struct SequencerFixture {
  app::track::TrackState tracks[app::MAX_TRACKS]{};
  app::sequencer::SequencerState sequencer{};

  SequencerFixture() {
    app::sequencer::InitSequencerContext ctx{};
    ctx.tracksArr = tracks;
    ctx.numTracks = app::sequencer::MAX_LANES;
    ctx.callback = getParamValue;
    app::sequencer::initSequencer(sequencer, ctx);
  }
};

app::sequencer::LanePattern oneStepPattern() {
  app::sequencer::LanePattern pattern{};
  pattern.numSteps = 1;
  pattern.stepsPerBeat = 4;
  pattern.steps[0].active = true;
  pattern.steps[0].noteOn = true;
  pattern.steps[0].note = 60;
  pattern.steps[0].velocity = 100;
  pattern.steps[0].gate = 0.5f;
  return pattern;
}

app::sequencer::LanePattern fourStepPattern() {
  app::sequencer::LanePattern pattern{};
  pattern.numSteps = 4;
  pattern.stepsPerBeat = 1;

  pattern.steps[0].active = true;
  pattern.steps[0].noteOn = true;
  pattern.steps[0].note = 60;
  pattern.steps[0].velocity = 100;

  pattern.steps[1].active = false;

  pattern.steps[2].active = true;
  pattern.steps[2].noteOn = true;
  pattern.steps[2].note = 64;
  pattern.steps[2].velocity = 100;

  pattern.steps[3].active = false;
  return pattern;
}

void admitPattern(app::sequencer::SequencerState& sequencer,
                  const app::sequencer::LanePattern& pattern) {
  auto begin = app::sequencer::beginPatternEdit(sequencer, true);
  CHECK("begin edit", begin.ok);

  auto replace = app::sequencer::replacePattern(sequencer, 0, 0, pattern);
  CHECK("replace pattern", replace.ok);

  auto commit = app::sequencer::commitPattern(sequencer);
  CHECK("commit pattern", commit.ok);
}

int countNoteOn(const app::sequencer::LaneEvents& lane) {
  int count = 0;
  for (uint16_t i = 0; i < lane.count; ++i) {
    const auto& event = lane.events[i];
    if (event.kind == synth::events::ScheduledEvent::Kind::MIDI &&
        event.data.midi.type == synth::events::MIDIEvent::Type::NoteOn) {
      ++count;
    }
  }
  return count;
}

bool hasNoteOn(const app::sequencer::LaneEvents& lane, uint8_t note) {
  for (uint16_t i = 0; i < lane.count; ++i) {
    const auto& event = lane.events[i];
    if (event.kind == synth::events::ScheduledEvent::Kind::MIDI &&
        event.data.midi.type == synth::events::MIDIEvent::Type::NoteOn &&
        event.data.midi.data.noteOn.note == note) {
      return true;
    }
  }
  return false;
}

} // namespace

static void test_one_step_pattern_repeats_across_cycles() {
  TEST("one_step_pattern_repeats_across_cycles");

  SequencerFixture fixture{};
  admitPattern(fixture.sequencer, oneStepPattern());

  app::sequencer::SequencerBlockWindow block0{};
  block0.startBeat = 0.0;
  block0.endBeat = 0.25;
  block0.numFrames = 512;

  app::sequencer::SequencerLaneEvents events0{};
  app::sequencer::runSequencer(fixture.sequencer, block0, events0);
  CHECK("cycle 0 note", hasNoteOn(events0.lanes[0], 60));

  app::sequencer::SequencerBlockWindow block1{};
  block1.startBeat = 0.25;
  block1.endBeat = 0.5;
  block1.numFrames = 512;

  app::sequencer::SequencerLaneEvents events1{};
  app::sequencer::runSequencer(fixture.sequencer, block1, events1);
  CHECK("cycle 1 note", hasNoteOn(events1.lanes[0], 60));

  app::sequencer::SequencerBlockWindow block2{};
  block2.startBeat = 0.5;
  block2.endBeat = 0.75;
  block2.numFrames = 512;

  app::sequencer::SequencerLaneEvents events2{};
  app::sequencer::runSequencer(fixture.sequencer, block2, events2);
  CHECK("cycle 2 note", hasNoteOn(events2.lanes[0], 60));
}

static void test_four_step_pattern_uses_num_steps_not_steps_per_beat() {
  TEST("four_step_pattern_uses_num_steps_not_steps_per_beat");

  SequencerFixture fixture{};
  admitPattern(fixture.sequencer, fourStepPattern());

  app::sequencer::SequencerBlockWindow block{};
  block.startBeat = 0.0;
  block.endBeat = 4.0;
  block.numFrames = 512;

  app::sequencer::SequencerLaneEvents events{};
  app::sequencer::runSequencer(fixture.sequencer, block, events);

  CHECK("two active note ons", countNoteOn(events.lanes[0]) == 2);
  CHECK("has step 1 note", hasNoteOn(events.lanes[0], 60));
  CHECK("has step 3 note", hasNoteOn(events.lanes[0], 64));
}

static void test_stop_resets_last_step_cycle_identity() {
  TEST("stop_resets_last_step_cycle_identity");

  SequencerFixture fixture{};
  admitPattern(fixture.sequencer, oneStepPattern());

  app::sequencer::SequencerBlockWindow block0{};
  block0.startBeat = 0.0;
  block0.endBeat = 0.25;
  block0.numFrames = 512;

  app::sequencer::SequencerLaneEvents events0{};
  app::sequencer::runSequencer(fixture.sequencer, block0, events0);
  CHECK("initial note", hasNoteOn(events0.lanes[0], 60));

  app::sequencer::SequencerBlockWindow stopBlock{};
  stopBlock.startBeat = 0.25;
  stopBlock.endBeat = 0.25;
  stopBlock.numFrames = 0;
  stopBlock.stoppedThisBlock = true;

  app::sequencer::SequencerLaneEvents stopEvents{};
  app::sequencer::runSequencer(fixture.sequencer, stopBlock, stopEvents);

  app::sequencer::SequencerBlockWindow restartBlock{};
  restartBlock.startBeat = 0.0;
  restartBlock.endBeat = 0.25;
  restartBlock.numFrames = 512;

  app::sequencer::SequencerLaneEvents restartEvents{};
  app::sequencer::runSequencer(fixture.sequencer, restartBlock, restartEvents);
  CHECK("restart note", hasNoteOn(restartEvents.lanes[0], 60));
}

void runSequencerPlaybackTests() {
  SUITE("SequencerPlayback");
  test_one_step_pattern_repeats_across_cycles();
  test_four_step_pattern_uses_num_steps_not_steps_per_beat();
  test_stop_resets_last_step_cycle_identity();
}
