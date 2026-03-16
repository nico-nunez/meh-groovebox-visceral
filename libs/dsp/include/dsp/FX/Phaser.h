#pragma once

#include "dsp/Buffers.h"

#include <cstdint>

namespace dsp::fx::phaser {

constexpr int MAX_PHASER_STAGES = 12;

struct PhaserState {
  // TODO: should this be a StereoBuffer?
  float apL[MAX_PHASER_STAGES] = {};
  float apR[MAX_PHASER_STAGES] = {};
  float lfoPhase = 0.0f;
  float feedbackL = 0.0f; // last output sample, fed back to input
  float feedbackR = 0.0f;
};

struct PhaserFX {
  int8_t stages = 4; // allpass stage count
  float rate = 0.5f;
  float depth = 1.0f;
  float feedback = 0.5f;
  float mix = 0.5f;
  bool enabled = false;
  PhaserState state;
};

void initPhaserState(PhaserState& state, float sampleRate);
void destroyPhaserState(PhaserState& state);

void processPhaser(PhaserFX& phaser,
                   buffers::StereoBuffer buf,
                   size_t numSamples,
                   float sampleRate);
} // namespace dsp::fx::phaser
