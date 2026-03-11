#include "ParamRanges.h"
#include <algorithm>
#include <cstdint>

namespace synth::param::ranges {

// Oscillator Param Helpers
namespace osc {

float clampScanPos(float scanPos) {
  return std::clamp(scanPos, SCAN_POS_MIN, SCAN_POS_MAX);
}
float clampMixLevel(float mixLevel) {
  return std::clamp(mixLevel, MIX_LEVEL_MIN, MIX_LEVEL_MAX);
}
float clampDetune(float detuneAmount) {
  return std::clamp(detuneAmount, DETUNE_MIN, DETUNE_MAX);
}
float clampOctave(int8_t octaveOffset) {
  return std::clamp(octaveOffset, OCTAVE_MIN, OCTAVE_MAX);
}

float clampFMDepth(float fmDepth) {
  return std::clamp(fmDepth, FM_DEPTH_MIN, FM_DEPTH_MAX);
}
float clampFMRatio(float fmRatio) {
  return std::clamp(fmRatio, FM_RATIO_MIN, FM_RATIO_MAX);
}

namespace noise {
float clampMixLevel(float mixLevel) {
  return std::clamp(mixLevel, MIX_LEVEL_MIN, MIX_LEVEL_MAX);
}
} // namespace noise

} // namespace osc

namespace lfo {
float clampRate(float rate) {
  return std::clamp(rate, RATE_MIN, RATE_MAX);
}
float clampAmplitude(float amplitude) {
  return std::clamp(amplitude, AMPLITUDE_MIN, AMPLITUDE_MAX);
}
} // namespace lfo

// Envelope Param Helpers
namespace env {
float clampTime(float timeAmount) {
  return std::clamp(timeAmount, TIME_MIN, TIME_MAX);
}
float clampCurve(float curveAmount) {
  return std::clamp(curveAmount, CURVE_MIN, CURVE_MAX);
}
float clampSustain(float sustainLevel) {
  return std::clamp(sustainLevel, SUSTAIN_MIN, SUSTAIN_MAX);
}
} // namespace env

// Filter Param Helpers
namespace filter {
float clampCutoff(float cutoff) {
  return std::clamp(cutoff, CUTOFF_MIN, CUTOFF_MAX);
}
float clampResonance(float resonance) {
  return std::clamp(resonance, RESONANCE_MIN, RESONANCE_MAX);
}
float clampDrive(float drive) {
  return std::clamp(drive, DRIVE_MIN, DRIVE_MAX);
}
} // namespace filter

// Saturator Param Helpers
namespace saturator {
float clampDrive(float drive) {
  return std::clamp(drive, DRIVE_MIN, DRIVE_MAX);
}
float clampMix(float mix) {
  return std::clamp(mix, MIX_MIN, MIX_MAX);
}
} // namespace saturator

// Mod Matrix Param Helpers
namespace mod {
float clampCutoffMod(float cutoffMod) {
  return std::clamp(cutoffMod, CUTOFF_MOD_MIN, CUTOFF_MOD_MAX);
}
float clampPitchMod(float pitchMod) {
  return std::clamp(pitchMod, PITCH_MOD_MIN, PITCH_MOD_MAX);
}
float clampMixLevelMod(float mixLevel) {
  return std::clamp(mixLevel, MIX_LEVEL_MOD_MIN, MIX_LEVEL_MOD_MAX);
}
float clampResonanceMod(float resonanceMod) {
  return std::clamp(resonanceMod, RESONANCE_MOD_MIN, RESONANCE_MOD_MAX);
}
float clampScanPosMod(float scanPosMod) {
  return std::clamp(scanPosMod, SCAN_POS_MOD_MIN, SCAN_POS_MOD_MAX);
}
float clampFMDepthMod(float fmDepthMod) {
  return std::clamp(fmDepthMod, FM_DEPTH_MOD_MIN, FM_DEPTH_MOD_MAX);
}
float clampLFORateMod(float lfoRateMod) {
  return std::clamp(lfoRateMod, LFO_RATE_MOD_MIN, LFO_RATE_MOD_MAX);
}
float clampLFOAmplitudeMod(float lfoAmplitudeMod) {
  return std::clamp(lfoAmplitudeMod, LFO_AMPLITUDE_MOD_MIN, LFO_AMPLITUDE_MOD_MAX);
}
} // namespace mod

namespace pitch {
float clampBendRange(float bendRange) {
  return std::clamp(bendRange, BEND_RANGE_MIN, BEND_RANGE_MAX);
}
} // namespace pitch

namespace porta {
float clampTime(float time) {
  return std::clamp(time, TIME_MIN, TIME_MAX);
}
} // namespace porta

namespace unison {
int8_t clampVoices(int8_t voices) {
  return std::clamp(voices, VOICES_MIN, VOICES_MAX);
}
float clampDetune(float detune) {
  return std::clamp(detune, DETUNE_MIN, DETUNE_MAX);
}
float clampSpread(float spread) {
  return std::clamp(spread, SPREAD_MIN, SPREAD_MAX);
}
} // namespace unison
// Global Param Helpers
namespace global {
float clampMasterGain(float masterGain) {
  return std::clamp(masterGain, MASTER_GAIN_MIN, MASTER_GAIN_MAX);
}
} // namespace global

} // namespace synth::param::ranges
