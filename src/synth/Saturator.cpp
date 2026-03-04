#include "Saturator.h"

#include "dsp/Math.h"
#include "dsp/Waveshaper.h"

namespace synth::saturator {

namespace {
float denormalizeDrive(float drive) {
  return 1 + drive * 4.0f;
}

} // namespace

// Returns whatever drive is (Normalized or Denormalized)
float calcInvDrive(float drive) {
  if (drive <= 0)
    return 1.0f;

  return 1.0f / dsp::math::fastTanh(drive);
}
void updateDrive(Saturator& sat, float normalizedDrive) {
  sat.drive = denormalizeDrive(normalizedDrive);
  sat.invDrive = calcInvDrive(sat.drive);
}

float processSaturator(const Saturator& sat, float sample) {
  if (!sat.enabled)
    return sample;

  return dsp::waveshaper::softClip(sample, sat.drive, sat.invDrive, sat.mix);
}

} // namespace synth::saturator
