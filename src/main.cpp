#include "app/AppContext.h"
#include "app/GrooveboxPaths.h"
#include "app/sessions/AudioSession.h"
#include "app/sessions/MIDISession.h"

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
int main(int argc, char* argv[]) {
  app::GrooveboxPaths paths = app::resolveGrooveboxPaths(argc, argv);
  printf("session: %s\n", paths.sessionFile.c_str());

  auto deviceInfo = app::audio::queryDefaultDevice();

  printf("Audio device: %u Hz, %u frames, %u channels\n",
         deviceInfo.sampleRate,
         deviceInfo.bufferFrameSize,
         deviceInfo.numChannels);

  auto appContext = app::createAppContext(deviceInfo);
  appContext->grooveboxPaths = paths;
  app::editor::loadDocument(appContext->authoredEditor, paths.sessionFile.c_str());

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
