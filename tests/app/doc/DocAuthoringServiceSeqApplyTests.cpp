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

  auto result = app::doc::applySequencerRevision(app->documents.authoring,
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
      app::doc::applySequencerRevision(app->documents.authoring, *app, 1, kNonEmptyTrack1);

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

static void test_empty_document_replaces_with_default_targets() {
  TEST("empty_document_replaces_with_default_targets");

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);

  auto result = app::doc::applySequencerRevision(app->documents.authoring, *app, 1, "");

  CHECK("ok", result.ok);
  CHECK("pending ready", app->documents.pendingApply.ready.load());

  test::publishPending(app);

  auto bank = app::sequencer::getPatternBank(app->sequencer, 0);
  CHECK("bank readable", bank.ok);
  CHECK("default inactive", bank.value->activeSlot == app::sequencer::INVALID_PATTERN_SLOT);
  CHECK("default empty", !bank.value->slots[0].occupied);

  app::destroyAppContext(app);
}

static void test_mixed_synth_mixer_sequencer_apply_publishes_together() {
  TEST("mixed_synth_mixer_sequencer_apply_publishes_together");

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);

  std::string doc = "mixer(1, MixerSettings { gain = 0.7 })\n"
                    "synth(1, SynthSettings { osc1 = { mix = 0.5 } })\n";
  doc += kNonEmptyTrack1;

  auto result = app::doc::applySequencerRevision(app->documents.authoring, *app, 1, doc.c_str());

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
  test_empty_document_replaces_with_default_targets();
  test_mixed_synth_mixer_sequencer_apply_publishes_together();
}
