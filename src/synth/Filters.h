#pragma once

#include "synth/Types.h"

#include "dsp/Filters.h"
#include <cstddef>

namespace synth::filters {

enum class SVFMode { LP, HP, BP, Notch, MODE_COUNT };
using dsp::filters::SVFCoeffs;
using dsp::filters::SVFOutputs;
using dsp::filters::SVFState;

using dsp::filters::LadderState;

// ==== State Variable Filter (SVF) ====
struct SVFilter {
  SVFState voiceStates[MAX_VOICES]; // (hot path)

  // Cached coefficients (cold, recomputed on param change)
  SVFCoeffs coeffs{};

  // TODO(nico): need to define value ranges
  // Global settings (cold)
  SVFMode mode = SVFMode::LP;
  float cutoff = 1000.0f; // Hz
  float resonance = 0.5f; // 0.0–1.0 (mapped to Q internally)
  bool enabled = false;
};

// ==== Ladder Filter (Moog Style) ====
struct LadderFilter {
  LadderState voiceStates[MAX_VOICES]; // (hot path)

  // Cached coefficient (cold, recomputed on param change)
  // frequency coefficient: 2 * sin(π * cutoff / sampleRate)
  float coeff = 0.0f;

  // TODO(nico): need to define value ranges
  // Global settings (cold data)
  float cutoff = 1000.0f; // Hz
  float resonance = 0.3f; // 0.0–1.0 (mapped to 0–4 internally)
  float drive = 1.0f;     // 1.0 = neutral, higher = more saturation (nonlinear path)
  bool enabled = false;
};

// ==== FILTER HELPERS ====

// ==== SVF Helpers ====
void initSVFilter(SVFilter& filter, size_t voiceIndex);

void updateSVFCoefficients(SVFilter& filter, float invSampleRate);

// No modulation parameters
float processSVFilter(SVFilter& filter, float input, uint32_t voiceIndex);

// With modulation parameters
float processSVFilter(SVFilter& filter,
                      float input,
                      uint32_t voiceIndex,
                      float cutoffHz,
                      float resonance,
                      float invSampleRate);

// ==== Ladder Helpers ====
void initLadderFilter(LadderFilter& filter, size_t voiceIndex);

void updateLadderCoefficient(LadderFilter& filter, float invSampleRate);

// No modulation parameters
float processLadderFilter(LadderFilter& filter, float input, uint32_t voiceIndex);
// With modulation parameters
float processLadderFilter(LadderFilter& filter,
                          float input,
                          uint32_t voiceIndex,
                          float cutoffHz,
                          float resonance,
                          float invSampleRate);

} // namespace synth::filters
