#include "app/AppContext.h"
#include "app/AudioSession.h"
#include "app/MIDISession.h"

#include "lua/LuaREPL.h"
#include "utils/KeyProcessor.h"

#include <audio_io/AudioIO.h>

#include <csignal>
#include <cstdio>
#include <functional>
#include <thread>

// ==============
// App Runtime
// ==============
int main() {
  auto deviceInfo = app::audio::queryDefaultDevice();

  printf("Audio device: %u Hz, %u frames, %u channels\n",
         deviceInfo.sampleRate,
         deviceInfo.bufferFrameSize,
         deviceInfo.numChannels);

  auto appContext = app::createAppContext(deviceInfo);
  auto audioSession = app::audio::initSession(deviceInfo, appContext);
  app::audio::startSession(audioSession);

  auto midiSession = app::midi::initSession(appContext);

  std::thread terminalWorker(lua::repl::runLuaREPL, std::ref(appContext));
  terminalWorker.detach();

  app::utils::startGLFWLoop(appContext, midiSession);

  printf("Goodbye and thanks for playing :)\n");

  app::audio::stopSession(audioSession);
  app::audio::disposeSession(audioSession);

  return 0;
}
