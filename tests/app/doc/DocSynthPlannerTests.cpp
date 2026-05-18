#include "TestHelpers.h"
#include "TestRunner.h"

#include "app/doc/DocSynthPlanner.h"
#include "synth/params/ParamDefs.h"

namespace {

using test::parseDoc;

} // namespace

static void test_build_synth_target_programs_defaults_all_tracks() {
  TEST("build_synth_target_programs_defaults_all_tracks");

  auto parsed = parseDoc("");
  CHECK("parse ok", parsed.ok);

  synth::program::SynthProgram programs[app::MAX_TRACKS]{};
  auto result = app::doc::buildSynthTargetPrograms(&parsed.model, 1, 7, programs);

  CHECK("target ok", result.ok);
  CHECK("track 1 default mix",
        programs[0].paramValues[synth::param::OSC1_MIX_LEVEL] ==
            synth::param::PARAM_DEFS[synth::param::OSC1_MIX_LEVEL].defaultVal);
  CHECK("track 2 default mix",
        programs[1].paramValues[synth::param::OSC1_MIX_LEVEL] ==
            synth::param::PARAM_DEFS[synth::param::OSC1_MIX_LEVEL].defaultVal);
}

static void test_build_synth_target_programs_applies_authored_values() {
  TEST("build_synth_target_programs_applies_authored_values");

  auto parsed = parseDoc("synth(1, SynthSettings { osc1 = { mix = 0.8 }, "
                         "svf = { cutoff = 1200 } })");
  CHECK("parse ok", parsed.ok);

  synth::program::SynthProgram programs[app::MAX_TRACKS]{};
  auto result = app::doc::buildSynthTargetPrograms(&parsed.model, 1, 7, programs);

  CHECK("target ok", result.ok);
  CHECK("mix applied", programs[0].paramValues[synth::param::OSC1_MIX_LEVEL] == 0.8f);
  CHECK("cutoff applied", programs[0].paramValues[synth::param::SVF_CUTOFF] == 1200.0f);
}

static void test_build_synth_target_programs_uses_replacement_defaults() {
  TEST("build_synth_target_programs_uses_replacement_defaults");

  auto parsed = parseDoc("synth(2, SynthSettings { osc1 = { mix = 0.25 } })");
  CHECK("parse ok", parsed.ok);

  synth::program::SynthProgram programs[app::MAX_TRACKS]{};
  auto result = app::doc::buildSynthTargetPrograms(&parsed.model, 1, 7, programs);

  CHECK("target ok", result.ok);
  CHECK("track 2 authored", programs[1].paramValues[synth::param::OSC1_MIX_LEVEL] == 0.25f);
  CHECK("track 1 default",
        programs[0].paramValues[synth::param::OSC1_MIX_LEVEL] ==
            synth::param::PARAM_DEFS[synth::param::OSC1_MIX_LEVEL].defaultVal);
}

void runDocSynthPlannerTests() {
  SUITE("DocSynthPlanner");
  test_build_synth_target_programs_defaults_all_tracks();
  test_build_synth_target_programs_applies_authored_values();
  test_build_synth_target_programs_uses_replacement_defaults();
}
