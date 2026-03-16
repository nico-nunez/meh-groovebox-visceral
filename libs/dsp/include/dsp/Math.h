#pragma once

#include <cmath>
#include <cstdint>
namespace dsp::math {

inline constexpr float PI_F = 3.1415927f;
inline constexpr float INV_PI_F = 1.0f / 3.1415927f;

inline constexpr double PI_DOUBLE = 3.141592653589793;
inline constexpr double INV_PI_DOUBLE = 3.141592653589793;

inline constexpr float TWO_PI_F = 2 * PI_F;
inline constexpr float HALF_PI_F = PI_F / 2.0f;

inline constexpr float SEMITONE_PER_OCTAVE = 1.0f / 12.0f;

// Pre-caluated value of 2^(1/12)
inline constexpr float SEMITONE_RATIO = 1.059463094f;

// Accepted argument value range of (-PI/2, PI/2)
// Callers responsibility to ensure arg is within range
// consider clamping at ±0.95 * (pi/2)
float fastTan(float x);

float fastTanh(float x);

float fastSin(float x);

float fastExp2(float x);

float semitonesToFreqRatio(float x);

float fastLog2(float val);

// White noise PRNG
uint32_t xorshift32();

float randNoiseValue();

// Modulation/Envelope curve shaping [0.0, 1.0]
float expCurve(float t, float k);
float expCurve(float t, float k, float invK);

float calcPortamento(float t, float sampleRate);

// ==============
// Unison
// ==============
// Equal-power pan law. panPosition: -1.0 (full left) to +1.0 (full right).
// At center (0.0), both gains ≈ 0.707 (−3dB).
inline void panToLR(float panPosition, float& gainL, float& gainR) {
  float angle = (panPosition + 1.0f) * (HALF_PI_F * 0.5f);
  gainL = std::cos(angle);
  gainR = std::sin(angle);
}

// 1/sqrt(N) gain compensation for stacked nearly-correlated signals.
inline float unisonGainComp(int voices) {
  return (voices <= 1) ? 1.0f : 1.0f / std::sqrt(static_cast<float>(voices));
}

// Compute N symmetric detune offsets in semitones from a total spread in cents.
// count=1 writes 0. count>1: [-spread/2, ..., +spread/2] cents → semitones.
void computeDetuneOffsets(float* out, int count, float detuneCents);

// Compute N symmetric pan positions in [-width, +width].
// count=1 writes 0 (center). width=1 spans full stereo field.
void computePanPositions(float* out, int count, float width);

inline float midiKeyTrackAmount(int midiNote, int rootNote = 69) { // default A4
  return static_cast<float>(midiNote - rootNote) * SEMITONE_PER_OCTAVE;
}

// Lagrange 4-point cubic interpolation.
// p0=x[-1], p1=x[0], p2=x[1], p3=x[2]; t in [0, 1) interpolates between p1 and p2.
// At t=0 → p1; at t→1 → p2.
inline float cubicInterp(float p0, float p1, float p2, float p3, float t) {
  return p1 + 0.5f * t *
                  (p2 - p0 +
                   t * (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3 + t * (3.0f * (p1 - p2) + p3 - p0)));
}

inline size_t nextPow2(size_t x) {
  if (x == 0)
    return 1;
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;
  return x + 1;
}
} // namespace dsp::math
