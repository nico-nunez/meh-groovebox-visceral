#pragma once

#include "synth/Filters.h"

#include <cstdint>

namespace synth::param::ranges {

namespace osc {
inline constexpr float MIX_LEVEL_MIN = 0.0f;
inline constexpr float MIX_LEVEL_MAX = 4.0f;
inline constexpr float DETUNE_MIN = -100.0f; // cents
inline constexpr float DETUNE_MAX = 100.0f;
inline constexpr int8_t OCTAVE_MIN = -2;
inline constexpr int8_t OCTAVE_MAX = 2;
inline constexpr float FM_DEPTH_MIN = 0.0f; // route (not fmDepthMod)
inline constexpr float FM_DEPTH_MAX = 1.0f;
inline constexpr float RATIO_MIN = 0.5f;
inline constexpr float RATIO_MAX = 16.0f;
inline constexpr float SCAN_POS_MIN = 0.0f;
inline constexpr float SCAN_POS_MAX = 1.0f;

float clampMixLevel(float mixLevel);
float clampDetune(float detuneAmount);
float clampOctave(int8_t octaveOffset);
float clampFMDepth(float fmDepth);
float clampRatio(float ratio);
float clampScanPos(float scanPos);

namespace noise {
inline constexpr float MIX_LEVEL_MIN = 0.0f;
inline constexpr float MIX_LEVEL_MAX = 1.0f;

float clampMixLevel(float mixLevel);
} // namespace noise

} // namespace osc

namespace lfo {
inline constexpr float RATE_MIN = 0.0f;
inline constexpr float RATE_MAX = 20.0f; // can be audio-rate
inline constexpr float AMPLITUDE_MIN = 0.0f;
inline constexpr float AMPLITUDE_MAX = 1.0f;

float clampRate(float rate);
float clampAmplitude(float amplitude);
} // namespace lfo

namespace env {
inline constexpr float TIME_MIN = 0.0f;     // ms
inline constexpr float TIME_MAX = 10000.0f; // ms
inline constexpr float SUSTAIN_MIN = 0.0f;
inline constexpr float SUSTAIN_MAX = 1.0f;

inline constexpr float CURVE_MIN = -10.0f; // extreme concave
inline constexpr float CURVE_MAX = 10.0f;  // extreme convex

float clampTime(float timeAmount);
float clampCurve(float curveAmount);
float clampSustain(float sustainLevel);
} // namespace env

namespace filter {
inline constexpr int8_t FILTER_MODE_MIN = 0;
inline constexpr int8_t FILTER_MODE_MAX = static_cast<uint8_t>(filters::SVFMode::COUNT) - 1;
inline constexpr float CUTOFF_MIN = 20.0f;    // Hz
inline constexpr float CUTOFF_MAX = 20000.0f; // Hz
inline constexpr float RESONANCE_MIN = 0.0f;
inline constexpr float RESONANCE_MAX = 1.0f;
inline constexpr float DRIVE_MIN = 1.0f; // neutral / linear path
inline constexpr float DRIVE_MAX = 10.0f;

float clampCutoff(float cutoff);
float clampResonance(float resonance);
float clampDrive(float drive);
} // namespace filter

namespace saturator {
inline constexpr float DRIVE_MIN = 1.0f;
inline constexpr float DRIVE_MAX = 5.0f;
inline constexpr float MIX_MIN = 0.0f;
inline constexpr float MIX_MAX = 1.0f;

float clampDrive(float drive);
float clampMix(float mix);
} // namespace saturator

namespace mod {
// Cutoff modulation depth (octaves, bipolar)
inline constexpr float CUTOFF_MOD_MIN = -4.0f;
inline constexpr float CUTOFF_MOD_MAX = 4.0f;

// Pitch modulation depth (semitones, bipolar)
inline constexpr float PITCH_MOD_MIN = -24.0f;
inline constexpr float PITCH_MOD_MAX = 24.0f;

// Mix level modulation depth (linear, bipolar) — added to base mixLevel
inline constexpr float MIX_LEVEL_MOD_MIN = -1.0f;
inline constexpr float MIX_LEVEL_MOD_MAX = 1.0f;

// Resonance modulation depth (linear, bipolar) — added to base resonance
inline constexpr float RESONANCE_MOD_MIN = -1.0f;
inline constexpr float RESONANCE_MOD_MAX = 1.0f;

// Scan position modulation depth (bipolar offset to 0–1 base)
inline constexpr float SCAN_POS_MOD_MIN = -1.0f;
inline constexpr float SCAN_POS_MOD_MAX = 1.0f;

// FM depth modulation (bipolar, added to base fmDepth)
inline constexpr float FM_DEPTH_MOD_MIN = -1.0f;
inline constexpr float FM_DEPTH_MOD_MAX = 1.0f;

// LFO rate modulation (Hz offset, bipolar)
inline constexpr float LFO_RATE_MOD_MIN = -20.0f;
inline constexpr float LFO_RATE_MOD_MAX = 20.0f;

// LFO amplitude modulation (bipolar)
inline constexpr float LFO_AMPLITUDE_MOD_MIN = -1.0f;
inline constexpr float LFO_AMPLITUDE_MOD_MAX = 1.0f;

float clampCutoffMod(float cutoffMod);
float clampPitchMod(float pitchMod);
float clampMixLevelMod(float mixLevel);
float clampResonanceMod(float resonanceMod);
float clampScanPosMod(float scanPosMod);
float clampFMDepthMod(float fmDepthMod);
float clampLFORateMod(float lfoRateMod);
float clampLFOAmplitudeMod(float lfoAmplitudeMod);
} // namespace mod

namespace pitch {
inline constexpr float BEND_RANGE_MIN = 0.0f;
inline constexpr float BEND_RANGE_MAX = 48.0f; // ±4 octaves
inline constexpr float BEND_RANGE_DEFAULT = 2.0f;

float clampBendRange(float bendRange);
} // namespace pitch

namespace porta {
inline constexpr float TIME_MIN = 0.0f;
inline constexpr float TIME_MAX = 5000.0f; // ms

float clampTime(float time);
} // namespace porta

namespace unison {
inline constexpr int8_t VOICES_MIN = 1;
inline constexpr int8_t VOICES_MAX = 8;
inline constexpr float DETUNE_MIN = 0.0f;
inline constexpr float DETUNE_MAX = 100.0f; // cents
inline constexpr float SPREAD_MIN = 0.0f;
inline constexpr float SPREAD_MAX = 1.0f;

int8_t clampVoices(int8_t voices);
float clampDetune(float detune);
float clampSpread(float spread);
} // namespace unison

namespace global {
inline constexpr float MASTER_GAIN_MIN = 0.0f;
inline constexpr float MASTER_GAIN_MAX = 2.0f; // 2.0 ≈ +6 dB

float clampMasterGain(float masterGain);
} // namespace global

} // namespace synth::param::ranges
