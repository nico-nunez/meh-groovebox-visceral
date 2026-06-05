#pragma once

#include <cstdint>

namespace app {
inline constexpr uint8_t MAX_TRACKS = 8;

inline constexpr uint8_t MAX_MIDI_CHANNELS = 16;
inline constexpr uint8_t MIDI_CHANNEL_UNASSIGNED = 0xFF;

// =================
// app::audio
// =================
namespace audio {
inline constexpr uint32_t DEFAULT_SAMPLE_RATE = 48000;
inline constexpr uint32_t DEFAULT_FRAMES = 512;
inline constexpr uint16_t DEFAULT_CHANNELS = 2;
inline constexpr uint32_t MAX_BLOCK_FRAMES = DEFAULT_FRAMES;
} // namespace audio

// =================
// app::transport
// =================
namespace transport {
inline constexpr float MIN_BPM = 20.0f;
inline constexpr float MAX_BPM = 300.0f;
inline constexpr float DEFAULT_BPM = 120.0f;
inline constexpr double BEATS_PER_BAR = 4.0;
} // namespace transport

// =================
// app::sequencer
// =================
namespace sequencer {
inline constexpr uint8_t PATTERNS_PER_LANE = 8;
inline constexpr uint8_t INVALID_PATTERN_SLOT = 0xFF;

inline constexpr uint8_t MAX_LANES = MAX_TRACKS;
inline constexpr double MIN_GATE_BEAT =
    static_cast<double>(audio::DEFAULT_FRAMES) /
    (audio::DEFAULT_SAMPLE_RATE * (transport::DEFAULT_BPM / 60.0));

inline constexpr uint8_t MAX_PATTERN_STEPS = 64;
inline constexpr uint8_t DEFAULT_PATTERN_STEPS = 16;
inline constexpr uint8_t MAX_STEPS_PER_BEAT = 48;
inline constexpr uint8_t DEFAULT_STEPS_PER_BEAT = 4;

inline constexpr uint8_t MAX_NOTES_PER_STEP = 8;
inline constexpr uint16_t MAX_PENDING_NOTE_OFFS = MAX_PATTERN_STEPS * MAX_NOTES_PER_STEP;

inline constexpr uint8_t MAX_LOCKS_PER_STEP = 4;
inline constexpr uint32_t MAX_PENDING_UNLOCKS = MAX_LOCKS_PER_STEP * MAX_PATTERN_STEPS;

// Per active step, worst case:
// - MAX_LOCKS_PER_STEP param unlocks
// - MAX_LOCKS_PER_STEP param locks
// - per note: same-pitch cut NoteOff, NoteOn, same-block gate NoteOff
// After step processing, pending gate NoteOffs can also flush in this block.
inline constexpr uint16_t MAX_NOTE_EVENTS_PER_STEP = MAX_NOTES_PER_STEP * 3;
inline constexpr uint16_t MAX_LANE_EVENTS_PER_BLOCK =
    MAX_PATTERN_STEPS * ((2 * MAX_LOCKS_PER_STEP) + MAX_NOTE_EVENTS_PER_STEP) +
    MAX_PENDING_NOTE_OFFS;
} // namespace sequencer

// =================
// app::track
// =================
namespace track {
inline constexpr uint32_t MAX_EVENTS_PER_TRACK = sequencer::MAX_LANE_EVENTS_PER_BLOCK;
}

} // namespace app
