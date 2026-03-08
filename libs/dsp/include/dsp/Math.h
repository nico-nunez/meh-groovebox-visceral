#pragma once

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

} // namespace dsp::math
