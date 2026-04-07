#pragma once

#include "dsp/Buffers.h"
#include <cstddef>
#include <cstring>

namespace dsp::fx::distortion {

enum DistortionType : uint8_t {
  Soft = 0, // tanh soft clip — normalized
  Hard = 1, // hard clip at 1/drive threshold
  Unknown,
};
inline constexpr const char* UNKNOWN_TYPE = "unknown";

struct DistortionFX {
  float drive = 1.0f;
  float mix = 1.0f;
  bool enabled = false;

  DistortionType type = DistortionType::Soft;

  // precomputed: 1 / fastTanh(drive)
  // default = 1/tanh(1.0f) ≈ 1.3130
  float invNorm = 1.3130f;
};

float calcDistortionInvNorm(float drive);
void processDistortion(DistortionFX& fx, buffers::StereoBuffer buf, size_t numSamples);

// ===================
// Parsing Helpers
// ===================

inline DistortionType parseDistortionType(const char* name) {
  if (std::strcmp("soft", name) == 0)
    return DistortionType::Soft;

  if (std::strcmp("hard", name) == 0)
    return DistortionType::Hard;

  return DistortionType::Unknown;
}

inline const char* distortionTypeToString(DistortionType type) {
  switch (type) {
  case DistortionType::Soft:
    return "soft";

  case DistortionType::Hard:
    return "hard";

  default:
    return UNKNOWN_TYPE;
  }
}

} // namespace dsp::fx::distortion
