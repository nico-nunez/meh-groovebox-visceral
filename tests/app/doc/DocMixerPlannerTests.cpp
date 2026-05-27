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

static void test_empty_document_emits_no_mixer_writes() {
  TEST("empty_document_emits_no_mixer_writes");

  auto* ws = getParseTestWorkspace();
  auto parsed = parseWS("", ws);
  CHECK("parse ok", parsed.ok);

  app::MixerPatch patch{};
  auto result = app::doc::buildMixerPatch(&ws->model, 1, 7, &patch);

  CHECK("target ok", result.ok);
  CHECK("no writes", patch.writeCount == 0);
}

static void test_gain_only_emits_one_mixer_write() {
  TEST("gain_only_emits_one_mixer_write");

  auto* ws = getParseTestWorkspace();
  auto parsed = parseWS("mixer(1, MixerSettings { gain = 0.7 })", ws);
  CHECK("parse ok", parsed.ok);

  app::MixerPatch patch{};
  auto result = app::doc::buildMixerPatch(&ws->model, 1, 7, &patch);

  CHECK("target ok", result.ok);
  CHECK("one write", patch.writeCount == 1);
  CHECK("track 0", patch.writes[0].trackIndex == 0);
  CHECK("gain param", patch.writes[0].paramID == app::params::AppParamID::TrackGain);
  CHECK("gain value", near(patch.writes[0].value, 0.7f));
}

static void test_gain_only_does_not_emit_pan_or_mute() {
  TEST("gain_only_does_not_emit_pan_or_mute");

  auto* ws = getParseTestWorkspace();
  auto parsed = parseWS("mixer(1, MixerSettings { gain = 0.7 })", ws);
  CHECK("parse ok", parsed.ok);

  app::MixerPatch patch{};
  auto result = app::doc::buildMixerPatch(&ws->model, 1, 7, &patch);

  CHECK("target ok", result.ok);
  for (uint16_t i = 0; i < patch.writeCount; ++i) {
    CHECK("not pan", patch.writes[i].paramID != app::params::AppParamID::TrackPan);
    CHECK("not mute", patch.writes[i].paramID != app::params::AppParamID::TrackMute);
  }
}

static void test_all_track_fields_emit_sparse_writes() {
  TEST("all_track_fields_emit_sparse_writes");

  auto* ws = getParseTestWorkspace();
  auto parsed = parseWS("mixer(1, MixerSettings { gain = 0.8, pan = -0.2, mute = true })", ws);
  CHECK("parse ok", parsed.ok);

  app::MixerPatch patch{};
  auto result = app::doc::buildMixerPatch(&ws->model, 1, 7, &patch);

  CHECK("target ok", result.ok);
  CHECK("three writes", patch.writeCount == 3);
  CHECK("gain applied", patch.writes[0].paramID == app::params::AppParamID::TrackGain &&
                            near(patch.writes[0].value, 0.8f));
  CHECK("pan applied", patch.writes[1].paramID == app::params::AppParamID::TrackPan &&
                           near(patch.writes[1].value, -0.2f));
  CHECK("mute applied", patch.writes[2].paramID == app::params::AppParamID::TrackMute &&
                            near(patch.writes[2].value, 1.0f));
}

void runDocMixerPlannerTests() {
  SUITE("DocMixerPlanner");
  test_empty_document_emits_no_mixer_writes();
  test_gain_only_emits_one_mixer_write();
  test_gain_only_does_not_emit_pan_or_mute();
  test_all_track_fields_emit_sparse_writes();
}
