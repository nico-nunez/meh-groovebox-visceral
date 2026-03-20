#pragma once

#include "synth/Types.h"

#include <cstdint>

namespace synth::mono {
inline constexpr uint8_t MAX_HELD_NOTES = 128;
inline constexpr uint8_t MAX_NOTE_STACK = 16;

struct MonoState {
  bool enabled = false;
  bool legato = true;

  uint32_t voiceIndex = synth::MAX_VOICES;

  bool heldNotes[MAX_HELD_NOTES]{};
  uint8_t noteStack[MAX_NOTE_STACK]{};
  uint8_t stackDepth = 0;
};

void pushNoteToStack(MonoState& mono, uint8_t midiNote);
void removeNoteFromStack(MonoState& state, uint8_t midiNote);

} // namespace synth::mono
