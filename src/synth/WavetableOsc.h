#pragma once

#include "Types.h"

#include "dsp/Wavetable.h"

#include <cstdint>
#include <cstring>

namespace synth::wavetable::osc {

using dsp::wavetable::WavetableBank;

enum class FMSource : uint8_t {
  None = 0,
  Osc1,
  Osc2,
  Osc3,
  Osc4,
};

struct WavetableOscConfig {
  const WavetableBank* bank = nullptr;
  float scanPos = 0.0f;
  float mixLevel = 1.0f;
  float fmDepth = 0.0f;
  float fmRatio = 1.0f;
  FMSource fmSource = FMSource::None;
  int8_t octaveOffset = 0;
  float detuneAmount = 0.0f;
  bool enabled = true;
};

struct WavetableOsc {
  // ==== Per-voice hot data (SoA) ====
  float phases[MAX_VOICES];          // normalized [0, 1.0)
  float phaseIncrements[MAX_VOICES]; // cycles per sample (freq / sampleRate)

  // ==== Global settings for all voices in oscillator ====
  const WavetableBank* bank = nullptr;
  float scanPos = 0.0f;
  float mixLevel = 1.0f;
  float fmDepth = 0.0f;
  float fmRatio = 1.0f;
  FMSource fmSource = FMSource::None;
  int8_t octaveOffset = 0;
  float detuneAmount = 0.0f;
  bool enabled = true;
};

void initOsc(WavetableOsc& osc, uint32_t voiceIndex, uint8_t midiNote, float sampleRate);

// Mono (no retrigger/legato)
void updateOscPitch(WavetableOsc& osc, uint32_t voiceIndex, uint8_t midiNote, float sampleRate);

void updateConfig(WavetableOsc& osc, const WavetableOscConfig& config);

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
// Serialization
// ===================

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
  return FMSource::None; // unknown strings → None
}

inline const char* fmSourceToString(FMSource src) {
  for (const auto& m : fmSourceMappings)
    if (m.src == src)
      return m.name;
  return "none";
}

} // namespace synth::wavetable::osc
