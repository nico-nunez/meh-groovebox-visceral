#include "TestHelpers.h"
#include "TestRunner.h"

#include "app/doc/DocGrooveboxPatchBuilder.h"

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

const app::MixerParamPatchWrite* findMixerWrite(const app::MixerPatch& patch,
                                                app::params::AppParamID paramID) {
  for (uint16_t i = 0; i < patch.writeCount; ++i) {
    if (patch.writes[i].paramID == paramID)
      return &patch.writes[i];
  }
  return nullptr;
}

} // namespace

static void test_empty_document_builds_empty_patch() {
  TEST("empty_document_builds_empty_patch");

  auto* ws = getParseTestWorkspace();
  auto parsed = parseWS("", ws);
  CHECK("parse ok", parsed.ok);

  app::GrooveboxPatch patch{};
  auto result = app::doc::buildGrooveboxPatch(&ws->model, 1, 7, &patch);

  CHECK("target ok", result.ok);
  CHECK("no patch edits", !app::hasGrooveboxPatchEdits(patch));
  CHECK("no mixer", !patch.hasMixer);
  CHECK("no sequencer", !patch.hasSequencer);
  for (uint8_t t = 0; t < app::MAX_TRACKS; ++t)
    CHECK("no synth", !patch.hasSynth[t]);
}

static void test_synth_only_doc_marks_only_synth_domain() {
  TEST("synth_only_doc_marks_only_synth_domain");

  auto* ws = getParseTestWorkspace();
  auto parsed =
      parseWS("synth(1, SynthSettings { osc1 = { mixLevel = 0.25 }, svf = { cutoff = 1200 } })",
              ws);
  CHECK("parse ok", parsed.ok);

  app::GrooveboxPatch patch{};
  auto result = app::doc::buildGrooveboxPatch(&ws->model, 1, 7, &patch);

  CHECK("target ok", result.ok);
  CHECK("track 1 synth present", patch.hasSynth[0]);
  CHECK("two synth writes", patch.synth[0].writeCount == 2);
  const auto* mixLevel = findSynthWrite(patch.synth[0], synth::param::OSC1_MIX_LEVEL);
  const auto* cutoff = findSynthWrite(patch.synth[0], synth::param::SVF_CUTOFF);
  CHECK("mixLevel write", mixLevel && mixLevel->value == 0.25f);
  CHECK("cutoff write", cutoff && cutoff->value == 1200.0f);
  CHECK("no mixer", !patch.hasMixer);
  CHECK("no sequencer", !patch.hasSequencer);
}

static void test_mixer_only_doc_marks_only_mixer_domain() {
  TEST("mixer_only_doc_marks_only_mixer_domain");

  auto* ws = getParseTestWorkspace();
  auto parsed = parseWS("mixer(1, MixerSettings { gain = 0.5, pan = -0.25, mute = true })", ws);
  CHECK("parse ok", parsed.ok);

  app::GrooveboxPatch patch{};
  auto result = app::doc::buildGrooveboxPatch(&ws->model, 1, 7, &patch);

  CHECK("target ok", result.ok);
  CHECK("mixer present", patch.hasMixer);
  CHECK("three mixer writes", patch.mixer.writeCount == 3);
  const auto* gain = findMixerWrite(patch.mixer, app::params::AppParamID::TrackGain);
  const auto* pan = findMixerWrite(patch.mixer, app::params::AppParamID::TrackPan);
  const auto* mute = findMixerWrite(patch.mixer, app::params::AppParamID::TrackMute);
  CHECK("gain write", gain && gain->value == 0.5f);
  CHECK("pan write", pan && pan->value == -0.25f);
  CHECK("mute write", mute && mute->value == 1.0f);
  CHECK("no synth", !patch.hasSynth[0]);
  CHECK("no sequencer", !patch.hasSequencer);
}

static void test_sequencer_only_doc_does_not_mark_synth_or_mixer() {
  TEST("sequencer_only_doc_does_not_mark_synth_or_mixer");

  auto* ws = getParseTestWorkspace();
  auto parsed = parseWS("track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { "
                        "notes = { { note = 60 } } } } } } })",
                        ws);
  CHECK("parse ok", parsed.ok);

  app::GrooveboxPatch patch{};
  auto result = app::doc::buildGrooveboxPatch(&ws->model, 1, 7, &patch);

  CHECK("target ok", result.ok);
  CHECK("has sequencer", patch.hasSequencer);
  CHECK("track present", patch.sequencer.hasTrack[0]);
  CHECK("slot present", patch.sequencer.tracks[0].hasSlot[0]);
  CHECK("step note",
        patch.sequencer.tracks[0].slots[0].pattern.steps[0].hasNotePatch[0] &&
            patch.sequencer.tracks[0].slots[0].pattern.steps[0].notes[0].hasNote &&
            patch.sequencer.tracks[0].slots[0].pattern.steps[0].notes[0].note == 60);
  CHECK("no mixer", !patch.hasMixer);
  for (uint8_t t = 0; t < app::MAX_TRACKS; ++t)
    CHECK("no synth", !patch.hasSynth[t]);
}

void runDocGrooveboxTargetBuilderTests() {
  SUITE("DocGrooveboxPatchBuilder");
  test_empty_document_builds_empty_patch();
  test_synth_only_doc_marks_only_synth_domain();
  test_mixer_only_doc_marks_only_mixer_domain();
  test_sequencer_only_doc_does_not_mark_synth_or_mixer();
}
