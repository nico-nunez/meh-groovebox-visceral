#include "AppContext.h"

#include "app/sessions/AudioSession.h"

#include "synth/params/ParamUtils.h"

#include <cstdio>

namespace app {

namespace {
float getParamCallback(uint8_t id, void* ctx) {
  auto paramID = static_cast<synth::param::ParamID>(id);
  auto* engPtr = static_cast<synth::Engine*>(ctx);

  return synth::param::utils::getParamValueByID(engPtr, paramID);
}

} // namespace
AppContext* createAppContext(audio::DeviceInfo deviceInfo) {
  AppContext* ctx = new AppContext();

  // default: channel N → track N; channels beyond MAX_TRACKS fall back to currentTrack
  memset(ctx->midiChannelMap, MIDI_CHANNEL_UNASSIGNED, sizeof(ctx->midiChannelMap));
  for (uint8_t i = 0; i < MAX_TRACKS; i++)
    ctx->midiChannelMap[i] = i;

  // init engines and wire track pointers
  synth::EngineConfig engineConfig{};
  engineConfig.sampleRate = static_cast<float>(deviceInfo.sampleRate);
  engineConfig.numFrames = deviceInfo.bufferFrameSize;

  for (uint8_t i = 0; i < MAX_TRACKS; i++) {
    ctx->tracks[i].engine = synth::createEngine(engineConfig);
  }

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

  auto& lane = ctx->sequencer.store.buffers[0].lanes[0];
  lane.numSteps = 16;
  lane.stepsPerBeat = 4;

  // Kick on every beat (steps 0, 4, 8, 12)
  lane.steps[0].active = true;
  lane.steps[0].noteOn = true;
  lane.steps[0].note = 36;
  lane.steps[0].velocity = 100;

  lane.steps[4].active = true;
  lane.steps[4].noteOn = true;
  lane.steps[4].note = 36;
  lane.steps[4].velocity = 100;

  lane.steps[8].active = true;
  lane.steps[8].noteOn = true;
  lane.steps[8].note = 36;
  lane.steps[8].velocity = 100;

  lane.steps[12].active = true;
  lane.steps[12].noteOn = true;
  lane.steps[12].note = 36;
  lane.steps[12].velocity = 100;

  // Snare on beats 2 and 4 (steps 4, 12) — or try a different note

  lane.steps[2].active = true;
  lane.steps[2].noteOn = true;
  lane.steps[2].note = 48;
  lane.steps[2].velocity = 80;

  lane.steps[6].active = true;
  lane.steps[6].noteOn = true;
  lane.steps[6].note = 48;
  lane.steps[6].velocity = 80;

  lane.steps[10].active = true;
  lane.steps[10].noteOn = true;
  lane.steps[10].note = 48;
  lane.steps[10].velocity = 80;

  lane.steps[14].active = true;
  lane.steps[14].noteOn = true;
  lane.steps[14].note = 48;
  lane.steps[14].velocity = 80;

  ctx->sequencer.store.setReadIndex(0);

  return ctx;
}

void destroyAppContext(AppContext* ctx) {
  if (!ctx) {
    printf("AppContext is null");
    return;
  }

  delete ctx;
}

} // namespace app
