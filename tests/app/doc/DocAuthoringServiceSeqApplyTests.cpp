#include "TestHelpers.h"
#include "TestRunner.h"

#include "app/AppContext.h"
#include "app/GrooveboxEditSession.h"
#include "app/Sequencer.h"
#include "app/doc/DocAuthoringService.h"

namespace {

using test::hasDiagnostic;

const char* kNonEmptyTrack1 =
    "track(1, TrackSettings { patterns = { [1] = { numSteps = 1, stepsPerBeat = 4, "
    "steps = { { active = true, note = 60, velocity = 100 } } } }, activeSlot = 1 })";

app::AppContext* makeContext() {
  app::audio::DeviceInfo device{};
  device.sampleRate = 48000;
  device.bufferFrameSize = 64;
  device.numChannels = 2;
  return app::createAppContext(device);
}

} // namespace

static void test_parse_failure_sets_failed_without_admission() {
  TEST("parse_failure_sets_failed_without_admission");

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);

  auto result = app::doc::submitAuthoredDocRevision(app->documents.authoring,
                                                    *app,
                                                    1,
                                                    "track('one', TrackSettings {})");

  CHECK("not ok", !result.ok);
  CHECK("status Failed", app->documents.authoring.apply.status == app::doc::ApplyStatus::Failed);
  CHECK("diagnostic sequencer.track.invalid_index",
        hasDiagnostic(result.diagnostics, "sequencer.track.invalid_index"));
  CHECK("no admitted model", !app->documents.authoring.apply.hasLastAdmittedDocModel);
  CHECK("no pending apply", !app->documents.pendingApply.ready.load());

  app::destroyAppContext(app);
}

static void test_successful_sequencer_apply_publishes_at_boundary() {
  TEST("successful_sequencer_apply_publishes_at_boundary");

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);

  auto result =
      app::doc::submitAuthoredDocRevision(app->documents.authoring, *app, 1, kNonEmptyTrack1);

  CHECK("ok", result.ok);
  CHECK("status Completed",
        app->documents.authoring.apply.status == app::doc::ApplyStatus::Completed);
  CHECK("lastAdmittedRevision == 1", app->documents.authoring.buffer.lastAdmittedRevision == 1);
  CHECK("pending ready", app->documents.pendingApply.ready.load());

  auto pre = app::sequencer::getPatternBank(app->sequencer, 0);
  CHECK("pre bank readable", pre.ok);
  CHECK("not published yet", !pre.value->slots[0].occupied);

  test::publishPending(app);

  auto post = app::sequencer::getPatternBank(app->sequencer, 0);
  CHECK("post bank readable", post.ok);
  CHECK("published slot occupied", post.value->slots[0].occupied);
  CHECK("published active slot", post.value->activeSlot == 0);

  app::destroyAppContext(app);
}

static void test_empty_document_is_noop() {
  TEST("empty_document_is_noop");

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);

  auto seed =
      app::doc::submitAuthoredDocRevision(app->documents.authoring, *app, 1, kNonEmptyTrack1);
  CHECK("seed ok", seed.ok);
  test::publishPending(app);

  auto result = app::doc::submitAuthoredDocRevision(app->documents.authoring, *app, 2, "");

  CHECK("ok", result.ok);
  CHECK("no pending apply", !app->documents.pendingApply.ready.load());

  auto bank = app::sequencer::getPatternBank(app->sequencer, 0);
  CHECK("bank readable", bank.ok);
  CHECK("slot preserved", bank.value->slots[0].occupied);
  CHECK("note preserved", bank.value->slots[0].pattern.steps[0].note == 60);

  app::destroyAppContext(app);
}

static void test_sparse_note_patch_preserves_step_velocity_gate_and_active() {
  TEST("sparse_note_patch_preserves_step_velocity_gate_and_active");

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);

  auto seed = app::doc::submitAuthoredDocRevision(
      app->documents.authoring,
      *app,
      1,
      "track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { active = true, "
      "note = 60, velocity = 100, gate = 0.4 } } } }, activeSlot = 1 })");
  CHECK("seed ok", seed.ok);
  test::publishPending(app);

  auto patch = app::doc::submitAuthoredDocRevision(
      app->documents.authoring,
      *app,
      2,
      "track(1, TrackSettings { patterns = { [1] = { steps = { [1] = { note = 64 } } } } })");
  CHECK("patch ok", patch.ok);
  test::publishPending(app);

  auto bank = app::sequencer::getPatternBank(app->sequencer, 0);
  CHECK("bank readable", bank.ok);
  const auto& step = bank.value->slots[0].pattern.steps[0];
  CHECK("note changed", step.note == 64);
  CHECK("active preserved", step.active);
  CHECK("velocity preserved", step.velocity == 100);
  CHECK("gate preserved", step.gate == 0.4f);

  app::destroyAppContext(app);
}

static void test_step_false_clears_step_at_admission() {
  TEST("step_false_clears_step_at_admission");

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);

  auto seed =
      app::doc::submitAuthoredDocRevision(app->documents.authoring, *app, 1, kNonEmptyTrack1);
  CHECK("seed ok", seed.ok);
  test::publishPending(app);

  auto clear = app::doc::submitAuthoredDocRevision(
      app->documents.authoring,
      *app,
      2,
      "track(1, TrackSettings { patterns = { [1] = { steps = { [1] = false } } } })");
  CHECK("clear ok", clear.ok);
  test::publishPending(app);

  auto bank = app::sequencer::getPatternBank(app->sequencer, 0);
  CHECK("bank readable", bank.ok);
  const auto& step = bank.value->slots[0].pattern.steps[0];
  CHECK("inactive", !step.active);
  CHECK("note reset", step.note == 0);
  CHECK("velocity reset", step.velocity == 0);

  app::destroyAppContext(app);
}

static void test_mixed_synth_mixer_sequencer_apply_publishes_together() {
  TEST("mixed_synth_mixer_sequencer_apply_publishes_together");

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);

  std::string doc = "mixer(1, MixerSettings { gain = 0.7 })\n"
                    "synth(1, SynthSettings { osc1 = { mix = 0.5 } })\n";
  doc += kNonEmptyTrack1;

  auto result = app::doc::submitAuthoredDocRevision(app->documents.authoring, *app, 1, doc.c_str());

  CHECK("ok", result.ok);
  CHECK("pending ready", app->documents.pendingApply.ready.load());
  CHECK("synth old", app->tracks[0].engine.params[synth::param::OSC1_MIX_LEVEL] != 0.5f);
  CHECK("mixer old", app->mixer.current.tracks[0].gain == 1.0f);
  auto pre = app::sequencer::getPatternBank(app->sequencer, 0);
  CHECK("seq old", pre.ok && !pre.value->slots[0].occupied);

  test::publishPending(app);

  CHECK("synth published", app->tracks[0].engine.params[synth::param::OSC1_MIX_LEVEL] == 0.5f);
  CHECK("mixer published", app->mixer.current.tracks[0].gain == 0.7f);
  auto post = app::sequencer::getPatternBank(app->sequencer, 0);
  CHECK("seq published", post.ok && post.value->slots[0].occupied);

  app::destroyAppContext(app);
}

void runDocAuthoringServiceSeqApplyTests() {
  SUITE("DocAuthoringService / SequencerApply");
  test_parse_failure_sets_failed_without_admission();
  test_successful_sequencer_apply_publishes_at_boundary();
  test_empty_document_is_noop();
  test_sparse_note_patch_preserves_step_velocity_gate_and_active();
  test_step_false_clears_step_at_admission();
  test_mixed_synth_mixer_sequencer_apply_publishes_together();
}
