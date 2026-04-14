#include "AudioSession.h"

#include "AppContext.h"

#include "app/BlockScheduler.h"
#include "app/Transport.h"
#include "audio_io/AudioIO.h"
#include "audio_io/AudioIOTypes.h"
#include "audio_io/AudioIOTypesFwd.h"

#include <cassert>
#include <cstdio>

namespace app::audio {
using audio_io::AudioBuffer;
using audio_io::hAudioSession;

namespace {
void audioCallback(audio_io::AudioBuffer buffer, void* context) {
  auto* ctx = static_cast<AppContext*>(context);

  // 1. Admit transport actions
  auto previousMode = ctx->transport.mode;

  transport::TransportEvent evt;
  while (ctx->transportQueue.pop(evt))
    transport::applyTransportEvent(ctx->transport, evt);

  auto blockInfo = transport::advanceTransportBlock(ctx->transport, previousMode, buffer.numFrames);
  runBlockScheduler(ctx, blockInfo);

  // 2. Drain queues for all tracks
  for (int i = 0; i < MAX_TRACKS; i++) {
    auto& track = ctx->tracks[i];

    synth::MIDIEvent midi;
    while (track.queues.midi.pop(midi))
      track.engine.processMIDIEvent(midi);

    synth::ParamEvent param;
    while (track.queues.param.pop(param))
      track.engine.processParamEvent(param);

    synth::EngineEvent evt;
    while (track.queues.engine.pop(evt))
      track.engine.processEngineEvent(evt);
  }

  // 3. TODO: render mixed tracks NOT just the current one
  ctx->tracks[ctx->currentTrack].engine.processAudioBlock(buffer.channelPtrs,
                                                          buffer.numChannels,
                                                          buffer.numFrames,
                                                          ctx->transport.bpm);
}

} // namespace

DeviceInfo queryDefaultDevice() {
  auto info = audio_io::queryDefaultDevice();
  return {info.sampleRate, info.bufferFrameSize, info.numChannels};
}

hAudioSession initSession(const DeviceInfo& deviceInfo, app::AppContext* ctx) {
  audio_io::Config audioConfig{};
  audioConfig.sampleRate = deviceInfo.sampleRate;
  audioConfig.numFrames = deviceInfo.bufferFrameSize;
  audioConfig.numChannels = deviceInfo.numChannels;

  return audio_io::setupAudioSession(audioConfig, audioCallback, ctx);
}

int startSession(hAudioSession sessionPtr) {
  return audio_io::startAudioSession(sessionPtr);
}

int stopSession(hAudioSession sessionPtr) {
  return audio_io::stopAudioSession(sessionPtr);
}

int disposeSession(hAudioSession sessionPtr) {
  int status = audio_io::cleanupAudioSession(sessionPtr);
  if (status != 0) {
    printf("Unable to cleanup Audio Session");
    return 1;
  }
  return 0;
}

} // namespace app::audio
