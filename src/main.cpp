#include "app/SynthSession.h"
#include "utils/InputProcessor.h"
#include "utils/KeyProcessor.h"

#include "synth/Engine.h"

#include <audio_io/AudioIO.h>
#include <csignal>
#include <cstdio>
#include <functional>
#include <iostream>
#include <thread>

static void processMIDIEvent(synth::MIDIEvent event, void* myContext) {
  auto engine = static_cast<synth::Engine*>(myContext);
  engine->processMIDIEvent(event);
}

static void processParamEvent(synth::ParamEvent event, void* myContext) {
  auto engine = static_cast<synth::Engine*>(myContext);
  engine->processParamEvent(event);
}

static void processEngineEvent(synth::EngineEvent event, void* ctx) {
  auto* engine = static_cast<synth::Engine*>(ctx);
  engine->processEngineEvent(event);
}

static void
processAudioBlock(float** outputBuffer, size_t numChannels, size_t numFrames, void* myContext) {
  auto engine = static_cast<synth::Engine*>(myContext);
  engine->processAudioBlock(outputBuffer, numChannels, numFrames);
}

static void getUserInput(synth::Engine& engine, app::session::hSynthSession sessionPtr) {
  bool isRunning = true;
  std::string input;

  while (isRunning) {
    printf(">");
    std::getline(std::cin, input);

    synth::utils::parseCommand(input, engine, sessionPtr);

    if (input == "quit") {
      synth::utils::requestQuit();
      isRunning = false;
    }
  }
}

int main() {
  using synth::Engine;
  using synth::EngineConfig;

  using app::session::hSynthSession;
  using app::session::SessionConfig;
  using app::session::SynthCallbacks;

  // Query hardware for actual device parameters
  auto deviceInfo = app::session::queryDefaultDevice();
  float sampleRate = static_cast<float>(deviceInfo.sampleRate);

  printf("Audio device: %u Hz, %u frames, %u channels\n",
         deviceInfo.sampleRate,
         deviceInfo.bufferFrameSize,
         deviceInfo.numChannels);

  EngineConfig engineConfig{};
  engineConfig.sampleRate = sampleRate;
  engineConfig.numFrames = deviceInfo.bufferFrameSize;

  Engine engine = synth::createEngine(engineConfig);

  SessionConfig sessionConfig{};
  sessionConfig.sampleRate = deviceInfo.sampleRate;
  sessionConfig.numFrames = deviceInfo.bufferFrameSize;
  sessionConfig.numChannels = deviceInfo.numChannels;

  SynthCallbacks sessionCallbacks{};
  sessionCallbacks.processMIDIEvent = processMIDIEvent;
  sessionCallbacks.processParamEvent = processParamEvent;
  sessionCallbacks.processEngineEvent = processEngineEvent;
  sessionCallbacks.processAudioBlock = processAudioBlock;

  hSynthSession session = app::session::initSession(sessionConfig, sessionCallbacks, &engine);

  app::session::startSession(session);
  auto midiSession = synth::utils::initMidiSession(session);

  std::thread terminalWorker(getUserInput, std::ref(engine), session);
  terminalWorker.detach();

  synth::utils::startGLFWLoop(session, midiSession);

  printf("Goodbye and thanks for playing :)\n");

  app::session::stopSession(session);
  app::session::disposeSession(session);

  return 0;
}
