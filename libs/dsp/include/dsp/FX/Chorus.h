#pragma once

#include "dsp/Buffers.h"

#include <cstddef>

namespace dsp::fx::chorus {
using dsp::buffers::StereoBuffer;
using dsp::buffers::StereoRingBuffer;

inline constexpr float MAX_CHORUS_DELAY_MS = 30.0f;
inline constexpr float BASE_DELAY_MS = 7.0f; // LFO center point
inline constexpr float MAX_DEPTH_MS = 7.0f;  // max sweep at depth=1
inline constexpr uint8_t CHORUS_VOICES = 4;  // fixed; SoA layout enables SIMD

struct ChorusState {
  StereoRingBuffer buffer;
  size_t writeHead = 0;

  // Quadrature oscillator state — one (re, im) pair per voice per channel.
  // Initialized in initChorusState with evenly distributed phase offsets.
  // L voices:  0/4, 1/4, 2/4, 3/4 of a full cycle
  // R voices: offset by 0.5/CHORUS_VOICES → 0.125, 0.375, 0.625, 0.875
  // This gives 8 evenly-distributed phases across L+R for maximum decorrelation.
  float lfoReL[CHORUS_VOICES] = {};
  float lfoImL[CHORUS_VOICES] = {};
  float lfoReR[CHORUS_VOICES] = {};
  float lfoImR[CHORUS_VOICES] = {};

  // Feedback: store last wet output per channel
  float lastWetL = 0.0f;
  float lastWetR = 0.0f;
};

struct ChorusFX {
  float rate = 1.0f;  // LFO rate, Hz
  float depth = 0.5f; // modulation depth [0, 1]
  float mix = 0.5f;
  float feedback = 0.0f; // [0, ~0.5] — higher values ring; keep max < 1.0
  bool enabled = false;

  // Precomputed on rate change (UpdateGroup::ChorusDerived)
  float cos_w = 1.0f;
  float sin_w = 0.0f;

  ChorusState state;

  float invVoices = 1.0f / static_cast<float>(CHORUS_VOICES);

  float samplesPerMS = 48000.0f;
  float baseSamples = BASE_DELAY_MS * samplesPerMS;
  float depthSamples = MAX_DEPTH_MS * samplesPerMS * depth;
  float dryGain = 1.0f - mix;
};

void initChorusState(ChorusState& state, float sampleRate);
void destroyChorusState(ChorusState& state);

void recalcChorusDerivedVals(ChorusFX& fx, float sampleRate);

void processChorus(ChorusFX& chorus, StereoBuffer buf, size_t numSamples);

} // namespace dsp::fx::chorus
