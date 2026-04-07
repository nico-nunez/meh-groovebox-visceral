#pragma once

#include "dsp/Noise.h"

#include <cstdint>
#include <cstring>

namespace synth::noise {
using dsp::noise::PinkNoiseState;

enum NoiseType : uint8_t { White = 0, Pink, Unknown, COUNT };
inline constexpr const char* UNKNOWN_NOISE = "unknown";

struct Noise {
  PinkNoiseState pinkNoiseState;
  float mixLevel = 0.0f;
  NoiseType type = NoiseType::White;
  bool enabled = false;
};

// Returns noise in [-1, 1] scaled by mixLevel.
// White: flat spectrum. Pink: -3dB/octave via Kellet approximation.
float processNoise(Noise& noise);

// ============================================================
// Parsing Helpers
// ============================================================

struct NoiseTypeMapping {
  const char* name;
  noise::NoiseType type;
};

inline constexpr NoiseTypeMapping noiseTypeMappings[] = {
    {"white", noise::NoiseType::White},
    {"pink", noise::NoiseType::Pink},
};

inline noise::NoiseType parseNoiseType(const char* name) {
  for (const auto& m : noiseTypeMappings)
    if (std::strcmp(m.name, name) == 0)
      return m.type;

  return noise::NoiseType::Unknown;
}

inline const char* noiseTypeToString(noise::NoiseType type) {
  for (const auto& m : noiseTypeMappings)
    if (m.type == type)
      return m.name;

  return UNKNOWN_NOISE;
}

} // namespace synth::noise
