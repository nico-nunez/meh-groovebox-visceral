#pragma once

#include "dsp/Buffers.h"

#include <cstdint>

namespace dsp::fx::phaser {

inline constexpr int MAX_PHASER_STAGES = 12;
inline constexpr float PHASER_MIN_FREQ = 100.0f;  // Hz — low end of notch sweep
inline constexpr float PHASER_MAX_FREQ = 4000.0f; // Hz — high end of notch sweep

struct PhaserState {
  float apL[MAX_PHASER_STAGES] = {}; // allpass lattice state, L channel
  float apR[MAX_PHASER_STAGES] = {}; // allpass lattice state, R channel

  // Quadrature oscillator — L at 0°, R at 90° for built-in stereo width.
  // L and R always sweep different notch positions simultaneously.
  float lfoReL = 1.0f;
  float lfoImL = 0.0f; // initialized to (cos 0°, sin 0°)
  float lfoReR = 0.0f;
  float lfoImR = 1.0f; // initialized to (cos 90°, sin 90°)

  // Last phased output, used for feedback path
  float lastOutputL = 0.0f;
  float lastOutputR = 0.0f;
};

struct PhaserFX {
  int8_t stages = 4;
  float rate = 0.5f;
  float depth = 1.0f;
  float feedback = 0.5f;
  float mix = 0.5f;
  bool enabled = false;

  // Precomputed on rate change (UpdateGroup::PhaserDerived):
  float cosW = 1.0f;
  float sinW = 0.0f;

  // Precomputed on depth change (UpdateGroup::PhaserDerived):
  float aHalfRange = 0.0f;

  // Set once at init (createEngine, depends on sampleRate):
  float aCenter = -0.782f;      // placeholder; overwritten at runtime
  float aHalfRangeMax = 0.205f; // placeholder; overwritten at runtime

  PhaserState state;
};

void initPhaser(PhaserFX& fx, float invSampleRate);
void destroyPhaser(PhaserFX& fx);

void recalcPhaseDerivedVals(PhaserFX& fx, float invSampleRate);

void processPhaser(PhaserFX& fx, buffers::StereoBuffer buf, size_t numSamples);

} // namespace dsp::fx::phaser
