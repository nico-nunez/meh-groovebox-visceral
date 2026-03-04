#pragma once

#include "Types.h"

#include "dsp/Waveforms.h"

#include <cstdint>

namespace synth::oscillator {
using dsp::waveforms::WaveformType;

struct OscConfig {
  WaveformType waveform = WaveformType::Sine;
  float mixLevel = 1.0f;     // 0.0-4.0 (-inf to +12DB)
  int8_t octaveOffset = 0;   // -2 to +2
  float detuneAmount = 0.0f; // Cents: -100 to +100
  bool enabled = true;
};

struct Oscillator {
  // === Per-voice state (hot data) ===
  float phases[MAX_VOICES];
  float phaseIncrements[MAX_VOICES];

  // === Global settings (cold data) ===
  WaveformType waveform = WaveformType::Sine;
  float mixLevel = 1.0f;     // 0.0-1.0
  int8_t octaveOffset = 0;   // -2 to +2
  float detuneAmount = 0.0f; // Cents: -100 to +100
  bool enabled = true;
};

Oscillator createOscillator(const OscConfig& settings);
void updateConfig(Oscillator& osc, const OscConfig& settings);

void setWaveformType(Oscillator& osc, WaveformType newType);
void setMixLevel(Oscillator& osc, float newLevel);
void setOctiveOffset(Oscillator& osc, int8_t newOffest);
void setDetuneAmount(Oscillator& osc, float newDetuneAmount);
void toggleEnabled(Oscillator& osc, bool isEnabled);

void initOscillator(Oscillator& osc, uint32_t voiceIndex, uint8_t midiNote, float sampleRate);

void incrementPhase(Oscillator& osc, uint32_t voiceIndex);

// Original - pre Mod Matrix and acts as pass-through
float processOscillator(Oscillator& osc, uint32_t voiceIndex);

// Mod Matrix version allowing pitch modulation (called/returned via original)
float processOscillator(Oscillator& osc, uint32_t voiceIndex, float phaseIncrement, float mixLevel);

} // namespace synth::oscillator
