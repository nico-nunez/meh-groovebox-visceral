#pragma once

namespace synth::saturator {
struct Saturator {
  float drive = 1.0f;
  float invDrive = 1.0f;
  float mix = 1.0f;
  bool enabled = false;
};

float calcInvDrive(float drive);

void updateDrive(Saturator& sat, float normalizedDrive);

float processSaturator(const Saturator& sat, float input);

} // namespace synth::saturator
