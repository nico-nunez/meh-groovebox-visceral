#pragma once

#include <cstdint>
#include <cstring>

namespace dsp::tempo {

enum Subdivision : uint8_t {
  // Straight
  Whole,        // 1/1  — 4.0 beats
  Half,         // 1/2  — 2.0 beats
  Quarter,      // 1/4  — 1.0 beat
  Eighth,       // 1/8  — 0.5 beats
  Sixteenth,    // 1/16 — 0.25 beats
  ThirtySecond, // 1/32 — 0.125 beats
  SixtyFourth,  // 1/64 — 0.0625 beats

  // Dotted (1.5×)
  DottedHalf,      // 3.0 beats
  DottedQuarter,   // 1.5 beats
  DottedEighth,    // 0.75 beats
  DottedSixteenth, // 0.375 beats

  // Triplet (2/3)
  HalfTriplet,      // 4/3 beats
  QuarterTriplet,   // 2/3 beat
  EighthTriplet,    // 1/3 beat
  SixteenthTriplet, // 1/6 beat
  Unknown,
};

// Beat fractions indexed by Subdivision ordinal
constexpr float SUBDIVISION_BEATS[] = {
    4.0f,
    2.0f,
    1.0f,
    0.5f,
    0.25f,
    0.125f,
    0.0625f, // straight
    3.0f,
    1.5f,
    0.75f,
    0.375f, // dotted
    4.0f / 3.0f,
    2.0f / 3.0f,
    1.0f / 3.0f,
    1.0f / 6.0f, // triplet
};

inline float subdivisionToBeats(Subdivision s) {
  return SUBDIVISION_BEATS[static_cast<uint8_t>(s)];
}

inline float subdivisionPeriodSeconds(Subdivision s, float bpm) {
  return (60.0f / bpm) * subdivisionToBeats(s);
}

inline const char* subdivisionToString(Subdivision s) {
  switch (s) {
  case Subdivision::Whole:
    return "1/1";
  case Subdivision::Half:
    return "1/2";
  case Subdivision::Quarter:
    return "1/4";
  case Subdivision::Eighth:
    return "1/8";
  case Subdivision::Sixteenth:
    return "1/16";
  case Subdivision::ThirtySecond:
    return "1/32";
  case Subdivision::SixtyFourth:
    return "1/64";
  case Subdivision::DottedHalf:
    return "d1/2";
  case Subdivision::DottedQuarter:
    return "d1/4";
  case Subdivision::DottedEighth:
    return "d1/8";
  case Subdivision::DottedSixteenth:
    return "d1/16";
  case Subdivision::HalfTriplet:
    return "1/2t";
  case Subdivision::QuarterTriplet:
    return "1/4t";
  case Subdivision::EighthTriplet:
    return "1/8t";
  case Subdivision::SixteenthTriplet:
    return "1/16t";
  default:
    return "1/4"; // unreachable
  }
}

inline Subdivision parseSubdivision(const char* str) {
  if (std::strcmp(str, "1/1") == 0)
    return Subdivision::Whole;
  if (std::strcmp(str, "1/2") == 0)
    return Subdivision::Half;
  if (std::strcmp(str, "1/4") == 0)
    return Subdivision::Quarter;
  if (std::strcmp(str, "1/8") == 0)
    return Subdivision::Eighth;
  if (std::strcmp(str, "1/16") == 0)
    return Subdivision::Sixteenth;
  if (std::strcmp(str, "1/32") == 0)
    return Subdivision::ThirtySecond;
  if (std::strcmp(str, "1/64") == 0)
    return Subdivision::SixtyFourth;
  if (std::strcmp(str, "d1/2") == 0)
    return Subdivision::DottedHalf;
  if (std::strcmp(str, "d1/4") == 0)
    return Subdivision::DottedQuarter;
  if (std::strcmp(str, "d1/8") == 0)
    return Subdivision::DottedEighth;
  if (std::strcmp(str, "d1/16") == 0)
    return Subdivision::DottedSixteenth;
  if (std::strcmp(str, "1/2t") == 0)
    return Subdivision::HalfTriplet;
  if (std::strcmp(str, "1/4t") == 0)
    return Subdivision::QuarterTriplet;
  if (std::strcmp(str, "1/8t") == 0)
    return Subdivision::EighthTriplet;
  if (std::strcmp(str, "1/16t") == 0)
    return Subdivision::SixteenthTriplet;
  return Subdivision::Unknown; // unknown — caller should warn
}

inline float calcEffectiveRate(Subdivision sub, float bpm) {
  return 1.0f / dsp::tempo::subdivisionPeriodSeconds(sub, bpm);
}

} // namespace dsp::tempo
