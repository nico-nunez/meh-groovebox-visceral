#pragma once

#include "Types.h"
#include "synth/WavetableOsc.h"
#include <cstdint>

namespace synth::unison {
using wavetable::osc::PhaseMode;

inline constexpr uint8_t MAX_UNISON_VOICES = 8;

struct UnisonState {
  int8_t voices = 4;    // 1–8 (1 = off)
  float detune = 20.0f; // total spread in cents (0–100)
  float spread = 0.5f;  // stereo width [0.0, 1.0]
  bool enabled = false;

  // Precomputed — updated on param change & read in hot loop
  float detuneOffsets[MAX_UNISON_VOICES]{}; // semitones
  float detuneRatios[MAX_UNISON_VOICES]{};  // precomputed semitonesToFreqRatio(offset)
  float panPositions[MAX_UNISON_VOICES]{};  // [-1, +1]
  float panGainsL[MAX_UNISON_VOICES]{};     // precomputed cos(angle)
  float panGainsR[MAX_UNISON_VOICES]{};     // precomputed sin(angle)
  float gainComp = 1.0f;                    // 1/sqrt(voices)

  // Per-osc per-sub-voice per-slot phase accumulators
  float subPhases[NUM_OSCS][MAX_UNISON_VOICES][MAX_VOICES]{};
};

void initUnisonSubPhases(UnisonState& unison,
                         const PhaseMode phaseModes[NUM_OSCS],
                         const float randomRanges[NUM_OSCS],
                         const float resetPhases[NUM_OSCS],
                         uint32_t voiceIndex);

// ==== Helpers ====
void updateDetuneOffsets(unison::UnisonState& u);
void updatePanPositions(unison::UnisonState& u);
void updateGainComp(unison::UnisonState& u);

} // namespace synth::unison
