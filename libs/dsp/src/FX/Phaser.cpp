#include "dsp/FX/Phaser.h"
#include "dsp/Buffers.h"
#include "dsp/Math.h"

#include <cmath>
#include <cstddef>

namespace dsp::fx::phaser {

namespace {
void initPhaserState(PhaserState& state) {
  // Default member initializers in the struct handle zeroing.
  // Explicit reset for clarity on re-init:
  for (int i = 0; i < MAX_PHASER_STAGES; i++) {
    state.apL[i] = 0.0f;
    state.apR[i] = 0.0f;
  }
  state.lfoReL = 1.0f;
  state.lfoImL = 0.0f; // L phase: 0°
  state.lfoReR = 0.0f;
  state.lfoImR = 1.0f; // R phase: 90°
  state.lastOutputL = 0.0f;
  state.lastOutputR = 0.0f;
}

} // anonymous namespace

void initPhaser(PhaserFX& fx, float sampleRate) {
  const float invSampleRate = 1.0f / sampleRate;

  initPhaserState(fx.state);

  float tanLo = tanf(dsp::math::PI_F * PHASER_MIN_FREQ * invSampleRate);
  float tanHi = tanf(dsp::math::PI_F * PHASER_MAX_FREQ * invSampleRate);
  float aLo = (tanLo - 1.0f) / (tanLo + 1.0f);
  float aHi = (tanHi - 1.0f) / (tanHi + 1.0f);

  fx.aCenter = (aLo + aHi) * 0.5f;
  fx.aHalfRangeMax = (aHi - aLo) * 0.5f;

  recalcPhaseDerivedVals(fx, invSampleRate);
}

void destroyPhaser(PhaserFX& fx) {
  initPhaserState(fx.state);
}

void recalcPhaseDerivedVals(PhaserFX& fx, float invSampleRate) {
  // Initial PhaserDerived values (same computation as the UpdateGroup handler)
  float omega = dsp::math::TWO_PI_F * fx.rate * invSampleRate;
  fx.cosW = cosf(omega);
  fx.sinW = sinf(omega);
  fx.aHalfRange = fx.aHalfRangeMax * fx.depth;
}

void processPhaser(PhaserFX& fx, buffers::StereoBuffer buf, size_t numSamples) {
  auto& s = fx.state;
  const float dryGain = 1.0f - fx.mix;
  const int stages = static_cast<int>(fx.stages);

  // Renormalize LFO pairs once per block (prevents long-term floating-point drift)
  float magL = sqrtf(s.lfoReL * s.lfoReL + s.lfoImL * s.lfoImL);
  s.lfoReL /= magL;
  s.lfoImL /= magL;
  float magR = sqrtf(s.lfoReR * s.lfoReR + s.lfoImR * s.lfoImR);
  s.lfoReR /= magR;
  s.lfoImR /= magR;

  for (size_t i = 0; i < numSamples; i++) {
    // Step quadrature oscillators (no trig per sample)
    float reL = fx.cosW * s.lfoReL - fx.sinW * s.lfoImL;
    float imL = fx.sinW * s.lfoReL + fx.cosW * s.lfoImL;
    s.lfoReL = reL;
    s.lfoImL = imL;

    float reR = fx.cosW * s.lfoReR - fx.sinW * s.lfoImR;
    float imR = fx.sinW * s.lfoReR + fx.cosW * s.lfoImR;
    s.lfoReR = reR;
    s.lfoImR = imR;

    // Allpass coefficient — no division per sample
    const float aL = fx.aCenter + fx.aHalfRange * imL;
    const float aR = fx.aCenter + fx.aHalfRange * imR;

    // Save dry; mix feedback into input
    const float dryL = buf.left[i];
    const float dryR = buf.right[i];
    float xL = dryL + fx.feedback * s.lastOutputL;
    float xR = dryR + fx.feedback * s.lastOutputR;

    // Allpass chain — lattice form, one state per stage
    for (int st = 0; st < stages; st++) {
      float vL = xL - aL * s.apL[st];
      float yL = aL * vL + s.apL[st];
      s.apL[st] = vL;
      xL = yL;

      float vR = xR - aR * s.apR[st];
      float yR = aR * vR + s.apR[st];
      s.apR[st] = vR;
      xR = yR;
    }

    // xL/xR are now the phased output
    s.lastOutputL = xL;
    s.lastOutputR = xR;

    buf.left[i] = fx.mix * xL + dryGain * dryL;
    buf.right[i] = fx.mix * xR + dryGain * dryR;
  }
}

} // namespace dsp::fx::phaser
