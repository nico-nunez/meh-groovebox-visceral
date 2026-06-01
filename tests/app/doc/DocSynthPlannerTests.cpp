#include "TestHelpers.h"
#include "TestRunner.h"

#include "app/doc/DocSynthPlanner.h"
#include "synth/params/ParamDefs.h"

namespace {

using test::getParseTestWorkspace;
using test::parseWS;

const app::SynthParamPatchWrite* findSynthWrite(const app::TrackSynthPatch& patch,
                                                synth::param::ParamID paramID) {
  for (uint16_t i = 0; i < patch.writeCount; ++i) {
    if (patch.writes[i].paramID == paramID)
      return &patch.writes[i];
  }
  return nullptr;
}

} // namespace

static void test_empty_document_emits_no_synth_patches() {
  TEST("empty_document_emits_no_synth_patches");

  auto* ws = getParseTestWorkspace();
  auto parsed = parseWS("", ws);
  CHECK("parse ok", parsed.ok);

  app::TrackSynthPatch patches[app::MAX_TRACKS]{};
  bool hasSynth[app::MAX_TRACKS]{};
  auto result = app::doc::buildSynthPatches(&ws->model, 1, 7, patches, hasSynth);

  CHECK("target ok", result.ok);
  for (uint8_t t = 0; t < app::MAX_TRACKS; ++t) {
    CHECK("no synth patch", !hasSynth[t]);
    CHECK("no writes", patches[t].writeCount == 0);
  }
}

static void test_osc_mix_only_emits_one_synth_write() {
  TEST("osc_mix_only_emits_one_synth_write");

  auto* ws = getParseTestWorkspace();
  auto parsed = parseWS("synth(1, SynthSettings { osc1 = { mixLevel = 0.5 } })", ws);
  CHECK("parse ok", parsed.ok);

  app::TrackSynthPatch patches[app::MAX_TRACKS]{};
  bool hasSynth[app::MAX_TRACKS]{};
  auto result = app::doc::buildSynthPatches(&ws->model, 1, 7, patches, hasSynth);

  CHECK("target ok", result.ok);
  CHECK("track has synth", hasSynth[0]);
  CHECK("one write", patches[0].writeCount == 1);
  CHECK("param", patches[0].writes[0].paramID == synth::param::OSC1_MIX_LEVEL);
  CHECK("value", patches[0].writes[0].value == 0.5f);
  CHECK("track 2 absent", !hasSynth[1]);
}

static void test_multiple_synth_values_emit_sparse_writes() {
  TEST("multiple_synth_values_emit_sparse_writes");

  auto* ws = getParseTestWorkspace();
  auto parsed = parseWS("synth(1, SynthSettings { osc1 = { mixLevel = 0.8 }, "
                        "svf = { cutoff = 1200 } })",
                        ws);
  CHECK("parse ok", parsed.ok);

  app::TrackSynthPatch patches[app::MAX_TRACKS]{};
  bool hasSynth[app::MAX_TRACKS]{};
  auto result = app::doc::buildSynthPatches(&ws->model, 1, 7, patches, hasSynth);

  CHECK("target ok", result.ok);
  CHECK("track has synth", hasSynth[0]);
  CHECK("two writes", patches[0].writeCount == 2);
  const auto* mixLevel = findSynthWrite(patches[0], synth::param::OSC1_MIX_LEVEL);
  const auto* cutoff = findSynthWrite(patches[0], synth::param::SVF_CUTOFF);
  CHECK("mixLevel applied", mixLevel && mixLevel->value == 0.8f);
  CHECK("cutoff applied", cutoff && cutoff->value == 1200.0f);
}

static void test_other_tracks_remain_absent() {
  TEST("other_tracks_remain_absent");

  auto* ws = getParseTestWorkspace();
  auto parsed = parseWS("synth(2, SynthSettings { osc1 = { mixLevel = 0.25 } })", ws);
  CHECK("parse ok", parsed.ok);

  app::TrackSynthPatch patches[app::MAX_TRACKS]{};
  bool hasSynth[app::MAX_TRACKS]{};
  auto result = app::doc::buildSynthPatches(&ws->model, 1, 7, patches, hasSynth);

  CHECK("target ok", result.ok);
  CHECK("track 2 authored", hasSynth[1]);
  CHECK("track 2 value",
        patches[1].writes[0].paramID == synth::param::OSC1_MIX_LEVEL &&
            patches[1].writes[0].value == 0.25f);
  CHECK("track 1 absent", !hasSynth[0]);
  CHECK("track 1 no writes", patches[0].writeCount == 0);
}

void runDocSynthPlannerTests() {
  SUITE("DocSynthPlanner");
  test_empty_document_emits_no_synth_patches();
  test_osc_mix_only_emits_one_synth_write();
  test_multiple_synth_values_emit_sparse_writes();
  test_other_tracks_remain_absent();
}
