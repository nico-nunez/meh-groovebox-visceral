#pragma once

#include "device_io/MidiCapture.h"

namespace app {
struct AppContext;
}

namespace app::midi {
using device_io::hMidiSession;

hMidiSession initSession(AppContext* ctx);

} // namespace app::midi
