#include "dsp/FX/Distortion.h"

#include "dsp/Buffers.h"
#include "dsp/Math.h"
#include "dsp/Waveshaper.h"

#include <cstddef>

namespace dsp::fx::distortion {

float calcDistortionInvNorm(float drive) {
  if (drive <= 0.0f)
    return 1.0f;
  return 1.0f / math::fastTanh(drive);
}

void processDistortion(DistortionFX& fx, buffers::StereoBuffer buf, size_t numSamples) {
  const float drive = fx.drive;
  const float invNorm = fx.invNorm;
  const float mix = fx.mix;

  if (fx.type == DistortionType::Hard) {
    const float threshold =
        1.0f / drive; // drive=1 → threshold=1.0 (no clip); drive=10 → threshold=0.1

    for (size_t i = 0; i < numSamples; i++) {
      float dryL = buf.left[i];
      float dryR = buf.right[i];

      buf.left[i] = mix * waveshaper::hardClip(dryL, threshold) + (1.0f - mix) * dryL;
      buf.right[i] = mix * waveshaper::hardClip(dryR, threshold) + (1.0f - mix) * dryR;
    }

    return;
  }

  // Soft — tanh, level-normalized
  for (size_t i = 0; i < numSamples; i++) {
    buf.left[i] = waveshaper::softClip(buf.left[i], drive, invNorm, mix);
    buf.right[i] = waveshaper::softClip(buf.right[i], drive, invNorm, mix);
  }
}

} // namespace dsp::fx::distortion
