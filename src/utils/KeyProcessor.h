#pragma once

#include "app/AudioSession.h"

#include "device_io/MidiCapture.h"

#include <cstdint>

namespace app {
struct AppContext;
}

namespace app::utils {
using device_io::hMidiSession;

int startGLFWLoop(AppContext*, hMidiSession);
void requestQuit();

uint8_t asciiToMidi(char key);
} // namespace app::utils
