#pragma once

#include "synth/Types.h"

#include "dsp/Wavetable.h"

#include <cstdint>
#include <cstring>

namespace synth::wavetable::osc {

using dsp::wavetable::WavetableBank;

enum PhaseMode : uint8_t {
  Reset = 0,
  Free,
  Random,
  Spread,
  Unknown,
};
inline const char* UNKNOWN_PHASE_MODE = "unknown phase mode";
inline constexpr uint8_t NUM_PHASE_MODES = 4;

enum FMSource : uint8_t {
  None = 0,
  Osc1,
  Osc2,
  Osc3,
  Osc4,
};

struct FMRoute {
  FMSource source = FMSource::None;
  float depth = 0.0f; // valid range [0.0, 10.0]
};

struct WavetableOsc {
  // ==== Per-voice hot data (SoA) ====
  float phases[MAX_VOICES];          // normalized [0, 1.0)
  float phaseIncrements[MAX_VOICES]; // cycles per sample (freq / sampleRate)

  // ==== Global settings for all voices in oscillator ====
  const WavetableBank* bank = nullptr;
  float mixLevel = 1.0f;
  int8_t octaveOffset = 0;
  float detuneAmount = 0.0f;

  float scanPos = 0.0f;

  PhaseMode phaseMode = PhaseMode::Reset;
  float randomRange = 1.0f; // [0.0, 1.0]
  float resetPhase = 0.0f;  // [0.0, 1.0]

  float ratio = 1.0f;
  bool fixed = false; // false == ratio | true == fixedFreq
  float fixedFreq = 440.0f;
  float fixedPhaseInc = 0.0f; // fixedFreq / sampleRate

  FMRoute fmRoutes[NUM_OSCS] = {};
  uint8_t fmRouteCount = 0;
  float fmDepthMod = 0.0f;

  bool enabled = true;
};

void initOsc(WavetableOsc& osc, uint32_t voiceIndex, uint8_t midiNote, float sampleRate);

// Mono (no retrigger/legato)
void updateOscPitch(WavetableOsc& osc, uint32_t voiceIndex, uint8_t midiNote, float sampleRate);

/* Read one sample with dual-mip linear interpolation.
 * mipF: continuous mip level from selectMipLevel() — fractional part drives mip
 * crossfade. effectiveScanPos: base scanPos + mod delta, clamped [0,1] by
 * caller.
 *
 * fmPhaseOffset: phase displacement from FM in cycles (0.0 = no FM). Any
 * magnitude, any sign — wrapping is handled internally via floorf. This is why
 * phases are normalized.
 */
float readWavetable(const WavetableOsc& osc,
                    uint32_t voiceIndex,
                    float mipF,
                    float effectiveScanPos,
                    float fmPhaseOffset);

// Process oscillator (read table and increment phase)
float processOsc(WavetableOsc& osc,
                 uint32_t voiceIndex,
                 float mipF,
                 float effectiveScanPos,
                 float fmPhaseOffset,
                 float pitchIncrement);

// =====================
// FM (phase modulation)
// =====================
struct WavetableOscModState {
  float osc1[MAX_VOICES] = {};
  float osc2[MAX_VOICES] = {};
  float osc3[MAX_VOICES] = {};
  float osc4[MAX_VOICES] = {};

  float osc1Feedback[MAX_VOICES] = {};
  float osc2Feedback[MAX_VOICES] = {};
  float osc3Feedback[MAX_VOICES] = {};
  float osc4Feedback[MAX_VOICES] = {};
};

void resetOscModState(WavetableOscModState& modState, uint32_t voiceIndex);

float getFmInputValue(WavetableOscModState& modState,
                      uint32_t voiceIndex,
                      FMSource src,
                      FMSource dest);

// ===================
// Parsing Helpers
// ===================
void parseFMCmd();

inline const char* phaseModeToString(PhaseMode mode) {
  switch (mode) {
  case PhaseMode::Reset:
    return "reset";
  case PhaseMode::Free:
    return "free";
  case PhaseMode::Random:
    return "random";
  case PhaseMode::Spread:
    return "spread";
  default:
    return UNKNOWN_PHASE_MODE;
  }
}

inline PhaseMode parsePhaseMode(const char* s) {
  if (std::strcmp(s, "reset") == 0)
    return PhaseMode::Reset;
  if (std::strcmp(s, "free") == 0)
    return PhaseMode::Free;
  if (std::strcmp(s, "random") == 0)
    return PhaseMode::Random;
  if (std::strcmp(s, "spread") == 0)
    return PhaseMode::Spread;
  return PhaseMode::Unknown;
}

struct FMSourceMapping {
  const char* name;
  FMSource src;
};

inline constexpr FMSourceMapping fmSourceMappings[] = {
    {"none", FMSource::None},
    {"osc1", FMSource::Osc1},
    {"osc2", FMSource::Osc2},
    {"osc3", FMSource::Osc3},
    {"osc4", FMSource::Osc4},
};

inline FMSource parseFMSource(const char* name) {
  for (const auto& m : fmSourceMappings)
    if (std::strcmp(m.name, name) == 0)
      return m.src;

  return FMSource::None;
}

inline const char* fmSourceToString(FMSource src) {
  for (const auto& m : fmSourceMappings)
    if (m.src == src)
      return m.name;

  return "none";
}

} // namespace synth::wavetable::osc
