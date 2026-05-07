#include "TestRunner.h"

#include "app/doc/DocSequencerParser.h"
#include "app/doc/DocSynthPlanner.h"
#include "synth/params/ParamDefs.h"

namespace {

app::doc::AuthoredDocumentNormalizeResult parseDoc(const char* text) {
  return app::doc::parseAndNormalizeAuthoredDocument(1, 7, text);
}

const app::doc::AuthoredSynthParamWrite* findWrite(const app::doc::AuthoredTrackSynthPatch& patch,
                                                   synth::param::ParamID id) {
  for (const auto& write : patch.writes) {
    if (write.paramID == id)
      return &write;
  }
  return nullptr;
}

bool hasOp(const app::doc::PlannedSynthApply& plan,
           uint8_t trackIndex,
           synth::param::ParamID id,
           float value) {
  for (const auto& op : plan.paramOps) {
    if (op.trackIndex == trackIndex && op.paramID == id && op.value == value)
      return true;
  }
  return false;
}

} // namespace

static void test_plan_synth_apply_plans_first_patch_writes() {
  TEST("plan_synth_apply_plans_first_patch_writes");

  auto parsed = parseDoc("synth(1, SynthSettings { osc1 = { mix = 0.8 }, "
                         "svf = { cutoff = 1200 } })");
  CHECK("parse ok", parsed.ok);

  auto plan = app::doc::planSynthApply(parsed.model, nullptr);

  CHECK("plan ok", plan.ok);
  CHECK("two ops", plan.paramOps.size() == 2);
  CHECK("mix op", hasOp(plan, 0, synth::param::OSC1_MIX_LEVEL, 0.8f));
  CHECK("cutoff op", hasOp(plan, 0, synth::param::SVF_CUTOFF, 1200.0f));
}

static void test_plan_synth_apply_skips_unchanged_previous_values() {
  TEST("plan_synth_apply_skips_unchanged_previous_values");

  auto first = parseDoc("synth(1, SynthSettings { osc1 = { mix = 0.8 } })");
  auto previous = app::doc::buildAdmittedSynthTargetModel(first.model, nullptr);

  auto second = parseDoc("synth(1, SynthSettings { osc1 = { mix = 0.8 } })");
  auto plan = app::doc::planSynthApply(second.model, &previous);

  CHECK("plan ok", plan.ok);
  CHECK("no ops", plan.paramOps.empty());
}

static void test_plan_synth_apply_plans_changed_previous_values() {
  TEST("plan_synth_apply_plans_changed_previous_values");

  auto first = parseDoc("synth(1, SynthSettings { osc1 = { mix = 0.8 } })");
  auto previous = app::doc::buildAdmittedSynthTargetModel(first.model, nullptr);

  auto second = parseDoc("synth(1, SynthSettings { osc1 = { mix = 0.25 } })");
  auto plan = app::doc::planSynthApply(second.model, &previous);

  CHECK("plan ok", plan.ok);
  CHECK("one op", plan.paramOps.size() == 1);
  CHECK("changed mix", hasOp(plan, 0, synth::param::OSC1_MIX_LEVEL, 0.25f));
}

static void test_build_admitted_synth_target_preserves_omitted_tracks() {
  TEST("build_admitted_synth_target_preserves_omitted_tracks");

  auto first = parseDoc("synth(1, SynthSettings { osc1 = { mix = 0.8 } })");
  auto previous = app::doc::buildAdmittedSynthTargetModel(first.model, nullptr);

  auto second = parseDoc("synth(2, SynthSettings { svf = { cutoff = 1200 } })");
  auto admitted = app::doc::buildAdmittedSynthTargetModel(second.model, &previous);

  CHECK("track 0 preserved", admitted.hasSynthState[0]);
  CHECK("track 1 present", admitted.hasSynthState[1]);
  CHECK("track 0 mix preserved",
        findWrite(admitted.synthTracks[0], synth::param::OSC1_MIX_LEVEL) != nullptr);
  CHECK("track 1 cutoff present",
        findWrite(admitted.synthTracks[1], synth::param::SVF_CUTOFF) != nullptr);
}

static void test_build_admitted_synth_target_merges_omitted_params() {
  TEST("build_admitted_synth_target_merges_omitted_params");

  auto first = parseDoc("synth(1, SynthSettings { osc1 = { mix = 0.8 }, "
                        "svf = { cutoff = 1200 } })");
  auto previous = app::doc::buildAdmittedSynthTargetModel(first.model, nullptr);

  auto second = parseDoc("synth(1, SynthSettings { osc1 = { mix = 0.25 } })");
  auto admitted = app::doc::buildAdmittedSynthTargetModel(second.model, &previous);

  const auto* mix = findWrite(admitted.synthTracks[0], synth::param::OSC1_MIX_LEVEL);
  const auto* cutoff = findWrite(admitted.synthTracks[0], synth::param::SVF_CUTOFF);

  CHECK("mix updated", mix && mix->value == 0.25f);
  CHECK("cutoff preserved", cutoff && cutoff->value == 1200.0f);
}

void runDocSynthPlannerTests() {
  SUITE("DocSynthPlanner");
  test_plan_synth_apply_plans_first_patch_writes();
  test_plan_synth_apply_skips_unchanged_previous_values();
  test_plan_synth_apply_plans_changed_previous_values();
  test_build_admitted_synth_target_preserves_omitted_tracks();
  test_build_admitted_synth_target_merges_omitted_params();
}
