
#pragma once

#include "dsp/Buffers.h"
#include "dsp/Tempo.h"

#include <cstddef>

namespace dsp::fx::delay {
using dsp::buffers::StereoBuffer;
using tempo::Subdivision;

struct DelayState {
  StereoBuffer buffer{};
  size_t bufSize = 0;
  size_t writeHead = 0;
  uint32_t delaySamples = 0; // precomputed by recalcDelayTime, read directly in hot path
};

struct DelayFX {
  float time = 0.5f; // seconds, used when !tempoSync
  Subdivision subdivision = Subdivision::Quarter;
  bool tempoSync = true;
  float feedback = 0.4f;
  bool pingPong = false;
  float mix = 0.5f;
  bool enabled = false;
  uint32_t delaySamples = 0; // precomputed; hot path reads this
  DelayState state;          // circular buffer, read/write heads
};

void initDelayState(DelayState& state, float sampleRate);
void destroyDelayState(DelayState& state);

void processDelay(DelayFX& delay, StereoBuffer buf, size_t numSamples);
} // namespace dsp::fx::delay
