#include "TestHelpers.h"
#include "TestRunner.h"

#include "app/doc/DocMixerPlanner.h"

#include <cmath>

namespace {

using test::getParseTestWorkspace;
using test::parseWS;

bool near(float a, float b) {
  return std::abs(a - b) < 1e-5f;
}

} // namespace

static void test_build_mixer_target_snapshot_defaults_omitted_state() {
  TEST("build_mixer_target_snapshot_defaults_omitted_state");

  auto* ws = getParseTestWorkspace();
  auto parsed = parseWS("", ws);
  CHECK("parse ok", parsed.ok);

  app::mixer::MixerSnapshot snapshot{};
  auto result = app::doc::buildMixerTargetSnapshot(&ws->model, 1, 7, &snapshot);

  CHECK("target ok", result.ok);
  CHECK("track 1 default gain", near(snapshot.tracks[0].gain, 1.0f));
  CHECK("track 1 default pan", near(snapshot.tracks[0].pan, 0.0f));
  CHECK("track 1 enabled", snapshot.tracks[0].enabled);
  CHECK("master gain default", near(snapshot.masterGain, 1.0f));
}

static void test_build_mixer_target_snapshot_applies_track_values() {
  TEST("build_mixer_target_snapshot_applies_track_values");

  auto* ws = getParseTestWorkspace();
  auto parsed = parseWS("mixer(1, MixerSettings { gain = 0.8, pan = -0.2, mute = true })", ws);
  CHECK("parse ok", parsed.ok);

  app::mixer::MixerSnapshot snapshot{};
  auto result = app::doc::buildMixerTargetSnapshot(&ws->model, 1, 7, &snapshot);

  CHECK("target ok", result.ok);
  CHECK("gain applied", near(snapshot.tracks[0].gain, 0.8f));
  CHECK("pan applied", near(snapshot.tracks[0].pan, -0.2f));
  CHECK("mute applied", !snapshot.tracks[0].enabled);
}

static void test_build_mixer_target_snapshot_uses_replacement_defaults() {
  TEST("build_mixer_target_snapshot_uses_replacement_defaults");

  auto* ws = getParseTestWorkspace();
  auto parsed = parseWS("mixer(2, MixerSettings { gain = 0.5 })", ws);
  CHECK("parse ok", parsed.ok);

  app::mixer::MixerSnapshot snapshot{};
  auto result = app::doc::buildMixerTargetSnapshot(&ws->model, 1, 7, &snapshot);

  CHECK("target ok", result.ok);
  CHECK("track 2 authored", near(snapshot.tracks[1].gain, 0.5f));
  CHECK("track 1 default", near(snapshot.tracks[0].gain, 1.0f));
}

void runDocMixerPlannerTests() {
  SUITE("DocMixerPlanner");
  test_build_mixer_target_snapshot_defaults_omitted_state();
  test_build_mixer_target_snapshot_applies_track_values();
  test_build_mixer_target_snapshot_uses_replacement_defaults();
}
