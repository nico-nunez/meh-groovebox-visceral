#pragma once

#include "dsp/Buffers.h"

#include <cstddef>

namespace dsp::fx::reverb {

struct DattorroState {
  // todo...
};

struct ReverbFX {
  float preDelay = 0.0f; // ms
  float decay = 0.75f;   // T60 decay time
  float damping = 0.5f;
  float bandwidth = 0.75f; // input bandwidth
  float mix = 0.5f;
  bool enabled = false;
  DattorroState state; // full plate network, fixed topology
};

void initReverbState(DattorroState& state, float sampleRate);
void destroyReverbState(DattorroState& state);

void processReverb(ReverbFX& reverb,
                   buffers::StereoBuffer buf,
                   size_t numSamples,
                   float sampleRate);
} // namespace dsp::fx::reverb
