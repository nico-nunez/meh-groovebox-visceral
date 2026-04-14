#include "AppContext.h"

#include "app/AudioSession.h"

#include <cstdio>

namespace app {

AppContext* createAppContext(audio::DeviceInfo deviceInfo) {
  AppContext* ctx = new AppContext();

  // default: channel N → track N; channels beyond MAX_TRACKS fall back to currentTrack
  memset(ctx->midiChannelMap, MIDI_CHANNEL_UNASSIGNED, sizeof(ctx->midiChannelMap));
  for (uint8_t i = 0; i < MAX_TRACKS; ++i)
    ctx->midiChannelMap[i] = i;

  // init engines and wire track pointers
  synth::EngineConfig engineConfig{};
  engineConfig.sampleRate = static_cast<float>(deviceInfo.sampleRate);
  engineConfig.numFrames = deviceInfo.bufferFrameSize;

  for (uint8_t i = 0; i < MAX_TRACKS; ++i) {
    ctx->engines[i] = synth::createEngine(engineConfig);
    ctx->tracks[i].engine = &ctx->engines[i];
  }

  // init shared transport
  transport::initTransportRuntime(ctx->transport, deviceInfo.sampleRate, 120.0f);

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
