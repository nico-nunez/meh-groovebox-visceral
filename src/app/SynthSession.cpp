#include "app/SynthSession.h"

#include "audio_io/AudioIO.h"
#include "audio_io/AudioIOTypes.h"
#include "audio_io/AudioIOTypesFwd.h"

#include <cstdint>
#include <cstdio>

namespace app::session {
using AudioBuffer = audio_io::AudioBuffer;
using hAudioSession = audio_io::hAudioSession;

// ==========================
// Audio Device Negotiation
// ==========================
DeviceInfo queryDefaultDevice() {
  auto info = audio_io::queryDefaultDevice();
  return {info.sampleRate, info.bufferFrameSize, info.numChannels};
}

// =============================
// Synth Session Initialization
// =============================
// TODO: use handles instead of actual queues
struct SynthSession {
  MIDIEventQueue midiEventQueue{};
  ParamEventQueue paramEventQueue{};

  AudioBufferHandler processAudioBlock;

  MIDIEventHandler processMIDIEvent;
  ParamEventHandler processParamEvent;

  hAudioSession audioSession;
  void* userContext;
};
using hSynthSession = SynthSession*;

static void audioCallback(AudioBuffer buffer, void* context) {
  auto* ctx = static_cast<SynthSession*>(context);

  // Drain MIDI Events
  if (ctx->processMIDIEvent) {
    MIDIEvent midiEvent;
    while (ctx->midiEventQueue.pop(midiEvent)) {
      ctx->processMIDIEvent(midiEvent, ctx->userContext);
    }
  }

  // Drain Param Events
  if (ctx->processParamEvent) {
    ParamEvent paramEvent;
    while (ctx->paramEventQueue.pop(paramEvent)) {
      ctx->processParamEvent(paramEvent, ctx->userContext);
    }
  }

  // Fill Audio Block
  if (ctx->processAudioBlock) {
    ctx->processAudioBlock(buffer.channelPtrs,
                           buffer.numChannels,
                           buffer.numFrames,
                           ctx->userContext);
  }
}

// ==== PUBLIC APIS ====

// ==== Session Handlers ====
hSynthSession initSession(SessionConfig userConfig,
                          SynthCallbacks userCallbacks,
                          void* userContext) {

  hSynthSession sessionPtr = new SynthSession();
  sessionPtr->processParamEvent = userCallbacks.processParamEvent;
  sessionPtr->processMIDIEvent = userCallbacks.processMIDIEvent;
  sessionPtr->processAudioBlock = userCallbacks.processAudioBlock;
  sessionPtr->userContext = userContext;

  // 2. Setup audio_io
  audio_io::Config config{};
  config.sampleRate = userConfig.sampleRate;
  config.numChannels = userConfig.numChannels;
  config.numFrames = userConfig.numFrames;
  config.bufferFormat = static_cast<audio_io::BufferFormat>(userConfig.bufferFormat);

  sessionPtr->audioSession = audio_io::setupAudioSession(config, audioCallback, sessionPtr);

  return sessionPtr;
};

int startSession(hSynthSession sessionPtr) {
  return audio_io::startAudioSession(sessionPtr->audioSession);
}

int stopSession(hSynthSession sessionPtr) {
  return audio_io::stopAudioSession(sessionPtr->audioSession);
}

int disposeSession(hSynthSession sessionPtr) {
  int status = audio_io::cleanupAudioSession(sessionPtr->audioSession);
  if (status != 0) {
    printf("Unable to cleanup Audio Session");
    return 1;
  }

  delete sessionPtr;

  return 0;
}

// ==== MIDI Event Handler ====
bool pushMIDIEvent(hSynthSession sessionPtr, MIDIEvent event) {
  return sessionPtr->midiEventQueue.push(event);
};

// ==== Parameter Event Handlers ====
bool setParam(hSynthSession sessionPtr, uint8_t id, float value) {
  // TODO(nico): replicate emplace_back() to reduce copy;
  return sessionPtr->paramEventQueue.push({id, value});
}

} // namespace app::session
