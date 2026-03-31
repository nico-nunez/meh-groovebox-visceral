#include "dsp/fx/Chorus.h"

#include "dsp/Buffers.h"
#include "dsp/Math.h"

#include <cstddef>

namespace dsp::fx::chorus {
namespace buf = dsp::buffers;

namespace {

void initChorusState(ChorusState& state, float sampleRate) {
  size_t requested = static_cast<size_t>(MAX_CHORUS_DELAY_MS / 1000.0f * sampleRate) + 1;

  buf::initStereoRingBuffer(state.buffer, requested); // rounds up to power of 2

  state.writeHead = 0;
  state.lastWetL = 0.0f;
  state.lastWetR = 0.0f;

  for (int v = 0; v < CHORUS_VOICES; v++) {
    float phaseL = static_cast<float>(v) / CHORUS_VOICES;
    float phaseR = (static_cast<float>(v) + 0.5f) / CHORUS_VOICES;

    state.lfoReL[v] = cosf(dsp::math::TWO_PI_F * phaseL);
    state.lfoImL[v] = sinf(dsp::math::TWO_PI_F * phaseL);
    state.lfoReR[v] = cosf(dsp::math::TWO_PI_F * phaseR);
    state.lfoImR[v] = sinf(dsp::math::TWO_PI_F * phaseR);
  }
}

} // anonymous namespace

void initChorus(ChorusFX& fx, float sampleRate) {
  initChorusState(fx.state, sampleRate);
  recalcChorusDerivedVals(fx, sampleRate);
}

void destroyChorus(ChorusFX& fx) {
  buf::destroyStereoRingBuffer(fx.state.buffer);
  fx.state.writeHead = 0;
  fx.state.lastWetL = 0.0f;
  fx.state.lastWetR = 0.0f;

  for (int v = 0; v < CHORUS_VOICES; v++) {
    fx.state.lfoReL[v] = 0.0f;
    fx.state.lfoImL[v] = 0.0f;
    fx.state.lfoReR[v] = 0.0f;
    fx.state.lfoImR[v] = 0.0f;
  }
}

void recalcChorusDerivedVals(ChorusFX& fx, float sampleRate) {
  fx.samplesPerMS = sampleRate / 1000.0f;
  fx.baseSamples = BASE_DELAY_MS * fx.samplesPerMS;
  fx.depthSamples = MAX_DEPTH_MS * fx.samplesPerMS * fx.depth;

  fx.dryGain = 1.0f - fx.mix;

  float w = dsp::math::TWO_PI_F * fx.rate / sampleRate;
  fx.cos_w = cosf(w);
  fx.sin_w = sinf(w);
}

void processChorus(ChorusFX& fx, buf::StereoBuffer buf, size_t numSamples) {
  auto& s = fx.state;

  const size_t mask = s.buffer.mask;
  const size_t cbSize = s.buffer.size;

  // Renormalize quadrature LFOs — magnitude drifts from floating-point accumulation.
  for (int v = 0; v < CHORUS_VOICES; v++) {
    const float magL = s.lfoReL[v] * s.lfoReL[v] + s.lfoImL[v] * s.lfoImL[v];
    if (magL > 0.0f) {
      const float invL = 1.0f / sqrtf(magL);
      s.lfoReL[v] *= invL;
      s.lfoImL[v] *= invL;
    }
    const float magR = s.lfoReR[v] * s.lfoReR[v] + s.lfoImR[v] * s.lfoImR[v];
    if (magR > 0.0f) {
      const float invR = 1.0f / sqrtf(magR);
      s.lfoReR[v] *= invR;
      s.lfoImR[v] *= invR;
    }
  }

  for (size_t i = 0; i < numSamples; i++) {
    const float dryL = buf.left[i];
    const float dryR = buf.right[i];

    // Write with feedback
    s.buffer.left[s.writeHead] = dryL + fx.feedback * s.lastWetL;
    s.buffer.right[s.writeHead] = dryR + fx.feedback * s.lastWetR;

    // Accumulate all voice taps
    float wetL = 0.0f, wetR = 0.0f;

    for (int v = 0; v < CHORUS_VOICES; v++) {
      // Step quadrature oscillator (no trig)
      float newReL = fx.cos_w * s.lfoReL[v] - fx.sin_w * s.lfoImL[v];
      float newImL = fx.sin_w * s.lfoReL[v] + fx.cos_w * s.lfoImL[v];
      s.lfoReL[v] = newReL;
      s.lfoImL[v] = newImL;

      float newReR = fx.cos_w * s.lfoReR[v] - fx.sin_w * s.lfoImR[v];
      float newImR = fx.sin_w * s.lfoReR[v] + fx.cos_w * s.lfoImR[v];
      s.lfoReR[v] = newReR;
      s.lfoImR[v] = newImR;

      // lfoIm = sin(phase) — use as LFO value in [-1, 1]
      const float offsetL = fx.baseSamples + fx.depthSamples * newImL;
      const float offsetR = fx.baseSamples + fx.depthSamples * newImR;

      // Cubic interpolated read — L
      float rpL = static_cast<float>(s.writeHead) - offsetL + static_cast<float>(cbSize);
      size_t i1L = static_cast<size_t>(rpL) & mask;
      size_t i0L = (i1L + cbSize - 1) & mask;
      size_t i2L = (i1L + 1) & mask;
      size_t i3L = (i1L + 2) & mask;
      float tL = rpL - floorf(rpL);
      wetL += dsp::math::cubicInterp(s.buffer.left[i0L],
                                     s.buffer.left[i1L],
                                     s.buffer.left[i2L],
                                     s.buffer.left[i3L],
                                     tL);

      // Cubic interpolated read — R
      float rpR = static_cast<float>(s.writeHead) - offsetR + static_cast<float>(cbSize);
      size_t i1R = static_cast<size_t>(rpR) & mask;
      size_t i0R = (i1R + cbSize - 1) & mask;
      size_t i2R = (i1R + 1) & mask;
      size_t i3R = (i1R + 2) & mask;
      float tR = rpR - floorf(rpR);
      wetR += dsp::math::cubicInterp(s.buffer.right[i0R],
                                     s.buffer.right[i1R],
                                     s.buffer.right[i2R],
                                     s.buffer.right[i3R],
                                     tR);
    }

    wetL *= fx.invVoices;
    wetR *= fx.invVoices;

    s.lastWetL = wetL;
    s.lastWetR = wetR;

    buf.left[i] = fx.mix * wetL + fx.dryGain * dryL;
    buf.right[i] = fx.mix * wetR + fx.dryGain * dryR;

    s.writeHead = (s.writeHead + 1) & mask;
  }
}

} // namespace dsp::fx::chorus
