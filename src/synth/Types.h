#pragma once

#include <cstdint>

namespace synth {
inline constexpr uint32_t ENGINE_BLOCK_SIZE = 64;
inline constexpr float INV_ENGINE_BLOCK_SIZE = 1.0f / static_cast<float>(ENGINE_BLOCK_SIZE);

inline constexpr uint32_t MAX_VOICES = 64;
inline constexpr uint8_t NUM_OSCS = 4;

inline constexpr int ROOT_NOTE_MIDI{69};
inline constexpr float ROOT_NOTE_FREQ{440.0f};

using MidiNote = uint8_t;

// Adjust/reduce gain based on N voices
// 1.0f / std::sqrtf(MAX_VOICES);
inline constexpr float VOICE_GAIN = 1.0f / 8.0f;

inline constexpr const char* UNKNOWN_STR = "unknown";

} // namespace synth
