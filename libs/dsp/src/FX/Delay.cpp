#include "dsp/FX/Delay.h"
#include "dsp/Buffers.h"
#include "dsp/Math.h"

#include <cmath>
#include <cstddef>

namespace dsp::fx::delay {

void recalcTargetDelaySamples(DelayFX& fx, float bpm, float sampleRate) {
  fx.targetDelaySamples = fx.tempoSync
                              ? tempo::subdivisionPeriodSeconds(fx.subdivision, bpm) * sampleRate
                              : fx.time * sampleRate;
}

void recalcDerivedDampCoeff(DelayFX& fx) {
  fx.dampCoeff = 1.0f - fx.damping;
  fx.hpCoeff = 1.0f - fx.hpDamping;
}

void initDelay(DelayFX& fx, float bpm, float sampleRate) {
  size_t requested = static_cast<size_t>(MAX_DELAY_SECONDS * sampleRate) + 1;
  dsp::buffers::initStereoRingBuffer(fx.state.buffer, requested);
  fx.state.writeHead = 0;
  fx.state.dampZ1L = 0.0f;
  fx.state.dampZ1R = 0.0f;
  fx.state.hpZ1L = 0.0f;
  fx.state.hpZ1R = 0.0f;

  // Smoothing coefficient for ~20ms time constant
  fx.smoothCoeff = 1.0f - expf(-1.0f / (0.020f * sampleRate));

  recalcTargetDelaySamples(fx, bpm, sampleRate);

  fx.state.currentDelaySamples = fx.targetDelaySamples; // hard-init
                                                        //
  recalcDerivedDampCoeff(fx);
}

void destroyDelay(DelayFX& fx) {
  dsp::buffers::destroyStereoRingBuffer(fx.state.buffer);
}

void processDelay(DelayFX& fx, StereoBuffer buf, size_t numSamples) {
  auto& s = fx.state;
  auto& b = fx.state.buffer;

  const float fb = fx.feedback;
  const float dry = 1.0f - fx.mix;
  const float coeff = fx.smoothCoeff;
  const float tgt = fx.targetDelaySamples;
  const float damp = fx.damping;     // = 1 - dampCoeff
  const float dCoeff = fx.dampCoeff; // = 1 - damping
  const float hpCoeff = fx.hpCoeff;  // = 1 - hpDamping
  const bool pp = fx.pingPong;

  for (size_t i = 0; i < numSamples; i++) {
    const float dryL = buf.left[i];
    const float dryR = buf.right[i];

    // Smooth read position toward target (runs unconditionally)
    s.currentDelaySamples += coeff * (tgt - s.currentDelaySamples);
    const float offset = s.currentDelaySamples;

    // Write: dry input + LP+HP filtered feedback from previous cycle.
    // hpZ1L/R is the terminal filtered value (HP output) stored last sample.
    b.left[s.writeHead] = pp ? dryL + fb * s.hpZ1R : dryL + fb * s.hpZ1L;
    b.right[s.writeHead] = pp ? dryR + fb * s.hpZ1L : dryR + fb * s.hpZ1R;

    // Cubic interpolated read — L and R share the same offset
    float rpL = static_cast<float>(s.writeHead) - offset + static_cast<float>(b.size);
    size_t i1L = static_cast<size_t>(rpL) & b.mask;
    size_t i0L = (i1L + b.size - 1) & b.mask;
    size_t i2L = (i1L + 1) & b.mask;
    size_t i3L = (i1L + 2) & b.mask;
    float tL = rpL - floorf(rpL);
    const float wetL =
        dsp::math::cubicInterp(b.left[i0L], b.left[i1L], b.left[i2L], b.left[i3L], tL);

    float rpR = static_cast<float>(s.writeHead) - offset + static_cast<float>(b.size);
    size_t i1R = static_cast<size_t>(rpR) & b.mask;
    size_t i0R = (i1R + b.size - 1) & b.mask;
    size_t i2R = (i1R + 1) & b.mask;
    size_t i3R = (i1R + 2) & b.mask;
    float tR = rpR - floorf(rpR);
    const float wetR =
        dsp::math::cubicInterp(b.right[i0R], b.right[i1R], b.right[i2R], b.right[i3R], tR);

    // LP filter (one-pole low-pass): attenuates HF each echo.
    // Save previous LP output as a local before overwriting — used by HP below.
    // At damping=0: dCoeff=1, damp=0 → lpOut = wet (pass-through)
    const float prevLpL = s.dampZ1L;
    const float prevLpR = s.dampZ1R;
    s.dampZ1L = dCoeff * wetL + damp * s.dampZ1L;
    s.dampZ1R = dCoeff * wetR + damp * s.dampZ1R;

    // HP filter (one-pole high-pass): attenuates LF each echo, applied after LP.
    // y[n] = hpCoeff * (y[n-1] + lpOut[n] - lpOut[n-1])
    // At hpDamping=0: hpCoeff=1 → hpOut = lpOut (pass-through, verified algebraically)
    // At hpDamping=1: hpCoeff=0 → hpOut = 0 (complete block)
    s.hpZ1L = hpCoeff * (s.hpZ1L + s.dampZ1L - prevLpL);
    s.hpZ1R = hpCoeff * (s.hpZ1R + s.dampZ1R - prevLpR);

    buf.left[i] = fx.mix * wetL + dry * dryL;
    buf.right[i] = fx.mix * wetR + dry * dryR;

    s.writeHead = (s.writeHead + 1) & b.mask;
  }
}

} // namespace dsp::fx::delay
