#include "TestHelpers.h"
#include "TestRunner.h"

#include "app/AppContext.h"
#include "app/GrooveboxEditSession.h"
#include "app/Transport.h"
#include "app/doc/DocAuthoringService.h"
#include "app/doc/DocGrooveboxPatchBuilder.h"
#include "app/doc/DocSequencerParser.h"
#include "app/sessions/AudioSession.h"

#include "synth/params/ParamDefs.h"

namespace {

app::AppContext* makeContext() {
  app::audio::DeviceInfo device{};
  device.sampleRate = 48000;
  device.bufferFrameSize = 64;
  device.numChannels = 2;
  return app::createAppContext(device);
}

app::transport::TransportBlockInfo block(app::transport::TransportMode mode, uint32_t flags) {
  app::transport::TransportBlockInfo info{};
  info.mode = mode;
  info.boundaryFlags = flags;
  info.numFrames = 64;
  info.bpm = 120.0f;
  return info;
}

bool prepareDoc(app::AppContext* app, const char* doc, app::GrooveboxApplyTiming timing) {
  auto* ws = test::getParseTestWorkspace();
  auto parsed = test::parseWorkspace(1, 1, doc, ws);
  if (!parsed.ok)
    return false;

  app::GrooveboxPatch* patch = &app->documents.authoring.applyWorkspace->patch;
  auto build = app::doc::buildGrooveboxPatch(&ws->model, 1, 1, patch);
  if (!build.ok)
    return false;

  app::GrooveboxEditSession session{};
  app::beginGrooveboxEdit(&session, 1);
  app::stageGrooveboxPatch(&session, patch);
  app::doc::DocDiagnostics diagnostics{};
  auto edit = app::commitGrooveboxEdit(&session, app, timing, &diagnostics);
  return edit.ok;
}

} // namespace

static void test_next_bar_holds_until_bar_boundary() {
  TEST("next_bar_holds_until_bar_boundary");

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);

  bool prepared = prepareDoc(app,
                             "synth(1, SynthSettings { osc1 = { mix = 0.25 } })",
                             app::GrooveboxApplyTiming::NextBar);
  CHECK("prepared", prepared);

  app::publishPendingGrooveboxEditIfReady(app, block(app::transport::TransportMode::Playing, 0));
  CHECK("still pending", app->documents.pendingApply.ready.load());
  CHECK("not published", app->tracks[0].engine.params[synth::param::OSC1_MIX_LEVEL] != 0.25f);

  app::publishPendingGrooveboxEditIfReady(app,
                                          block(app::transport::TransportMode::Playing,
                                                static_cast<uint32_t>(
                                                    app::transport::BoundaryFlags::CrossedBar)));
  CHECK("published", !app->documents.pendingApply.ready.load());
  CHECK("param applied", app->tracks[0].engine.params[synth::param::OSC1_MIX_LEVEL] == 0.25f);

  app::destroyAppContext(app);
}

static void test_next_beat_holds_until_beat_boundary() {
  TEST("next_beat_holds_until_beat_boundary");

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);

  bool prepared = prepareDoc(app,
                             "synth(1, SynthSettings { osc1 = { mix = 0.5 } })",
                             app::GrooveboxApplyTiming::NextBeat);
  CHECK("prepared", prepared);

  app::publishPendingGrooveboxEditIfReady(app, block(app::transport::TransportMode::Playing, 0));
  CHECK("still pending", app->documents.pendingApply.ready.load());

  app::publishPendingGrooveboxEditIfReady(app,
                                          block(app::transport::TransportMode::Playing,
                                                static_cast<uint32_t>(
                                                    app::transport::BoundaryFlags::CrossedBeat)));
  CHECK("published", !app->documents.pendingApply.ready.load());
  CHECK("param applied", app->tracks[0].engine.params[synth::param::OSC1_MIX_LEVEL] == 0.5f);

  app::destroyAppContext(app);
}

static void test_quantized_apply_publishes_immediately_when_stopped() {
  TEST("quantized_apply_publishes_immediately_when_stopped");

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);

  bool prepared = prepareDoc(app,
                             "synth(1, SynthSettings { osc1 = { mix = 0.75 } })",
                             app::GrooveboxApplyTiming::NextBar);
  CHECK("prepared", prepared);

  app::publishPendingGrooveboxEditIfReady(app, block(app::transport::TransportMode::Stopped, 0));

  CHECK("published", !app->documents.pendingApply.ready.load());
  CHECK("param applied", app->tracks[0].engine.params[synth::param::OSC1_MIX_LEVEL] == 0.75f);

  app::destroyAppContext(app);
}

static void test_quantized_apply_holds_while_paused() {
  TEST("quantized_apply_holds_while_paused");

  app::AppContext* app = makeContext();
  CHECK("context", app != nullptr);

  bool prepared = prepareDoc(app,
                             "synth(1, SynthSettings { osc1 = { mix = 0.33 } })",
                             app::GrooveboxApplyTiming::NextBar);
  CHECK("prepared", prepared);

  app::publishPendingGrooveboxEditIfReady(app,
                                          block(app::transport::TransportMode::Paused,
                                                static_cast<uint32_t>(
                                                    app::transport::BoundaryFlags::CrossedBar)));

  CHECK("still pending", app->documents.pendingApply.ready.load());
  CHECK("not published", app->tracks[0].engine.params[synth::param::OSC1_MIX_LEVEL] != 0.33f);

  app::destroyAppContext(app);
}

void runDocAuthoringServiceQuantizedApplyTests() {
  SUITE("DocAuthoringServiceQuantizedApply");
  test_next_bar_holds_until_bar_boundary();
  test_next_beat_holds_until_beat_boundary();
  test_quantized_apply_publishes_immediately_when_stopped();
  test_quantized_apply_holds_while_paused();
}
