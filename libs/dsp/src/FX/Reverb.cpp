#include "dsp/FX/Reverb.h"
#include "dsp/Buffers.h"
#include "dsp/Math.h"

#include <cmath>
#include <cstddef>
#include <cstring>

namespace dsp::fx::reverb {

void initReverb(ReverbFX& fx, float sampleRate) {
  const float scale = sampleRate / DATTORRO_SR;
  auto& s = fx.state;

  // 1. Compute scaled line lengths
  s.len[APD1] = static_cast<size_t>(roundf(APD1_LEN_29K * scale));
  s.len[APD2] = static_cast<size_t>(roundf(APD2_LEN_29K * scale));
  s.len[APD3] = static_cast<size_t>(roundf(APD3_LEN_29K * scale));
  s.len[APD4] = static_cast<size_t>(roundf(APD4_LEN_29K * scale));
  s.len[MOD_A] = static_cast<size_t>(roundf(MOD_A_LEN_29K * scale));
  s.len[DLY1_A] = static_cast<size_t>(roundf(DLY1_A_LEN_29K * scale));
  s.len[APF_A] = static_cast<size_t>(roundf(APF_A_LEN_29K * scale));
  s.len[DLY2_A] = static_cast<size_t>(roundf(DLY2_A_LEN_29K * scale));
  s.len[MOD_B] = static_cast<size_t>(roundf(MOD_B_LEN_29K * scale));
  s.len[DLY1_B] = static_cast<size_t>(roundf(DLY1_B_LEN_29K * scale));
  s.len[APF_B] = static_cast<size_t>(roundf(APF_B_LEN_29K * scale));
  s.len[DLY2_B] = static_cast<size_t>(roundf(DLY2_B_LEN_29K * scale));

  const size_t modPad = static_cast<size_t>(MAX_MOD_DEPTH_SAMPLES) + 2;
  s.modNominal[0] = s.len[MOD_A];
  s.modNominal[1] = s.len[MOD_B];
  s.len[MOD_A] = s.modNominal[0] + modPad;
  s.len[MOD_B] = s.modNominal[1] + modPad;

  const size_t loopA = s.modNominal[0] + s.len[DLY1_A] + s.len[APF_A] + s.len[DLY2_A];
  const size_t loopB = s.modNominal[1] + s.len[DLY1_B] + s.len[APF_B] + s.len[DLY2_B];
  fx.loopTime = static_cast<float>(loopA + loopB) / (2.0f * sampleRate);

  // 2. Compute contiguous offsets + total allocation size
  size_t total = 0;
  for (int i = 0; i < NUM_DATTORRO_LINES; i++) {
    s.offset[i] = total;
    total += s.len[i];
  }

  // 3. Flat allocation — zero-initialized
  s.buf = new float[total]();

  // 4. Write heads + tap offsets
  memset(s.head, 0, sizeof(s.head));

  // Output tap offsets — paper Table 1 values (at 29761 Hz), scaled to sample rate.
  // tap[0]=266, tap[1]=2974, tap[2]=1913, tap[3]=1996, tap[4]=187, tap[5]=1066, tap[6]=2974
  // tap[1] and tap[6] are the same paper offset (2974) used on different lines — explicit for clarity.
  // ASPIRATIONAL: verify all 7 offsets against Dattorro 1997 Table 1 before use.
  s.tap[0] = static_cast<size_t>(roundf(266.0f * scale));
  s.tap[1] = static_cast<size_t>(roundf(2974.0f * scale));
  s.tap[2] = static_cast<size_t>(roundf(1913.0f * scale));
  s.tap[3] = static_cast<size_t>(roundf(1996.0f * scale));
  s.tap[4] = static_cast<size_t>(roundf(187.0f * scale));
  s.tap[5] = static_cast<size_t>(roundf(1066.0f * scale));
  s.tap[6] = s.tap[1]; // same paper offset as tap[1]

  // 5. Pre-delay buffer (separate, param-controlled capacity)
  s.preDelayLen = static_cast<size_t>(sampleRate * 0.1f) + 1;
  s.preDelayBuf = new float[s.preDelayLen]();
  s.preDelayHead = 0;

  // 6. Filter state
  s.bwState = 0.0f;
  s.dampA = 0.0f;
  s.dampB = 0.0f;
  s.lowDampA = 0.0f;
  s.lowDampB = 0.0f;

  // 7. Modulation LFO — A at 0°, B at 90°
  s.lfoAre = 1.0f;
  s.lfoAim = 0.0f;
  s.lfoBre = 0.0f;
  s.lfoBim = 1.0f;

  recalcReverbDerivedVals(fx, sampleRate);
}

void destroyReverb(ReverbFX& fx) {
  delete[] fx.state.buf;
  fx.state.buf = nullptr;
  delete[] fx.state.preDelayBuf;
  fx.state.preDelayBuf = nullptr;
}

void recalcReverbDerivedVals(ReverbFX& fx, float sampleRate) {
  fx.dampCoeff = 1.0f - fx.damping;
  fx.lowDampCoeff = fx.lowDamping * 0.03f;

  fx.preDelaySamples = fx.preDelay * sampleRate / 1000.0f;
  fx.modDepthSamples = fx.modDepth * MAX_MOD_DEPTH_SAMPLES;

  const float w = 2.0f * dsp::math::PI_F * fx.modRate / sampleRate;
  fx.cosW = cosf(w);
  fx.sinW = sinf(w);

  fx.decay = powf(10.0f, -3.0f * fx.loopTime / fx.decaySeconds);
  fx.decayDiffusion = fx.decay * TANK_DIFF_COEFF;
}

void processReverb(ReverbFX& fx, buffers::StereoBuffer buf, size_t numSamples) {
  auto& s = fx.state;

  // Renormalize quadrature LFOs — magnitude drifts from floating-point accumulation.
  // Once per buffer is sufficient; sqrtf is not in the hot path.
  {
    const float magA = s.lfoAre * s.lfoAre + s.lfoAim * s.lfoAim;
    if (magA > 0.0f) {
      const float invA = 1.0f / sqrtf(magA);
      s.lfoAre *= invA;
      s.lfoAim *= invA;
    }
    const float magB = s.lfoBre * s.lfoBre + s.lfoBim * s.lfoBim;
    if (magB > 0.0f) {
      const float invB = 1.0f / sqrtf(magB);
      s.lfoBre *= invB;
      s.lfoBim *= invB;
    }
  }

  // Precompute loop-invariants from fx params
  const float bwInv = 1.0f - fx.bandwidth;
  const float coeff12 = fx.diffusion;
  const float coeff34 = fx.diffusion * DIFF_INNER_RATIO;
  const float dampFeed = 1.0f - fx.dampCoeff; // LP feedback amount; named dampFeed to avoid
                                              // shadowing the fx.damping user param
  const float modDepth = fx.modDepthSamples;
  const float dry = 1.0f - fx.mix;

  // Output tap offsets (paper Table 1 values, scaled to sample rate at init)
  // These are fixed offsets from each line's write head. Verify against Dattorro 1997.
  // Stored as size_t constants scaled at init time (not shown here — add to DattorroState
  // as tap[NUM_TAPS] arrays, initialized in initReverb alongside len[]).

  for (size_t i = 0; i < numSamples; i++) {
    // 1. Stereo sum to mono
    const float dryL = buf.left[i];
    const float dryR = buf.right[i];
    float mono = (dryL + dryR) * 0.5f;

    // 2. Pre-delay
    if (fx.preDelaySamples >= 1.0f) {
      const size_t pd = static_cast<size_t>(fx.preDelaySamples);
      const size_t ri = (s.preDelayHead + s.preDelayLen - pd) % s.preDelayLen;
      const float delayed = s.preDelayBuf[ri];
      s.preDelayBuf[s.preDelayHead] = mono;
      s.preDelayHead = (s.preDelayHead + 1) % s.preDelayLen;
      mono = delayed;
    }

    // 3. Input bandwidth LP
    s.bwState = fx.bandwidth * mono + bwInv * s.bwState;
    mono = s.bwState;

    // 4. Input diffusion — 4 series Schroeder allpass filters
    // Schroeder allpass: v = input + (-coeff) * d[read]; output = d[read] + coeff * v; d[write] = v
    // Helper lambda conceptually (inline in actual code):
    // auto schroeder = [&](DattorroLine line, float coeff, float x) -> float { ... };
    {
      float* b = s.buf + s.offset[APD1];
      const size_t len = s.len[APD1];
      size_t& head = s.head[APD1];
      const float d = b[head];
      const float v = mono + (-coeff12) * d;
      b[head] = v;
      head = (head + 1) % len;
      mono = d + coeff12 * v;
    }
    {
      float* b = s.buf + s.offset[APD2];
      const size_t len = s.len[APD2];
      size_t& head = s.head[APD2];
      const float d = b[head];
      const float v = mono + (-coeff12) * d;
      b[head] = v;
      head = (head + 1) % len;
      mono = d + coeff12 * v;
    }
    {
      float* b = s.buf + s.offset[APD3];
      const size_t len = s.len[APD3];
      size_t& head = s.head[APD3];
      const float d = b[head];
      const float v = mono + (-coeff34) * d;
      b[head] = v;
      head = (head + 1) % len;
      mono = d + coeff34 * v;
    }
    {
      float* b = s.buf + s.offset[APD4];
      const size_t len = s.len[APD4];
      size_t& head = s.head[APD4];
      const float d = b[head];
      const float v = mono + (-coeff34) * d;
      b[head] = v;
      head = (head + 1) % len;
      mono = d + coeff34 * v;
    }

    // 5. Cross-coupled tank
    // Read last outputs from terminal delay lines (before any writes this sample)
    const float lastA = s.buf[s.offset[DLY2_A] + s.head[DLY2_A]]; // oldest = current head
    const float lastB = s.buf[s.offset[DLY2_B] + s.head[DLY2_B]];

    // Loop A: seeded from diffusedInput + decay * lastB
    float sigA = mono + fx.decay * lastB;
    {
      float* b = s.buf + s.offset[MOD_A];
      const size_t len = s.len[MOD_A];
      size_t& head = s.head[MOD_A];
      const size_t pad = len - s.modNominal[0];

      const float frac = modDepth * s.lfoAim;              // bipolar, ∈ [-modDepth, +modDepth]
      const int frac_int = static_cast<int>(floorf(frac)); // integer floor
      const float t = frac - static_cast<float>(frac_int); // fractional remainder ∈ [0,1)
                                                           //
      const size_t ri = static_cast<size_t>(
          (static_cast<int>(head) + static_cast<int>(pad) - frac_int) % static_cast<int>(len));
      const size_t ri2 = (ri + 1) % len;
      const float dRead = b[ri] + t * (b[ri2] - b[ri]);

      const float v = sigA + (-fx.decayDiffusion) * dRead;
      b[head] = v;
      head = (head + 1) % len;
      sigA = dRead + fx.decayDiffusion * v;
    }
    {
      // DLY1_A — fixed integer read (full delay, read oldest = current head)
      float* b = s.buf + s.offset[DLY1_A];
      const size_t len = s.len[DLY1_A];
      size_t& head = s.head[DLY1_A];
      const float d = b[head];
      b[head] = sigA;
      head = (head + 1) % len;
      sigA = d;
    }
    {
      // Damping LP
      s.dampA = fx.dampCoeff * sigA + dampFeed * s.dampA;
      sigA = s.dampA;

      // HP damping (low-freq attenuation)
      s.lowDampA += fx.lowDampCoeff * (sigA - s.lowDampA);
      sigA -= s.lowDampA;
    }
    {
      // APF_A — Schroeder allpass
      float* b = s.buf + s.offset[APF_A];
      const size_t len = s.len[APF_A];
      size_t& head = s.head[APF_A];
      const float d = b[head];
      const float v = sigA + (-fx.decay) * d;
      b[head] = v;
      head = (head + 1) % len;
      sigA = d + fx.decay * v;
    }
    {
      // DLY2_A — fixed integer read
      float* b = s.buf + s.offset[DLY2_A];
      const size_t len = s.len[DLY2_A];
      size_t& head = s.head[DLY2_A];
      b[head] = sigA; // write first (lastA was read before this loop)
      head = (head + 1) % len;
      // sigA now holds the value that will feed back as lastA next sample
    }

    // Loop B: seeded from diffusedInput + decay * lastA (symmetric, mirror of A)
    float sigB = mono + fx.decay * lastA;
    {
      // Modulated allpass B — same formula as A, using lfoBim (90° offset for decorrelation)
      float* b = s.buf + s.offset[MOD_B];
      const size_t len = s.len[MOD_B];
      size_t& head = s.head[MOD_B];
      const size_t pad = len - s.modNominal[1];

      const float frac = modDepth * s.lfoBim;
      const int frac_int = static_cast<int>(floorf(frac));
      const float t = frac - static_cast<float>(frac_int);

      const size_t ri = static_cast<size_t>(
          (static_cast<int>(head) + static_cast<int>(pad) - frac_int) % static_cast<int>(len));
      const size_t ri2 = (ri + 1) % len;
      const float dRead = b[ri] + t * (b[ri2] - b[ri]);

      const float v = sigB + (-fx.decayDiffusion) * dRead;
      b[head] = v;
      head = (head + 1) % len;
      sigB = dRead + fx.decayDiffusion * v;
    }
    {
      float* b = s.buf + s.offset[DLY1_B];
      const size_t len = s.len[DLY1_B];
      size_t& head = s.head[DLY1_B];
      const float d = b[head];
      b[head] = sigB;
      head = (head + 1) % len;
      sigB = d;
    }
    {
      s.dampB = fx.dampCoeff * sigB + dampFeed * s.dampB;
      sigB = s.dampB;

      s.lowDampB += fx.lowDampCoeff * (sigB - s.lowDampB);
      sigB -= s.lowDampB;
    }
    {
      float* b = s.buf + s.offset[APF_B];
      const size_t len = s.len[APF_B];
      size_t& head = s.head[APF_B];
      const float d = b[head];
      const float v = sigB + (-fx.decay) * d;
      b[head] = v;
      head = (head + 1) % len;
      sigB = d + fx.decay * v;
    }
    {
      float* b = s.buf + s.offset[DLY2_B];
      const size_t len = s.len[DLY2_B];
      size_t& head = s.head[DLY2_B];
      b[head] = sigB;
      head = (head + 1) % len;
    }

    // 6. Step LFO oscillators (quadrature, no trig)
    {
      const float re = s.lfoAre * fx.cosW - s.lfoAim * fx.sinW;
      const float im = s.lfoAre * fx.sinW + s.lfoAim * fx.cosW;
      s.lfoAre = re;
      s.lfoAim = im;
    }
    {
      const float re = s.lfoBre * fx.cosW - s.lfoBim * fx.sinW;
      const float im = s.lfoBre * fx.sinW + s.lfoBim * fx.cosW;
      s.lfoBre = re;
      s.lfoBim = im;
    }

    // 7. Output taps (Dattorro 1997, Table 1)
    // tapRead(line, offset): reads `offset` samples back from line's write head.
    // ri = (head[line] + len[line] - offset) % len[line]
    // Positive = add to output, negative = subtract.
    // R output is L with A↔B swapped — all 7 tap[] offsets are shared between L and R.
    // ASPIRATIONAL: tap[] values must be verified against Dattorro 1997 Table 1 before use.
    auto tapRead = [&](DattorroLine line, size_t offset) -> float {
      const float* tb = s.buf + s.offset[line];
      const size_t tl = s.len[line];
      return tb[(s.head[line] + tl - offset) % tl];
    };

    float wetL = +tapRead(DLY2_A, s.tap[0]) + tapRead(DLY2_A, s.tap[1]) - tapRead(APF_B, s.tap[2]) +
                 tapRead(DLY2_B, s.tap[3]) - tapRead(DLY1_B, s.tap[4]) - tapRead(APF_A, s.tap[5]) -
                 tapRead(DLY1_A, s.tap[6]);

    float wetR = +tapRead(DLY2_B, s.tap[0]) + tapRead(DLY2_B, s.tap[1]) - tapRead(APF_B, s.tap[2]) +
                 tapRead(DLY2_A, s.tap[3]) - tapRead(DLY1_A, s.tap[4]) - tapRead(APF_A, s.tap[5]) -
                 tapRead(DLY1_B, s.tap[6]);

    // 8. DC blocking — one-pole high-pass on wet signal before mix.
    // y[n] = x[n] - x[n-1] + R * y[n-1],  R = 0.9997f  (fc ≈ 8 Hz at 48kHz)
    // Prevents DC offset from accumulating in the tank (high decay + DC-biased input)
    // from reaching the output or interacting with downstream clipping/limiting.
    {
      const float R = 0.9997f;
      const float rawL = wetL;
      const float rawR = wetR;
      wetL = rawL - s.dcPrevL + R * s.dcOutL;
      wetR = rawR - s.dcPrevR + R * s.dcOutR;
      s.dcPrevL = rawL;
      s.dcPrevR = rawR;
      s.dcOutL = wetL;
      s.dcOutR = wetR;
    }

    wetL *= 0.2f;
    wetR *= 0.2f;

    // 9. Dry/wet mix
    buf.left[i] = fx.mix * wetL + dry * dryL;
    buf.right[i] = fx.mix * wetR + dry * dryR;
  }
}

} // namespace dsp::fx::reverb
