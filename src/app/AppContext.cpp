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
