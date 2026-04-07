#pragma once

#include "synth/Types.h"

#include "dsp/Filters.h"
#include <cstddef>
#include <cstring>

namespace synth::filters {

using dsp::filters::SVFCoeffs;
using dsp::filters::SVFOutputs;
using dsp::filters::SVFState;

using dsp::filters::LadderState;

enum SVFMode : uint8_t { LP, HP, BP, Notch, Unknown, COUNT };
inline constexpr const char* UNKNOWN_MODE = "unknown";

// ==== State Variable Filter (SVF) ====
struct SVFilter {
  SVFState statesL[MAX_VOICES];
  SVFState statesR[MAX_VOICES];

  // Cached coefficients (cold, recomputed on param change)
  SVFCoeffs coeffs{};

  // Global settings (cold)
  SVFMode mode = SVFMode::LP;
  float cutoff = 1000.0f; // Hz
  float resonance = 0.5f; // 0.0–1.0 (mapped to Q internally)
  bool enabled = false;
};

// ==== Ladder Filter (Moog Style) ====
struct LadderFilter {
  // ==== <hot path> ====
  LadderState statesL[MAX_VOICES]{};
  LadderState statesR[MAX_VOICES]{};

  // NR solver state for nonlinear TPT (per-voice, per-channel)
  float y4EstimateL[MAX_VOICES] = {};
  float y4EstimateR[MAX_VOICES] = {};
  // ==== </hot path> ====

  // Cached coefficient (cold, recomputed on param change)
  float coeff = 0.0f;

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
void processSVFilter(SVFilter& filter, float& inOutL, float& inOutR, uint32_t voiceIndex);

// With modulation parameters
void processSVFilter(SVFilter& filter,
                     float& inOutL,
                     float& inOutR,
                     uint32_t voiceIndex,
                     float cutoffHz,
                     float resonance,
                     float invSampleRate);

// ==== Ladder Helpers ====
void initLadderFilter(LadderFilter& filter, size_t voiceIndex);

void updateLadderCoefficient(LadderFilter& filter, float invSampleRate);

// No modulation parameters
void processLadderFilter(LadderFilter& filter, float& inOutL, float& inOutR, uint32_t voiceIndex);

// With modulation parameters
void processLadderFilter(LadderFilter& filter,
                         float& inOutL,
                         float& inOutR,
                         uint32_t voiceIndex,
                         float cutoffHz,
                         float resonance,
                         float invSampleRate);

// ============================================================
// Parsing Helpers
// ============================================================

struct SVFModeMapping {
  const char* name;
  filters::SVFMode mode;
};

// TODO: consider adding "None" (aka disabled)
inline constexpr SVFModeMapping svfModeMappings[] = {
    {"lp", filters::SVFMode::LP},
    {"hp", filters::SVFMode::HP},
    {"bp", filters::SVFMode::BP},
    {"notch", filters::SVFMode::Notch},
};

inline filters::SVFMode parseSVFMode(const char* name) {
  for (const auto& m : svfModeMappings)
    if (std::strcmp(m.name, name) == 0)
      return m.mode;

  return filters::SVFMode::Unknown;
}

inline const char* svfModeToString(filters::SVFMode mode) {
  for (const auto& m : svfModeMappings)
    if (m.mode == mode)
      return m.name;
  return UNKNOWN_MODE;
}

} // namespace synth::filters
