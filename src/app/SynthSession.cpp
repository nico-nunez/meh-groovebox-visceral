#include "app/SynthSession.h"

#include "audio_io/AudioIO.h"
#include "audio_io/AudioIOTypes.h"
#include "audio_io/AudioIOTypesFwd.h"

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

struct SynthSession {
  MIDIEventQueue midiEventQueue{};
  ParamEventQueue paramEventQueue{};
  EngineEventQueue engineEventQueue{};

  AudioBufferHandler processAudioBlock;

  MIDIEventHandler processMIDIEvent;
  ParamEventHandler processParamEvent;
  EngineEventHandler processEngineEvent;

  hAudioSession audioSession;
  void* userContext;
};

static void audioCallback(AudioBuffer buffer, void* context) {
  auto* ctx = static_cast<SynthSession*>(context);

  // Drain MIDI Events
  if (ctx->processMIDIEvent) {
    MIDIEvent evt;
    while (ctx->midiEventQueue.pop(evt))
      ctx->processMIDIEvent(evt, ctx->userContext);
  }

  // Drain Param Events (scalar)
  if (ctx->processParamEvent) {
    ParamEvent evt;
    while (ctx->paramEventQueue.pop(evt))
      ctx->processParamEvent(evt, ctx->userContext);
  }

  // Drain Engine Events (non-scalar params)
  if (ctx->processEngineEvent) {
    EngineEvent evt;
    while (ctx->engineEventQueue.pop(evt))
      ctx->processEngineEvent(evt, ctx->userContext);
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
  sessionPtr->processEngineEvent = userCallbacks.processEngineEvent;
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

// ==== Event Handlers ====
// TODO(nico): replicate emplace_back() to reduce copy;

bool pushMIDIEvent(hSynthSession sessionPtr, MIDIEvent evt) {
  return sessionPtr->midiEventQueue.push(evt);
};

bool pushParamEvent(hSynthSession sessionPtr, ParamEvent evt) {
  return sessionPtr->paramEventQueue.push(evt);
}

bool pushEngineEvent(hSynthSession sessionPtr, EngineEvent evt) {
  return sessionPtr->engineEventQueue.push(evt);
}

} // namespace app::session
