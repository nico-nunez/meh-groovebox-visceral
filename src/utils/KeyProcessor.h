#pragma once

#include "app/SynthSession.h"

#include "device_io/MidiCapture.h"

#include <cstdint>

namespace synth::utils {
using app::session::hSynthSession;
using device_io::hMidiSession;

hMidiSession initMidiSession(hSynthSession);

int startGLFWLoop(hSynthSession, hMidiSession);
void requestQuit();

uint8_t asciiToMidi(char key);
} // namespace synth::utils
