
#pragma once

#include "dsp/Buffers.h"
#include "dsp/Tempo.h"

#include <cstddef>

namespace dsp::fx::delay {
using dsp::buffers::StereoBuffer;
using dsp::buffers::StereoRingBuffer;
using tempo::Subdivision;

inline constexpr float MAX_DELAY_SECONDS = 4.0f;

struct DelayState {
  StereoRingBuffer buffer; // power-of-2 buffer; size and mask owned here
  size_t writeHead = 0;
  float currentDelaySamples = 0.0f; // smoothed read position

  // LP filter state (one-pole low-pass in feedback path)
  float dampZ1L = 0.0f;
  float dampZ1R = 0.0f;

  // HP filter state (one-pole high-pass, applied after LP)
  // Holds the previous HP output; used as feedback value in the write step.
  float hpZ1L = 0.0f;
  float hpZ1R = 0.0f;
};

struct DelayFX {
  float time = 0.5f;
  Subdivision subdivision = Subdivision::Quarter;
  bool tempoSync = true;
  float feedback = 0.4f;
  float damping = 0.0f;   // [0, 1]; LP in feedback — 0 = clean, 1 = HF muted
  float hpDamping = 0.0f; // [0, 1]; HP in feedback — 0 = clean, 1 = LF muted
  bool pingPong = false;
  float mix = 0.5f;
  bool enabled = false;

  // Set by DelayDerived on time/tempoSync/BPM change
  float targetDelaySamples = 0.0f;

  // Precomputed at init; not user-settable.
  // One-pole smoothing coefficient for delay time changes.
  // Time constant ~20ms: coeff = 1 - exp(-1 / (0.020 * sampleRate))
  float smoothCoeff = 0.0f;

  // Set by DelayDamping on damping/hpDamping change
  float dampCoeff = 1.0f; // = 1 - damping
  float hpCoeff = 1.0f;   // = 1 - hpDamping

  DelayState state;
};

void initDelay(DelayFX& fx, float bpm, float sampleRate);
void destroyDelay(DelayFX& fx);

void recalcTargetDelaySamples(DelayFX& fx, float bpm, float sampleRate);
void recalcDerivedDampCoeff(DelayFX& fx);

void processDelay(DelayFX& fx, StereoBuffer buf, size_t numSamples);

} // namespace dsp::fx::delay
