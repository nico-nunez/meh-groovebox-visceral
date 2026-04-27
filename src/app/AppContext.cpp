#include "AppContext.h"

#include "app/AppParams.h"
#include "app/Constants.h"
#include "app/sessions/AudioSession.h"

#include "dsp/Dynamics.h"
#include "synth/params/ParamUtils.h"

#include <cassert>
#include <cstdio>

namespace app {

namespace {
float getParamCallback(uint8_t id, void* ctx) {
  auto paramID = static_cast<synth::param::ParamID>(id);
  auto* engPtr = static_cast<synth::Engine*>(ctx);

  return synth::param::utils::getParamValueByID(engPtr, paramID);
}

bool initAudioBuffers(AppContext* ctx) {
  using namespace dsp::buffers;
  const uint32_t numPoolSlots = MAX_TRACKS + 1;
  if (!initStereoBufferPool(ctx->renderBufferPool, numPoolSlots, audio::MAX_BLOCK_FRAMES))
    return false;

  for (uint8_t i = 0; i < MAX_TRACKS; ++i) {
    int32_t slot = acquireStereoBufferSlot(ctx->renderBufferPool);
    assert(slot >= 0);
    ctx->tracks[i].outputSlot = static_cast<uint32_t>(slot);
    ctx->tracks[i].outputBuffer =
        getStereoBufferView(ctx->renderBufferPool, ctx->tracks[i].outputSlot);
  }

  int32_t slot = acquireStereoBufferSlot(ctx->renderBufferPool);
  assert(slot >= 0);
  ctx->masterBus.busBufferSlot = static_cast<uint32_t>(slot);
  ctx->masterBus.busBuffer =
      getStereoBufferView(ctx->renderBufferPool, ctx->masterBus.busBufferSlot);

  return true;
}

} // namespace

AppContext* createAppContext(audio::DeviceInfo deviceInfo) {
  AppContext* ctx = new AppContext();

  // default: MIDI follows currently selected track
  memset(ctx->midiChannelMap, MIDI_CHANNEL_UNASSIGNED, sizeof(ctx->midiChannelMap));
  ctx->midiStickyTrack = MIDI_CHANNEL_UNASSIGNED;

  // default: channel N → track N; channels beyond MAX_TRACKS fall back to currentTrack
  // memset(ctx->midiChannelMap, MIDI_CHANNEL_UNASSIGNED, sizeof(ctx->midiChannelMap));
  // for (uint8_t i = 0; i < MAX_TRACKS; i++)
  //   ctx->midiChannelMap[i] = i;

  // init engines and wire track pointers
  synth::EngineConfig engineConfig{};
  engineConfig.sampleRate = static_cast<float>(deviceInfo.sampleRate);
  engineConfig.numFrames = deviceInfo.bufferFrameSize;

  for (uint8_t i = 0; i < MAX_TRACKS; i++) {
    ctx->tracks[i].engine = synth::createEngine(engineConfig);
  }

  if (!initAudioBuffers(ctx)) {
    destroyAppContext(ctx);
    return nullptr;
  }

  if (!app::params::initAppParams(ctx).ok) {
    destroyAppContext(ctx);
    return nullptr;
  }

  // init master bus limiter
  dsp::dynamics::initPeakLimiter(ctx->masterBus.limiter,
                                 static_cast<float>(deviceInfo.sampleRate),
                                 1.0f,  // attack ms
                                 100.0f // release ms
  );

  // init shared transport
  transport::initTransport(ctx->transport, deviceInfo.sampleRate, 120.0f);

  // init sequencer
  for (uint8_t i = 0; i < MAX_TRACKS; i++) {
    ctx->sequencer.laneCtxs[i].getParamCallback = getParamCallback;
    ctx->sequencer.laneCtxs[i].getParamCtx = &ctx->tracks[i].engine;
    ctx->sequencer.store.buffers[0].lanes[i].numSteps = 16;
    ctx->sequencer.store.buffers[0].lanes[i].stepsPerBeat = 4;
  }
  ctx->sequencer.numLanes = MAX_TRACKS;

  auto& lane0 = ctx->sequencer.store.buffers[0].lanes[0];

  // Kick on every beat (steps 0, 4, 8, 12)
  lane0.steps[0].active = true;
  lane0.steps[0].noteOn = true;
  lane0.steps[0].note = 36;
  lane0.steps[0].velocity = 100;

  lane0.steps[4].active = true;
  lane0.steps[4].noteOn = true;
  lane0.steps[4].note = 36;
  lane0.steps[4].velocity = 100;

  lane0.steps[8].active = true;
  lane0.steps[8].noteOn = true;
  lane0.steps[8].note = 36;
  lane0.steps[8].velocity = 100;

  lane0.steps[12].active = true;
  lane0.steps[12].noteOn = true;
  lane0.steps[12].note = 36;
  lane0.steps[12].velocity = 100;

  // Snare on beats 2 and 4 (steps 4, 12) — or try a different note

  lane0.steps[2].active = true;
  lane0.steps[2].noteOn = true;
  lane0.steps[2].note = 48;
  lane0.steps[2].velocity = 80;

  lane0.steps[6].active = true;
  lane0.steps[6].noteOn = true;
  lane0.steps[6].note = 48;
  lane0.steps[6].velocity = 80;

  lane0.steps[10].active = true;
  lane0.steps[10].noteOn = true;
  lane0.steps[10].note = 48;
  lane0.steps[10].velocity = 80;

  lane0.steps[14].active = true;
  lane0.steps[14].noteOn = true;
  lane0.steps[14].note = 48;
  lane0.steps[14].velocity = 80;

  auto& lane1 = ctx->sequencer.store.buffers[0].lanes[1];
  lane1.numSteps = 16;
  lane1.stepsPerBeat = 4;

  // Kick on every beat (steps 0, 4, 8, 12)
  lane1.steps[0].active = true;
  lane1.steps[0].noteOn = true;
  lane1.steps[0].note = 64;
  lane1.steps[0].velocity = 80;

  lane1.steps[4].active = true;
  lane1.steps[4].noteOn = true;
  lane1.steps[4].note = 67;
  lane1.steps[4].velocity = 80;

  lane1.steps[8].active = true;
  lane1.steps[8].noteOn = true;
  lane1.steps[8].note = 71;
  lane1.steps[8].velocity = 80;

  lane1.steps[12].active = true;
  lane1.steps[12].noteOn = true;
  lane1.steps[12].note = 67;
  lane1.steps[12].velocity = 80;

  auto& lane2 = ctx->sequencer.store.buffers[0].lanes[2];
  lane2.numSteps = 16;
  lane2.stepsPerBeat = 4;

  // Kick on every beat (steps 0, 4, 8, 12)
  lane2.steps[0].active = true;
  lane2.steps[0].noteOn = true;
  lane2.steps[0].note = 64;
  lane2.steps[0].velocity = 80;
  lane2.steps[1].active = true;
  lane2.steps[1].noteOn = true;
  lane2.steps[1].note = 64;
  lane2.steps[1].velocity = 80;
  lane2.steps[2].active = true;
  lane2.steps[2].noteOn = true;
  lane2.steps[2].note = 64;
  lane2.steps[2].velocity = 80;
  lane2.steps[3].active = true;
  lane2.steps[3].noteOn = true;
  lane2.steps[3].note = 64;
  lane2.steps[3].velocity = 80;
  lane2.steps[4].active = true;
  lane2.steps[4].noteOn = true;
  lane2.steps[4].note = 67;
  lane2.steps[4].velocity = 80;
  lane2.steps[5].active = true;
  lane2.steps[5].noteOn = true;
  lane2.steps[5].note = 67;
  lane2.steps[5].velocity = 80;
  lane2.steps[6].active = true;
  lane2.steps[6].noteOn = true;
  lane2.steps[6].note = 67;
  lane2.steps[6].velocity = 80;
  lane2.steps[7].active = true;
  lane2.steps[7].noteOn = true;
  lane2.steps[7].note = 67;
  lane2.steps[7].velocity = 80;
  lane2.steps[8].active = true;
  lane2.steps[8].noteOn = true;
  lane2.steps[8].note = 67;
  lane2.steps[8].velocity = 80;
  lane2.steps[9].active = true;
  lane2.steps[9].noteOn = true;
  lane2.steps[9].note = 67;
  lane2.steps[9].velocity = 80;
  lane2.steps[10].active = true;
  lane2.steps[10].noteOn = true;
  lane2.steps[10].note = 67;
  lane2.steps[10].velocity = 80;
  lane2.steps[11].active = true;
  lane2.steps[11].noteOn = true;
  lane2.steps[11].note = 67;
  lane2.steps[11].velocity = 80;
  lane2.steps[12].active = true;
  lane2.steps[12].noteOn = true;
  lane2.steps[12].note = 67;
  lane2.steps[12].velocity = 80;
  lane2.steps[13].active = true;
  lane2.steps[13].noteOn = true;
  lane2.steps[13].note = 67;
  lane2.steps[13].velocity = 80;
  lane2.steps[14].active = true;
  lane2.steps[14].noteOn = true;
  lane2.steps[14].note = 67;
  lane2.steps[14].velocity = 80;
  lane2.steps[15].active = true;
  lane2.steps[15].noteOn = true;
  lane2.steps[15].note = 67;
  lane2.steps[15].velocity = 80;

  ctx->sequencer.store.setReadIndex(0);

  return ctx;
}

void destroyAppContext(AppContext* ctx) {
  if (!ctx) {
    printf("AppContext is null");
    return;
  }

  dsp::buffers::destroyStereoBufferPool(ctx->renderBufferPool);
  delete ctx;
}

} // namespace app
