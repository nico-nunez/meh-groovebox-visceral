#include "AudioSession.h"

#include "app/AppContext.h"
#include "app/BlockScheduler.h"
#include "app/Constants.h"
#include "app/Transport.h"

#include "audio_io/AudioIO.h"
#include "audio_io/AudioIOTypes.h"
#include "audio_io/AudioIOTypesFwd.h"

#include "dsp/Buffers.h"
#include "dsp/Dynamics.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>

namespace app::audio {
using audio_io::AudioBuffer;
using audio_io::hAudioSession;

namespace {

void renderTrackToBuffer(track::TrackState& track, uint32_t numFrames, float bpm) {
  float* channelPtrs[2] = {
      track.outputBuffer.left,
      track.outputBuffer.right,
  };

  track.engine.processAudioBlock(channelPtrs, 2, numFrames, bpm);
}

void sumTrackToMaster(const track::TrackState& track,
                      const mixer::TrackMixState& mixState,
                      mixer::MasterBusState& master,
                      uint32_t numFrames) {
  if (!mixState.enabled)
    return;

  for (uint32_t i = 0; i < numFrames; ++i)
    master.busBuffer.left[i] += track.outputBuffer.left[i] * mixState.gain;

  for (uint32_t i = 0; i < numFrames; ++i)
    master.busBuffer.right[i] += track.outputBuffer.right[i] * mixState.gain;
}

void applyMasterGain(mixer::MasterBusState& master, float gain, uint32_t numFrames) {
  for (uint32_t i = 0; i < numFrames; ++i)
    master.busBuffer.left[i] *= gain;

  for (uint32_t i = 0; i < numFrames; ++i)
    master.busBuffer.right[i] *= gain;
}

void writeMasterToDevice(const mixer::MasterBusState& master, audio_io::AudioBuffer buffer) {
  const uint32_t frames = buffer.numFrames;
  const uint32_t channels = buffer.numChannels;

  if (channels == 0)
    return;

  for (uint32_t i = 0; i < frames; ++i)
    buffer.channelPtrs[0][i] = master.busBuffer.left[i];

  if (channels >= 2) {
    for (uint32_t i = 0; i < frames; ++i)
      buffer.channelPtrs[1][i] = master.busBuffer.right[i];
  }

  for (uint32_t ch = 2; ch < channels; ++ch)
    std::fill_n(buffer.channelPtrs[ch], frames, 0.0f);
}

void audioCallback(audio_io::AudioBuffer buffer, void* context) {
  auto* ctx = static_cast<AppContext*>(context);

  // 1. Admit transport actions
  auto previousMode = ctx->transport.mode;

  transport::TransportEvent transEvt;
  while (ctx->transportQueue.pop(transEvt))
    transport::applyTransportEvent(ctx->transport, transEvt);

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

    synth::EngineEvent engEvt;
    while (track.queues.engine.pop(engEvt))
      track.engine.processEngineEvent(engEvt);
  }

  // 3. render and mix
  auto& masterBus = ctx->masterBus;
  auto& mixer = ctx->mixer;

  dsp::buffers::clearStereoBuffer(masterBus.busBuffer);
  for (uint8_t i = 0; i < MAX_TRACKS; i++) {
    auto& currentTrack = ctx->tracks[i];

    dsp::buffers::clearStereoBuffer(currentTrack.outputBuffer);
    renderTrackToBuffer(currentTrack, buffer.numFrames, ctx->transport.bpm);
    sumTrackToMaster(currentTrack, mixer.tracks[i], masterBus, buffer.numFrames);
  }

  applyMasterGain(masterBus, mixer.masterGain, buffer.numFrames);

  dsp::dynamics::processPeakLimiter(masterBus.limiter,
                                    masterBus.busBuffer,
                                    buffer.numFrames,
                                    mixer.limiterThreshold);

  writeMasterToDevice(masterBus, buffer);
}

} // namespace

DeviceInfo queryDefaultDevice() {
  auto info = audio_io::queryDefaultDevice();
  return {info.sampleRate, info.bufferFrameSize, info.numChannels};
}

hAudioSession initSession(const DeviceInfo& deviceInfo, app::AppContext* ctx) {
  assert(deviceInfo.bufferFrameSize <= audio::MAX_BLOCK_FRAMES);

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
