#include "TestRunner.h"
#include "TestHelpers.h"

#include "app/doc/DocGrooveboxTargetBuilder.h"

#include "synth/params/ParamDefs.h"

namespace {
using test::parseDoc;

app::GrooveboxTargetState& targetScratch() {
  static app::GrooveboxTargetState target{};
  return target;
}

} // namespace

static void test_target_builder_defaults_omitted_state() {
  TEST("target_builder_defaults_omitted_state");

  auto parsed = parseDoc("");
  CHECK("parse ok", parsed.ok);

  auto& target = targetScratch();
  auto result = app::doc::buildGrooveboxTargetState(&parsed.model, 1, 7, &target);

  CHECK("target ok", result.ok);
  CHECK("mixer present", target.hasMixer);
  CHECK("sequencer present", target.hasSequencer);
  CHECK("track 1 synth present", target.hasSynthProgram[0]);
  CHECK("default synth mix",
        target.synthPrograms[0].paramValues[synth::param::OSC1_MIX_LEVEL] ==
            synth::param::PARAM_DEFS[synth::param::OSC1_MIX_LEVEL].defaultVal);
  CHECK("default mixer gain", target.mixer.tracks[0].gain == 1.0f);
  CHECK("default sequencer inactive",
        target.sequencer.lanes[0].activeSlot == app::sequencer::INVALID_PATTERN_SLOT);
}

static void test_target_builder_applies_authored_synth_values() {
  TEST("target_builder_applies_authored_synth_values");

  auto parsed =
      parseDoc("synth(1, SynthSettings { osc1 = { mix = 0.25 }, svf = { cutoff = 1200 } })");
  CHECK("parse ok", parsed.ok);

  auto& target = targetScratch();
  auto result = app::doc::buildGrooveboxTargetState(&parsed.model, 1, 7, &target);

  CHECK("target ok", result.ok);
  CHECK("mix applied", target.synthPrograms[0].paramValues[synth::param::OSC1_MIX_LEVEL] == 0.25f);
  CHECK("cutoff applied", target.synthPrograms[0].paramValues[synth::param::SVF_CUTOFF] == 1200.0f);
  CHECK("other track default",
        target.synthPrograms[1].paramValues[synth::param::OSC1_MIX_LEVEL] ==
            synth::param::PARAM_DEFS[synth::param::OSC1_MIX_LEVEL].defaultVal);
}

static void test_target_builder_applies_authored_mixer_values() {
  TEST("target_builder_applies_authored_mixer_values");

  auto parsed = parseDoc("mixer(1, MixerSettings { gain = 0.5, pan = -0.25, mute = true })");
  CHECK("parse ok", parsed.ok);

  auto& target = targetScratch();
  auto result = app::doc::buildGrooveboxTargetState(&parsed.model, 1, 7, &target);

  CHECK("target ok", result.ok);
  CHECK("gain applied", target.mixer.tracks[0].gain == 0.5f);
  CHECK("pan applied", target.mixer.tracks[0].pan == -0.25f);
  CHECK("mute applied", target.mixer.tracks[0].enabled == false);
  CHECK("other track default", target.mixer.tracks[1].gain == 1.0f);
}

static void test_target_builder_applies_authored_sequencer_bank() {
  TEST("target_builder_applies_authored_sequencer_bank");

  auto parsed = parseDoc("track(1, TrackSettings { patterns = { [1] = { numSteps = 1, "
                         "stepsPerBeat = 4, steps = { { note = 36, active = true } } } }, "
                         "activeSlot = 1 })");
  CHECK("parse ok", parsed.ok);

  auto& target = targetScratch();
  auto result = app::doc::buildGrooveboxTargetState(&parsed.model, 1, 7, &target);

  CHECK("target ok", result.ok);
  CHECK("slot occupied", target.sequencer.lanes[0].slots[0].occupied);
  CHECK("active slot", target.sequencer.lanes[0].activeSlot == 0);
  CHECK("step active", target.sequencer.lanes[0].slots[0].pattern.steps[0].active);
  CHECK("step note", target.sequencer.lanes[0].slots[0].pattern.steps[0].note == 36);
}

static void test_target_builder_uses_replacement_not_previous_carry_forward() {
  TEST("target_builder_uses_replacement_not_previous_carry_forward");

  auto parsed = parseDoc("synth(2, SynthSettings { osc1 = { mix = 0.75 } })");
  CHECK("parse ok", parsed.ok);

  auto& target = targetScratch();
  auto result = app::doc::buildGrooveboxTargetState(&parsed.model, 1, 7, &target);

  CHECK("target ok", result.ok);
  CHECK("track 2 authored",
        target.synthPrograms[1].paramValues[synth::param::OSC1_MIX_LEVEL] == 0.75f);
  CHECK("track 1 default",
        target.synthPrograms[0].paramValues[synth::param::OSC1_MIX_LEVEL] ==
            synth::param::PARAM_DEFS[synth::param::OSC1_MIX_LEVEL].defaultVal);
}

void runDocGrooveboxTargetBuilderTests() {
  SUITE("DocGrooveboxTargetBuilder");
  test_target_builder_defaults_omitted_state();
  test_target_builder_applies_authored_synth_values();
  test_target_builder_applies_authored_mixer_values();
  test_target_builder_applies_authored_sequencer_bank();
  test_target_builder_uses_replacement_not_previous_carry_forward();
}
