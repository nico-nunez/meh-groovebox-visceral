#include "TestRunner.h"

#include "app/Sequencer.h"

namespace {

float getParamValue(uint8_t, void*) {
  return 0.0f;
}

int countMidiType(const app::sequencer::LaneEvents& lane,
                  synth::events::MIDIEvent::Type type,
                  uint8_t note) {
  int count = 0;
  for (uint16_t i = 0; i < lane.count; ++i) {
    const auto& event = lane.events[i];
    if (event.kind != synth::events::ScheduledEvent::Kind::MIDI)
      continue;
    if (event.data.midi.type != type)
      continue;
    if (type == synth::events::MIDIEvent::Type::NoteOn && event.data.midi.data.noteOn.note == note)
      ++count;
    if (type == synth::events::MIDIEvent::Type::NoteOff &&
        event.data.midi.data.noteOff.note == note)
      ++count;
  }
  return count;
}

const synth::events::ScheduledEvent* findMidiEvent(const app::sequencer::LaneEvents& lane,
                                                   synth::events::MIDIEvent::Type type,
                                                   uint8_t note) {
  for (uint16_t i = 0; i < lane.count; ++i) {
    const auto& event = lane.events[i];
    if (event.kind != synth::events::ScheduledEvent::Kind::MIDI)
      continue;
    if (event.data.midi.type != type)
      continue;
    if (type == synth::events::MIDIEvent::Type::NoteOn && event.data.midi.data.noteOn.note == note)
      return &event;
    if (type == synth::events::MIDIEvent::Type::NoteOff &&
        event.data.midi.data.noteOff.note == note)
      return &event;
  }
  return nullptr;
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
  pattern.steps[0].noteCount = 1;
  pattern.steps[0].notes[0] = app::sequencer::StepNote{true, false, 60, 100, 0.5f};

  return pattern;
}

app::sequencer::LanePattern fourStepPattern() {
  app::sequencer::LanePattern pattern{};
  pattern.numSteps = 4;
  pattern.stepsPerBeat = 1;

  pattern.steps[0].active = true;
  pattern.steps[0].noteCount = 1;
  pattern.steps[0].notes[0] = app::sequencer::StepNote{true, false, 60, 100, 0.5f};

  pattern.steps[1].active = false;
  pattern.steps[1].noteCount = 0;

  pattern.steps[2].active = true;
  pattern.steps[2].noteCount = 1;
  pattern.steps[2].notes[0] = app::sequencer::StepNote{true, false, 64, 100, 0.5f};

  pattern.steps[3].active = false;
  pattern.steps[3].noteCount = 0;

  return pattern;
}

void admitPattern(app::sequencer::SequencerState& sequencer,
                  const app::sequencer::LanePattern& pattern) {
  app::sequencer::PatternSnapshot snapshot = app::sequencer::getPatternSnapshot(sequencer);
  snapshot.lanes[0].slots[0].occupied = true;
  snapshot.lanes[0].slots[0].pattern = pattern;
  snapshot.lanes[0].activeSlot = 0;

  auto prepare = app::sequencer::prepareSequencerSnapshotSwap(sequencer, snapshot);
  CHECK("prepare sequencer snapshot", prepare.ok);

  auto commit = app::sequencer::commitSequencerSnapshotSwap(sequencer);
  CHECK("commit sequencer snapshot", commit.ok);

  app::sequencer::publishPendingSequencerSnapshotIfReady(sequencer);
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

static void test_gate_overlap_allows_different_note_legato() {
  TEST("gate_overlap_allows_different_note_legato");

  SequencerFixture fixture{};
  app::sequencer::LanePattern pattern{};
  pattern.numSteps = 4;
  pattern.stepsPerBeat = 1;

  pattern.steps[0].active = true;
  pattern.steps[0].noteCount = 1;
  pattern.steps[0].notes[0] = app::sequencer::StepNote{true, false, 60, 100, 2.5f};

  pattern.steps[2].active = true;
  pattern.steps[2].noteCount = 1;
  pattern.steps[2].notes[0] = app::sequencer::StepNote{true, false, 62, 100, 1.0f};

  admitPattern(fixture.sequencer, pattern);

  app::sequencer::SequencerBlockWindow block{};
  block.startBeat = 0.0;
  block.endBeat = 4.0;
  block.numFrames = 4000;

  app::sequencer::SequencerLaneEvents events{};
  app::sequencer::runSequencer(fixture.sequencer, block, events);

  const auto* cOn = findMidiEvent(events.lanes[0], synth::events::MIDIEvent::Type::NoteOn, 60);
  const auto* dOn = findMidiEvent(events.lanes[0], synth::events::MIDIEvent::Type::NoteOn, 62);
  const auto* cOff = findMidiEvent(events.lanes[0], synth::events::MIDIEvent::Type::NoteOff, 60);

  CHECK("C on", cOn != nullptr);
  CHECK("D on", dOn != nullptr);
  CHECK("C off", cOff != nullptr);
  CHECK("D starts before C releases", dOn->sampleOffset < cOff->sampleOffset);
}

static void test_gate_overlap_same_note_retriggers() {
  TEST("gate_overlap_same_note_retriggers");

  SequencerFixture fixture{};
  app::sequencer::LanePattern pattern{};
  pattern.numSteps = 4;
  pattern.stepsPerBeat = 1;

  pattern.steps[0].active = true;
  pattern.steps[0].noteCount = 1;
  pattern.steps[0].notes[0] = app::sequencer::StepNote{true, false, 60, 100, 2.5f};

  pattern.steps[2].active = true;
  pattern.steps[2].noteCount = 1;
  pattern.steps[2].notes[0] = app::sequencer::StepNote{true, false, 60, 100, 1.0f};

  admitPattern(fixture.sequencer, pattern);

  app::sequencer::SequencerBlockWindow block{};
  block.startBeat = 0.0;
  block.endBeat = 4.0;
  block.numFrames = 4000;

  app::sequencer::SequencerLaneEvents events{};
  app::sequencer::runSequencer(fixture.sequencer, block, events);

  int retriggerCutOffs = 0;
  int lateOldGateOffs = 0;
  for (uint16_t i = 0; i < events.lanes[0].count; ++i) {
    const auto& event = events.lanes[0].events[i];
    if (event.kind != synth::events::ScheduledEvent::Kind::MIDI)
      continue;
    if (event.data.midi.type != synth::events::MIDIEvent::Type::NoteOff)
      continue;
    if (event.data.midi.data.noteOff.note != 60)
      continue;
    if (event.sampleOffset == 2000)
      ++retriggerCutOffs;
    if (event.sampleOffset == 2500)
      ++lateOldGateOffs;
  }

  CHECK("same-note cut happens at retrigger", retriggerCutOffs == 1);
  CHECK("old gate off was cancelled", lateOldGateOffs == 0);
  CHECK("two same-note ons",
        countMidiType(events.lanes[0], synth::events::MIDIEvent::Type::NoteOn, 60) == 2);
  CHECK("two same-note offs",
        countMidiType(events.lanes[0], synth::events::MIDIEvent::Type::NoteOff, 60) == 2);
}

static void test_polyphonic_step_emits_multiple_note_ons() {
  TEST("polyphonic_step_emits_multiple_note_ons");

  SequencerFixture fixture{};
  app::sequencer::LanePattern pattern{};
  pattern.numSteps = 1;
  pattern.stepsPerBeat = 1;
  pattern.steps[0].active = true;
  pattern.steps[0].noteCount = 3;
  pattern.steps[0].notes[0] = {true, false, 60, 100, 1.0f};
  pattern.steps[0].notes[1] = {true, false, 64, 100, 1.0f};
  pattern.steps[0].notes[2] = {true, false, 67, 100, 1.0f};

  admitPattern(fixture.sequencer, pattern);

  app::sequencer::SequencerBlockWindow block{};
  block.startBeat = 0.0;
  block.endBeat = 1.0;
  block.numFrames = 1000;

  app::sequencer::SequencerLaneEvents events{};
  app::sequencer::runSequencer(fixture.sequencer, block, events);

  CHECK("C", hasNoteOn(events.lanes[0], 60));
  CHECK("E", hasNoteOn(events.lanes[0], 64));
  CHECK("G", hasNoteOn(events.lanes[0], 67));
}

static void test_polyphonic_step_notes_have_independent_gates() {
  TEST("polyphonic_step_notes_have_independent_gates");

  SequencerFixture fixture{};
  app::sequencer::LanePattern pattern{};
  pattern.numSteps = 1;
  pattern.stepsPerBeat = 1;
  pattern.steps[0].active = true;
  pattern.steps[0].noteCount = 2;
  pattern.steps[0].notes[0] = {true, false, 60, 100, 0.5f};
  pattern.steps[0].notes[1] = {true, false, 64, 100, 1.0f};

  admitPattern(fixture.sequencer, pattern);

  app::sequencer::SequencerBlockWindow block{};
  block.startBeat = 0.0;
  block.endBeat = 2.0;
  block.numFrames = 2000;

  app::sequencer::SequencerLaneEvents events{};
  app::sequencer::runSequencer(fixture.sequencer, block, events);

  const auto* cOff = findMidiEvent(events.lanes[0], synth::events::MIDIEvent::Type::NoteOff, 60);
  const auto* eOff = findMidiEvent(events.lanes[0], synth::events::MIDIEvent::Type::NoteOff, 64);
  CHECK("C off", cOff != nullptr);
  CHECK("E off", eOff != nullptr);
  CHECK("C releases first", cOff->sampleOffset < eOff->sampleOffset);
}

static void test_polyphonic_gate_overlap_keeps_old_different_note_held() {
  TEST("polyphonic_gate_overlap_keeps_old_different_note_held");

  SequencerFixture fixture{};
  app::sequencer::LanePattern pattern{};
  pattern.numSteps = 4;
  pattern.stepsPerBeat = 1;
  pattern.steps[0].active = true;
  pattern.steps[0].noteCount = 1;
  pattern.steps[0].notes[0] = {true, false, 60, 100, 2.5f};
  pattern.steps[2].active = true;
  pattern.steps[2].noteCount = 1;
  pattern.steps[2].notes[0] = {true, false, 62, 100, 1.0f};

  admitPattern(fixture.sequencer, pattern);

  app::sequencer::SequencerBlockWindow block{};
  block.startBeat = 0.0;
  block.endBeat = 4.0;
  block.numFrames = 4000;

  app::sequencer::SequencerLaneEvents events{};
  app::sequencer::runSequencer(fixture.sequencer, block, events);

  const auto* dOn = findMidiEvent(events.lanes[0], synth::events::MIDIEvent::Type::NoteOn, 62);
  const auto* cOff = findMidiEvent(events.lanes[0], synth::events::MIDIEvent::Type::NoteOff, 60);
  CHECK("D on", dOn != nullptr);
  CHECK("C off", cOff != nullptr);
  CHECK("D starts before C releases", dOn->sampleOffset < cOff->sampleOffset);
}

static void test_polyphonic_same_note_overlap_retriggers() {
  TEST("polyphonic_same_note_overlap_retriggers");

  SequencerFixture fixture{};
  app::sequencer::LanePattern pattern{};
  pattern.numSteps = 4;
  pattern.stepsPerBeat = 1;
  pattern.steps[0].active = true;
  pattern.steps[0].noteCount = 1;
  pattern.steps[0].notes[0] = {true, false, 60, 100, 2.5f};
  pattern.steps[2].active = true;
  pattern.steps[2].noteCount = 1;
  pattern.steps[2].notes[0] = {true, false, 60, 100, 1.0f};

  admitPattern(fixture.sequencer, pattern);

  app::sequencer::SequencerBlockWindow block{};
  block.startBeat = 0.0;
  block.endBeat = 4.0;
  block.numFrames = 4000;

  app::sequencer::SequencerLaneEvents events{};
  app::sequencer::runSequencer(fixture.sequencer, block, events);

  CHECK("two note-ons",
        countMidiType(events.lanes[0], synth::events::MIDIEvent::Type::NoteOn, 60) == 2);
  CHECK("two note-offs",
        countMidiType(events.lanes[0], synth::events::MIDIEvent::Type::NoteOff, 60) == 2);
}

static void test_same_note_tie_extends_without_retrigger() {
  TEST("same_note_tie_extends_without_retrigger");

  SequencerFixture fixture{};
  app::sequencer::LanePattern pattern{};
  pattern.numSteps = 4;
  pattern.stepsPerBeat = 1;

  pattern.steps[0].active = true;
  pattern.steps[0].noteCount = 1;
  pattern.steps[0].notes[0] = {true, false, 60, 100, 1.0f};

  pattern.steps[1].active = true;
  pattern.steps[1].noteCount = 1;
  pattern.steps[1].notes[0] = {true, true, 60, 100, 2.0f};

  admitPattern(fixture.sequencer, pattern);

  app::sequencer::SequencerBlockWindow block{};
  block.startBeat = 0.0;
  block.endBeat = 4.0;
  block.numFrames = 4000;

  app::sequencer::SequencerLaneEvents events{};
  app::sequencer::runSequencer(fixture.sequencer, block, events);

  int offAtTieBoundary = 0;
  int offAtExtendedGate = 0;
  for (uint16_t i = 0; i < events.lanes[0].count; ++i) {
    const auto& event = events.lanes[0].events[i];
    if (event.kind != synth::events::ScheduledEvent::Kind::MIDI)
      continue;
    if (event.data.midi.type != synth::events::MIDIEvent::Type::NoteOff)
      continue;
    if (event.data.midi.data.noteOff.note != 60)
      continue;
    if (event.sampleOffset == 1000)
      ++offAtTieBoundary;
    if (event.sampleOffset == 3000)
      ++offAtExtendedGate;
  }

  CHECK("one tied note-on",
        countMidiType(events.lanes[0], synth::events::MIDIEvent::Type::NoteOn, 60) == 1);
  CHECK("no note-off at tie boundary", offAtTieBoundary == 0);
  CHECK("extended gate note-off", offAtExtendedGate == 1);
  CHECK("one tied note-off",
        countMidiType(events.lanes[0], synth::events::MIDIEvent::Type::NoteOff, 60) == 1);
}

static void test_chord_tie_preserves_shared_pitch() {
  TEST("chord_tie_preserves_shared_pitch");

  SequencerFixture fixture{};
  app::sequencer::LanePattern pattern{};
  pattern.numSteps = 4;
  pattern.stepsPerBeat = 1;

  pattern.steps[0].active = true;
  pattern.steps[0].noteCount = 2;
  pattern.steps[0].notes[0] = {true, false, 60, 100, 1.0f};
  pattern.steps[0].notes[1] = {true, false, 64, 100, 1.0f};

  pattern.steps[1].active = true;
  pattern.steps[1].noteCount = 2;
  pattern.steps[1].notes[0] = {true, false, 62, 100, 1.0f};
  pattern.steps[1].notes[1] = {true, true, 64, 100, 2.0f};

  admitPattern(fixture.sequencer, pattern);

  app::sequencer::SequencerBlockWindow block{};
  block.startBeat = 0.0;
  block.endBeat = 4.0;
  block.numFrames = 4000;

  app::sequencer::SequencerLaneEvents events{};
  app::sequencer::runSequencer(fixture.sequencer, block, events);

  int eOffAtBoundary = 0;
  int eOffAtExtendedGate = 0;
  for (uint16_t i = 0; i < events.lanes[0].count; ++i) {
    const auto& event = events.lanes[0].events[i];
    if (event.kind != synth::events::ScheduledEvent::Kind::MIDI)
      continue;
    if (event.data.midi.type != synth::events::MIDIEvent::Type::NoteOff)
      continue;
    if (event.data.midi.data.noteOff.note != 64)
      continue;
    if (event.sampleOffset == 1000)
      ++eOffAtBoundary;
    if (event.sampleOffset == 3000)
      ++eOffAtExtendedGate;
  }

  CHECK("C starts", hasNoteOn(events.lanes[0], 60));
  CHECK("D starts", hasNoteOn(events.lanes[0], 62));
  CHECK("E starts once",
        countMidiType(events.lanes[0], synth::events::MIDIEvent::Type::NoteOn, 64) == 1);
  CHECK("E not released at chord boundary", eOffAtBoundary == 0);
  CHECK("E released after tied extension", eOffAtExtendedGate == 1);
}

void runSequencerPlaybackTests() {
  SUITE("SequencerPlayback");
  test_one_step_pattern_repeats_across_cycles();
  test_four_step_pattern_uses_num_steps_not_steps_per_beat();
  test_stop_resets_last_step_cycle_identity();
  test_gate_overlap_allows_different_note_legato();
  test_gate_overlap_same_note_retriggers();
  test_polyphonic_step_emits_multiple_note_ons();
  test_polyphonic_step_notes_have_independent_gates();
  test_polyphonic_gate_overlap_keeps_old_different_note_held();
  test_polyphonic_same_note_overlap_retriggers();
  test_same_note_tie_extends_without_retrigger();
  test_chord_tie_preserves_shared_pitch();
}
