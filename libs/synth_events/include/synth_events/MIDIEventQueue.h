#pragma once

#include <atomic>
#include <cstddef>
#include <cstdio>

namespace synth_events {

struct MIDIEvent {
  enum class Type : uint8_t {
    NoteOn,
    NoteOff,
    ControlChange,
    PitchBend,
    ProgramChange,
    Aftertouch,
    ChannelPressure,
    Unknown
  };

  Type type = Type::Unknown;
  uint8_t channel = 0;
  uint64_t timestamp = 0;

  union {
    struct {
      uint8_t note;
      uint8_t velocity;
    } noteOn;
    struct {
      uint8_t note;
      uint8_t velocity;
    } noteOff;
    struct {
      uint8_t number;
      uint8_t value;
    } cc;
    struct {
      int16_t value;
    } pitchBend;
    struct {
      uint8_t number;
    } programChange;
    struct {
      uint8_t note;
      uint8_t pressure;
    } aftertouch;
    struct {
      uint8_t pressure;
    } channelPressure;
  } data;
};

struct MIDIHandle;
using hMIDIHandle = MIDIHandle*;

hMIDIHandle initMIDIHandle();
void disposeMIDIHandle(hMIDIHandle handle);

bool pushMIDIEvent(hMIDIHandle handle, MIDIEvent event); // CoreMIDI thread
bool popMIDIEvent(hMIDIHandle handle, MIDIEvent& event); // audio thread (drain)

struct MIDIEventQueue {
  // NOTE(nico): SIZE value need to be power of to use bitmasking for wrapping
  // Alternative is modulo (%) which is more expensive
  static constexpr size_t SIZE{256};
  static constexpr size_t WRAP{SIZE - 1};

  MIDIEvent queue[SIZE];

  std::atomic<size_t> readIndex{0};
  std::atomic<size_t> writeIndex{0};

  bool push(const MIDIEvent& event);
  bool pop(MIDIEvent& event);
};

} // namespace synth_events
