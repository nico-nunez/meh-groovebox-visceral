#include "utils/InputProcessor.h"
#include "utils/KeyProcessor.h"

#include "device_io/KeyCapture.h"
#include "synth_io/Events.h"
#include "synth_io/SynthIO.h"

#include "synth/Engine.h"

#include <audio_io/AudioIO.h>
#include <csignal>
#include <cstdio>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

static void processParamEvent(synth_io::ParamEvent event, void* myContext) {
  auto engine = static_cast<synth::Engine*>(myContext);
  engine->processParamEvent(event);
}

static void processNoteEvent(synth_io::NoteEvent event, void* myContext) {
  auto engine = static_cast<synth::Engine*>(myContext);
  engine->processNoteEvent(event);
}

static void
processAudioBlock(float** outputBuffer, size_t numChannels, size_t numFrames, void* myContext) {
  auto engine = static_cast<synth::Engine*>(myContext);
  engine->processAudioBlock(outputBuffer, numChannels, numFrames);
}

static void getUserInput(synth::Engine& engine, synth_io::hSynthSession sessionPtr) {
  bool isRunning = true;
  std::string input;

  while (isRunning) {
    printf(">");
    std::getline(std::cin, input);

    synth::utils::parseCommand(input, engine, sessionPtr);

    if (input == "quit") {
      device_io::terminateKeyCaptureLoop();
      isRunning = false;
    }
  }
}

int main() {
  using synth::Engine;
  using synth::EngineConfig;

  constexpr float SAMPLE_RATE = 48000.0f;

  EngineConfig engineConfig{};
  engineConfig.sampleRate = SAMPLE_RATE;

  Engine engine = synth::createEngine(engineConfig);

  // 2. Setup audio_io
  synth_io::SessionConfig sessionConfig{};
  sessionConfig.sampleRate = static_cast<uint32_t>(SAMPLE_RATE);

  synth_io::SynthCallbacks sessionCallbacks{};
  sessionCallbacks.processAudioBlock = processAudioBlock;
  sessionCallbacks.processNoteEvent = processNoteEvent;

  sessionCallbacks.processParamEvent = processParamEvent;

  synth_io::hSynthSession session = synth_io::initSession(sessionConfig, sessionCallbacks, &engine);

  synth_io::startSession(session);

  auto midiSession = synth::utils::initMidiSession(session);

  std::thread terminalWorker(getUserInput, std::ref(engine), session);
  terminalWorker.detach();

  synth::utils::startKeyInputCapture(session, midiSession);

  /* This currently unreachable on MacOS due to `terminal:nil` being called
   * in `stopKeyCaptureLoop()` and immediately exits app.
   *
   * This can be updated to `stop:nil`, along with a "dummy-event" to simply
   * stop the loop and allow this to be reachable
   *
   * Currently this is fine since the OS cleans up when the app exits anyways.
   * However, if anything non-cleanup needs to occur (like auto-save) then
   * changes need to be made.
   */
  printf("Goodbye and thanks for playing :)\n");

  synth_io::stopSession(session);
  synth_io::disposeSession(session);

  return 0;
}
