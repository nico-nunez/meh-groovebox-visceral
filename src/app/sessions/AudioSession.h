#pragma once

#include "audio_io/AudioIOTypesFwd.h"

#include <cstddef>
#include <cstdint>

namespace app {
struct AppContext;
}

namespace app::audio {
using audio_io::hAudioSession;

// --- Constants ---
inline constexpr uint32_t DEFAULT_SAMPLE_RATE = 48000;
inline constexpr uint32_t DEFAULT_FRAMES = 512;
inline constexpr uint16_t DEFAULT_CHANNELS = 2;

struct DeviceInfo {
  uint32_t sampleRate = DEFAULT_SAMPLE_RATE;
  uint32_t bufferFrameSize = DEFAULT_FRAMES;
  uint16_t numChannels = DEFAULT_CHANNELS;
};

DeviceInfo queryDefaultDevice();

enum class BufferFormat {
  NonInterleaved, // channels in separate arrays [LLLL] [RRRR]
  Interleaved,    // channels interwoven in single array [LRLRLRLR]
};

hAudioSession initSession(const DeviceInfo& deviceInfo, app::AppContext* ctx);

int startSession(hAudioSession sessionPtr);
int stopSession(hAudioSession sessionPtr);
int disposeSession(hAudioSession sessionPtr);

} // namespace app::audio
