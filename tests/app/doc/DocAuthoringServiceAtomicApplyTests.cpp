#include "TestHelpers.h"
#include "TestRunner.h"

#include "app/AppContext.h"
#include "app/GrooveboxEditSession.h"
#include "app/doc/DocAuthoringService.h"
#include "app/sessions/AudioSession.h"

#include "synth/params/ParamDefs.h"

static void test_doc_apply_prepares_but_does_not_publish_until_audio_boundary() {
  TEST("doc_apply_prepares_but_does_not_publish_until_audio_boundary");

  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);

  const char* doc = "synth(1, SynthSettings { osc1 = { mixLevel = 0.25 } })";
  auto result = app::doc::submitAuthoredDocRevision(app->documents.authoring, *app, 1, doc);

  CHECK("apply accepted", result.ok);
  CHECK("pending ready", app->documents.pendingApply.ready.load());
  CHECK("not yet audible", app->tracks[0].engine.params[synth::param::OSC1_MIX_LEVEL] != 0.25f);

  test::publishPending(app);

  CHECK("pending cleared", !app->documents.pendingApply.ready.load());
  CHECK("synth published", app->tracks[0].engine.params[synth::param::OSC1_MIX_LEVEL] == 0.25f);

  test::destroyAppContext(app);
}

static void test_doc_apply_publishes_mixer_and_synth_together() {
  TEST("doc_apply_publishes_mixer_and_synth_together");

  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);

  const char* doc = "synth(1, SynthSettings { osc1 = { mixLevel = 0.5 } }) "
                    "mixer(1, MixerSettings { gain = 0.25 })";
  auto result = app::doc::submitAuthoredDocRevision(app->documents.authoring, *app, 1, doc);

  CHECK("apply accepted", result.ok);
  CHECK("synth old before publish",
        app->tracks[0].engine.params[synth::param::OSC1_MIX_LEVEL] != 0.5f);
  CHECK("mixer old before publish", app->mixer.current.tracks[0].gain == 1.0f);

  test::publishPending(app);

  CHECK("synth published", app->tracks[0].engine.params[synth::param::OSC1_MIX_LEVEL] == 0.5f);
  CHECK("mixer published", app->mixer.current.tracks[0].gain == 0.25f);

  test::destroyAppContext(app);
}

static void test_second_doc_apply_rejected_while_pending_unpublished() {
  TEST("second_doc_apply_rejected_while_pending_unpublished");

  app::AppContext* app = test::makeAppContext();
  CHECK("context", app != nullptr);

  auto first =
      app::doc::submitAuthoredDocRevision(app->documents.authoring,
                                          *app,
                                          1,
                                          "synth(1, SynthSettings { osc1 = { mixLevel = 0.5 } })");
  auto second =
      app::doc::submitAuthoredDocRevision(app->documents.authoring,
                                          *app,
                                          2,
                                          "synth(1, SynthSettings { osc1 = { mixLevel = 0.25 } })");

  CHECK("first accepted", first.ok);
  CHECK("second rejected", !second.ok);
  CHECK("pending still ready", app->documents.pendingApply.ready.load());

  test::publishPending(app);
  test::destroyAppContext(app);
}

void runDocAuthoringServiceAtomicApplyTests() {
  SUITE("DocAuthoringServiceAtomicApply");
  test_doc_apply_prepares_but_does_not_publish_until_audio_boundary();
  test_doc_apply_publishes_mixer_and_synth_together();
  test_second_doc_apply_rejected_while_pending_unpublished();
}
