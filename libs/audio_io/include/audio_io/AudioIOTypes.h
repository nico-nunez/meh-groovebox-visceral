#pragma once

#include <cstdint>

namespace audio_io {
// --- Constants ---
inline constexpr uint32_t DEFAULT_SAMPLE_RATE = 48000;
inline constexpr uint32_t DEFAULT_FRAMES = 512;
inline constexpr uint16_t DEFAULT_CHANNELS = 2;

inline constexpr uint32_t MAX_SAMPLE_RATE_OPTIONS = 6;
inline constexpr uint32_t MAX_FRAME_SIZE_OPTIONS = 6;

struct DeviceInfo {
  uint32_t sampleRate;
  uint32_t sampleRateOptions[MAX_SAMPLE_RATE_OPTIONS];
  uint32_t bufferFrameSize;
  uint32_t bufferFrameSizeOptions[MAX_FRAME_SIZE_OPTIONS];
  uint16_t numChannels;
};

enum class BufferFormat {
  NonInterleaved, // channels in separate arrays [LLLL] [RRRR]
  Interleaved,    // channels interwoven in single array [LRLRLRLR]
};

// --- Shared Types ---
struct Config {
  uint32_t sampleRate = DEFAULT_SAMPLE_RATE;
  uint32_t numFrames = DEFAULT_FRAMES;
  uint16_t numChannels = DEFAULT_CHANNELS;
  BufferFormat bufferFormat = BufferFormat::NonInterleaved;
};

struct AudioBuffer {
  BufferFormat format;
  uint32_t numChannels;
  uint32_t numFrames; // Frame == sample interval/step (ie. 128, 256, 512, etc.)

  union {
    float** channelPtrs; // Non-Interleaved (Planar)
    float* interleavedPtr;
  };
};

} // namespace audio_io
