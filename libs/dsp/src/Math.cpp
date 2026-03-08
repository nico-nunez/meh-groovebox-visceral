#include "dsp/Math.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace dsp::math {
float clampFastTan(float x) {
  return std::clamp(x, -0.95f * HALF_PI_F, 0.95f * HALF_PI_F);
}

float fastTan(float x) {
  const float x2 = x * x;
  const float x4 = x2 * x2;
  const float num = x * (1.0f - (1.0f / 9.0f) * x2 + (1.0f / 945.0f) * x4);
  const float den = 1.0f - (4.0f / 9.0f) * x2 + (1.0f / 63.0f) * x4;
  return num / den;
}

float fastTanh(float x) {
  if (x > 3.0f)
    return 1.0f;
  if (x < -3.0f)
    return -1.0f;
  return (x * (27.0f + x * x)) / (27.0f + 9.0f * x * x);
}

float fastSin(float x) {
  const float x2 = x * x;
  return x * (1.0f - x2 * (1.0f / 6.0f - x2 * (1.0f / 120.0f - x2 * (1.0f / 5040.0f))));
}

// NOTE(nico): Modern compilers often auto-vectorize std::exp2f using SIMD
// instructions (like AVX or SSE), which can sometimes outperform manual
// bit-hacks on modern CPUs.
float fastExp2(float x) {
  int32_t xi = static_cast<int32_t>(x);
  float xf = x - static_cast<float>(xi);

  float p = 1.0f + xf * (0.6931472f + xf * (0.2402265f + xf * (0.0555041f + xf * 0.0096181f)));

  int32_t bits;
  std::memcpy(&bits, &p, 4);

  bits += xi << 23;

  std::memcpy(&p, &bits, 4);
  return p;
}

float semitonesToFreqRatio(float x) {
  return fastExp2(x / 12);
}

// Fast approximation of log2(x)
float fastLog2(float val) {
  uint32_t i;
  std::memcpy(&i, &val, sizeof(i));

  float y = float(i) * 1.19209290e-7f; // 1 / 2^23

  return y - 126.94269504f;
}

// White noise PRNG
static uint32_t s_seed = 2463534242u;

uint32_t xorshift32() {
  s_seed ^= s_seed << 13;
  s_seed ^= s_seed >> 17;
  s_seed ^= s_seed << 5;
  return s_seed;
}

float randNoiseValue() {
  return static_cast<float>(static_cast<int32_t>(xorshift32())) * 4.6566129e-10f; // float → [-1, 1]
}

// Modulation/Envelope curve shapes
float expCurve(float t, float k) {
  return (std::exp(t * k) - 1.0f) / (std::exp(k) - 1.0f); // [0.0, 1.0]
}

float expCurve(float t, float k, float invDenom) {
  return (std::exp(t * k) - 1.0f) * invDenom; // [0.0, 1.0]
}

float calcPortamento(float t, float sampleRate) {
  if (t <= 0.0f || sampleRate == 0)
    return 0.0f;

  t = t / 1000.0f;
  return expf(-1.0f / (t * sampleRate));
}

} // namespace dsp::math
