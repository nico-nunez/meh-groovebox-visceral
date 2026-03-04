#pragma once

#include "dsp/Noise.h"

#include <cstdint>

namespace synth::noise {
using dsp::noise::PinkNoiseState;

enum class NoiseType : uint8_t {
  White = 0,
  Pink,
};

struct NoiseConfig {
  float mixLevel = 0.0f;
  NoiseType type = NoiseType::White;
  bool enabled = false;
};

struct Noise {
  PinkNoiseState pinkNoiseState;
  float mixLevel = 0.0f;
  NoiseType type = NoiseType::White;
  bool enabled = false;
};

void updateConfig(Noise& noise, const NoiseConfig& config);

float processNoise(Noise& noise);
// Returns noise in [-1, 1] scaled by mixLevel.
// White: flat spectrum. Pink: -3dB/octave via Kellet approximation.

} // namespace synth::noise
