#include "TestRunner.h"

#include "app/doc/DocAuthoredModel.h"
#include "app/doc/DocMixerPlanner.h"
#include "app/doc/DocSequencerParser.h"

#include <cmath>

static void test_plan_mixer_apply_no_previous_all_writes_become_ops() {
  TEST("plan_mixer_apply_no_previous_all_writes_become_ops");
  auto parsed = app::doc::parseAndNormalizeAuthoredDocument(
      1,
      1,
      "mixer(1, MixerSettings { gain = 0.8, pan = -0.2 })");
  CHECK("ok", parsed.ok);
  auto plan = app::doc::planMixerApply(parsed.model, nullptr);
  CHECK("plan ok", plan.ok);
  CHECK("op count", plan.paramOps.size() == 2);
}

static void test_plan_mixer_apply_same_value_as_previous_no_op() {
  TEST("plan_mixer_apply_same_value_as_previous_no_op");
  auto parsed =
      app::doc::parseAndNormalizeAuthoredDocument(1, 1, "mixer(1, MixerSettings { gain = 0.8 })");
  CHECK("ok", parsed.ok);

  app::doc::AuthoredDocModel previous = parsed.model;
  auto plan = app::doc::planMixerApply(parsed.model, &previous);
  CHECK("plan ok", plan.ok);
  CHECK("no ops", plan.paramOps.empty());
}

static void test_plan_mixer_apply_changed_value_op_produced() {
  TEST("plan_mixer_apply_changed_value_op_produced");
  auto parsed1 =
      app::doc::parseAndNormalizeAuthoredDocument(1, 1, "mixer(1, MixerSettings { gain = 0.8 })");
  auto parsed2 =
      app::doc::parseAndNormalizeAuthoredDocument(1, 2, "mixer(1, MixerSettings { gain = 0.5 })");
  CHECK("ok", parsed1.ok && parsed2.ok);

  auto plan = app::doc::planMixerApply(parsed2.model, &parsed1.model);
  CHECK("one op", plan.paramOps.size() == 1);
  CHECK("gain op",
        plan.paramOps.size() > 0 && plan.paramOps[0].paramID == app::params::AppParamID::TrackGain);
  CHECK("value", plan.paramOps.size() > 0 && plan.paramOps[0].value == 0.5f);
}

static void test_plan_mixer_apply_omitted_track_is_not_planned() {
  TEST("plan_mixer_apply_omitted_track_is_not_planned");
  auto parsed1 =
      app::doc::parseAndNormalizeAuthoredDocument(1,
                                                  1,
                                                  "mixer(1, MixerSettings { gain = 0.8 })\n"
                                                  "mixer(2, MixerSettings { mute = false })");
  auto parsed2 =
      app::doc::parseAndNormalizeAuthoredDocument(1, 2, "mixer(1, MixerSettings { gain = 0.8 })");
  CHECK("ok", parsed1.ok && parsed2.ok);

  auto plan = app::doc::planMixerApply(parsed2.model, &parsed1.model);
  CHECK("plan ok", plan.ok);
  for (const auto& op : plan.paramOps)
    CHECK("track 1 only", op.trackIndex == 0);
}

static void test_plan_mixer_apply_empty_mixer_settings_no_ops() {
  TEST("plan_mixer_apply_empty_mixer_settings_no_ops");
  auto parsed = app::doc::parseAndNormalizeAuthoredDocument(1, 1, "mixer(1, MixerSettings {})");
  CHECK("ok", parsed.ok);
  auto plan = app::doc::planMixerApply(parsed.model, nullptr);
  CHECK("plan ok", plan.ok);
  CHECK("no ops", plan.paramOps.empty());
}

static void test_build_admitted_mixer_target_model_preserves_previous_track_state() {
  TEST("build_admitted_mixer_target_model_preserves_previous_track_state");
  auto parsed1 =
      app::doc::parseAndNormalizeAuthoredDocument(1,
                                                  1,
                                                  "mixer(1, MixerSettings { gain = 0.8 })\n"
                                                  "mixer(2, MixerSettings { mute = true })");
  auto parsed2 =
      app::doc::parseAndNormalizeAuthoredDocument(1, 2, "mixer(1, MixerSettings { gain = 0.5 })");
  CHECK("ok", parsed1.ok && parsed2.ok);

  auto admitted = app::doc::buildAdmittedMixerTargetModel(parsed2.model, &parsed1.model);

  CHECK("track1 gain updated",
        admitted.mixerTracks[0].writes.size() > 0 &&
            admitted.mixerTracks[0].writes[0].value == 0.5f);
  CHECK("track2 preserved", admitted.hasMixerState[1]);
}

static void test_build_admitted_mixer_target_model_upserts_and_preserves_others() {
  TEST("build_admitted_mixer_target_model_upserts_and_preserves_others");
  auto parsed1 = app::doc::parseAndNormalizeAuthoredDocument(
      1,
      1,
      "mixer(1, MixerSettings { gain = 0.8, pan = 0.3 })");
  auto parsed2 =
      app::doc::parseAndNormalizeAuthoredDocument(1, 2, "mixer(1, MixerSettings { gain = 0.5 })");
  CHECK("ok", parsed1.ok && parsed2.ok);

  auto admitted = app::doc::buildAdmittedMixerTargetModel(parsed2.model, &parsed1.model);

  bool panPreserved = false;
  for (const auto& w : admitted.mixerTracks[0].writes) {
    if (w.paramID == app::params::AppParamID::TrackPan)
      panPreserved = (std::abs(w.value - 0.3f) < 1e-5f);
  }
  CHECK("pan preserved", panPreserved);
}

void runDocMixerPlannerTests() {
  SUITE("DocMixerPlanner");
  test_plan_mixer_apply_no_previous_all_writes_become_ops();
  test_plan_mixer_apply_same_value_as_previous_no_op();
  test_plan_mixer_apply_changed_value_op_produced();
  test_plan_mixer_apply_omitted_track_is_not_planned();
  test_plan_mixer_apply_empty_mixer_settings_no_ops();
  test_build_admitted_mixer_target_model_preserves_previous_track_state();
  test_build_admitted_mixer_target_model_upserts_and_preserves_others();
}
