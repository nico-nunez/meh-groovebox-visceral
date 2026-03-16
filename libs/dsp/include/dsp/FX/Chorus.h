#pragma once

#include "dsp/Buffers.h"

#include <cstddef>

namespace dsp::fx::chorus {
using dsp::buffers::StereoBuffer;

struct ChorusState {
  StereoBuffer buffer;
  size_t bufSize = 0; // needed for wrapping
  size_t writeHead = 0;
  float lfoPhase = 0.0f;
};

struct ChorusFX {
  float rate = 1.0f;  // LFO rate, Hz
  float depth = 0.5f; // modulation depth
  float mix = 0.5f;
  bool enabled = false;
  ChorusState state; // delay lines + LFO phase
};

void initChorusState(ChorusState& state, float sampleRate);
void destroyChorusState(ChorusState& state);

void processChorus(ChorusFX& chorus, StereoBuffer buf, size_t numSamples, float sampleRate);

} // namespace dsp::fx::chorus
