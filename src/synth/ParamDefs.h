#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace synth::param {
// X(enumId,           name,               type,       min,     max,      default, updateGroup)
#define PARAM_LIST                                                                                 \
  /* ==== Oscillators ==== */                                                                      \
  X(OSC1_MIX_LEVEL, "osc1.mixLevel", Float, 0.0f, 4.0f, 1.0f, None)                                \
  X(OSC1_DETUNE, "osc1.detuneAmount", Float, -100.0f, 100.0f, 0.0f, None)                          \
  X(OSC1_OCTAVE, "osc1.octaveOffset", Int8, -2.0f, 2.0f, 0.0f, None)                               \
  X(OSC1_SCAN_POS, "osc1.scanPos", Float, 0.0f, 1.0f, 0.0f, None)                                  \
  X(OSC1_FM_DEPTH, "osc1.fmDepth", Float, 0.0f, 5.0f, 0.0f, None)                                  \
  X(OSC1_FM_RATIO, "osc1.fmRatio", Float, 0.5f, 16.0f, 1.0f, None)                                 \
  X(OSC1_ENABLED, "osc1.enabled", Bool, 0.0f, 1.0f, 1.0f, OscEnable)                               \
                                                                                                   \
  X(OSC2_MIX_LEVEL, "osc2.mixLevel", Float, 0.0f, 4.0f, 1.0f, None)                                \
  X(OSC2_DETUNE, "osc2.detuneAmount", Float, -100.0f, 100.0f, 0.0f, None)                          \
  X(OSC2_OCTAVE, "osc2.octaveOffset", Int8, -2.0f, 2.0f, 0.0f, None)                               \
  X(OSC2_SCAN_POS, "osc2.scanPos", Float, 0.0f, 1.0f, 0.0f, None)                                  \
  X(OSC2_FM_DEPTH, "osc2.fmDepth", Float, 0.0f, 5.0f, 0.0f, None)                                  \
  X(OSC2_FM_RATIO, "osc2.fmRatio", Float, 0.5f, 16.0f, 1.0f, None)                                 \
  X(OSC2_ENABLED, "osc2.enabled", Bool, 0.0f, 1.0f, 0.0f, OscEnable)                               \
                                                                                                   \
  X(OSC3_MIX_LEVEL, "osc3.mixLevel", Float, 0.0f, 4.0f, 1.0f, None)                                \
  X(OSC3_DETUNE, "osc3.detuneAmount", Float, -100.0f, 100.0f, 0.0f, None)                          \
  X(OSC3_OCTAVE, "osc3.octaveOffset", Int8, -2.0f, 2.0f, 0.0f, None)                               \
  X(OSC3_SCAN_POS, "osc3.scanPos", Float, 0.0f, 1.0f, 0.0f, None)                                  \
  X(OSC3_FM_DEPTH, "osc3.fmDepth", Float, 0.0f, 5.0f, 0.0f, None)                                  \
  X(OSC3_FM_RATIO, "osc3.fmRatio", Float, 0.5f, 16.0f, 1.0f, None)                                 \
  X(OSC3_ENABLED, "osc3.enabled", Bool, 0.0f, 1.0f, 0.0f, OscEnable)                               \
                                                                                                   \
  X(OSC4_MIX_LEVEL, "osc4.mixLevel", Float, 0.0f, 4.0f, 1.0f, None)                                \
  X(OSC4_DETUNE, "osc4.detuneAmount", Float, -100.0f, 100.0f, 0.0f, None)                          \
  X(OSC4_OCTAVE, "osc4.octaveOffset", Int8, -2.0f, 2.0f, 0.0f, None)                               \
  X(OSC4_SCAN_POS, "osc4.scanPos", Float, 0.0f, 1.0f, 0.0f, None)                                  \
  X(OSC4_FM_DEPTH, "osc4.fmDepth", Float, 0.0f, 5.0f, 0.0f, None)                                  \
  X(OSC4_FM_RATIO, "osc4.fmRatio", Float, 0.5f, 16.0f, 1.0f, None)                                 \
  X(OSC4_ENABLED, "osc4.enabled", Bool, 0.0f, 1.0f, 0.0f, OscEnable)                               \
                                                                                                   \
  /* ==== Noise ==== */                                                                            \
  X(NOISE_MIX_LEVEL, "noise.mixLevel", Float, 0.0f, 1.0f, 0.0f, None)                              \
  X(NOISE_ENABLED, "noise.enabled", Bool, 0.0f, 1.0f, 0.0f, OscEnable)                             \
                                                                                                   \
  /* ==== Envelopes ==== */                                                                        \
  X(AMP_ENV_ATTACK, "ampEnv.attackMs", Float, 0.0f, 10000.0f, 10.0f, EnvTime)                      \
  X(AMP_ENV_DECAY, "ampEnv.decayMs", Float, 0.0f, 10000.0f, 100.0f, EnvTime)                       \
  X(AMP_ENV_SUSTAIN, "ampEnv.sustainLevel", Float, 0.0f, 1.0f, 0.7f, None)                         \
  X(AMP_ENV_RELEASE, "ampEnv.releaseMs", Float, 0.0f, 10000.0f, 200.0f, EnvTime)                   \
  X(AMP_ENV_ATTACK_CURVE, "ampEnv.attackCurve", Float, -10.0f, 10.0f, -5.0f, EnvCurve)             \
  X(AMP_ENV_DECAY_CURVE, "ampEnv.decayCurve", Float, -10.0f, 10.0f, -5.0f, EnvCurve)               \
  X(AMP_ENV_RELEASE_CURVE, "ampEnv.releaseCurve", Float, -10.0f, 10.0f, -5.0f, EnvCurve)           \
                                                                                                   \
  X(FILTER_ENV_ATTACK, "filterEnv.attackMs", Float, 0.0f, 10000.0f, 10.0f, EnvTime)                \
  X(FILTER_ENV_DECAY, "filterEnv.decayMs", Float, 0.0f, 10000.0f, 100.0f, EnvTime)                 \
  X(FILTER_ENV_SUSTAIN, "filterEnv.sustainLevel", Float, 0.0f, 1.0f, 0.7f, None)                   \
  X(FILTER_ENV_RELEASE, "filterEnv.releaseMs", Float, 0.0f, 10000.0f, 200.0f, EnvTime)             \
  X(FILTER_ENV_ATTACK_CURVE, "filterEnv.attackCurve", Float, -10.0f, 10.0f, -5.0f, EnvCurve)       \
  X(FILTER_ENV_DECAY_CURVE, "filterEnv.decayCurve", Float, -10.0f, 10.0f, -5.0f, EnvCurve)         \
  X(FILTER_ENV_RELEASE_CURVE, "filterEnv.releaseCurve", Float, -10.0f, 10.0f, -5.0f, EnvCurve)     \
                                                                                                   \
  X(MOD_ENV_ATTACK, "modEnv.attackMs", Float, 0.0f, 10000.0f, 10.0f, EnvTime)                      \
  X(MOD_ENV_DECAY, "modEnv.decayMs", Float, 0.0f, 10000.0f, 100.0f, EnvTime)                       \
  X(MOD_ENV_SUSTAIN, "modEnv.sustainLevel", Float, 0.0f, 1.0f, 0.7f, None)                         \
  X(MOD_ENV_RELEASE, "modEnv.releaseMs", Float, 0.0f, 10000.0f, 200.0f, EnvTime)                   \
  X(MOD_ENV_ATTACK_CURVE, "modEnv.attackCurve", Float, -10.0f, 10.0f, -5.0f, EnvCurve)             \
  X(MOD_ENV_DECAY_CURVE, "modEnv.decayCurve", Float, -10.0f, 10.0f, -5.0f, EnvCurve)               \
  X(MOD_ENV_RELEASE_CURVE, "modEnv.releaseCurve", Float, -10.0f, 10.0f, -5.0f, EnvCurve)           \
                                                                                                   \
  /* ==== Filters ==== */                                                                          \
  X(SVF_MODE, "svf.mode", FilterMode, 0.0f, 3.0f, 0.0f, None)                                      \
  X(SVF_CUTOFF, "svf.cutoff", Float, 20.0f, 20000.0f, 1000.0f, SVFCoeff)                           \
  X(SVF_RESONANCE, "svf.resonance", Float, 0.0f, 1.0f, 0.5f, SVFCoeff)                             \
  X(SVF_ENABLED, "svf.enabled", Bool, 0.0f, 1.0f, 0.0f, None)                                      \
                                                                                                   \
  X(LADDER_CUTOFF, "ladder.cutoff", Float, 20.0f, 20000.0f, 1000.0f, LadderCoeff)                  \
  X(LADDER_RESONANCE, "ladder.resonance", Float, 0.0f, 1.0f, 0.3f, LadderCoeff)                    \
  X(LADDER_DRIVE, "ladder.drive", Float, 1.0f, 10.0f, 1.0f, None)                                  \
  X(LADDER_ENABLED, "ladder.enabled", Bool, 0.0f, 1.0f, 0.0f, None)                                \
                                                                                                   \
  /* ==== Saturator ==== */                                                                        \
  X(SATURATOR_DRIVE, "saturator.drive", Float, 1.0f, 5.0f, 1.0f, SaturatorDerived)                 \
  X(SATURATOR_MIX, "saturator.mix", Float, 0.0f, 1.0f, 1.0f, None)                                 \
  X(SATURATOR_ENABLED, "saturator.enabled", Bool, 0.0f, 1.0f, 0.0f, None)                          \
                                                                                                   \
  /* ==== LFOs ==== */                                                                             \
  X(LFO1_RATE, "lfo1.rate", Float, 0.0f, 20.0f, 1.0f, None)                                        \
  X(LFO1_AMPLITUDE, "lfo1.amplitude", Float, 0.0f, 1.0f, 1.0f, None)                               \
  X(LFO1_RETRIGGER, "lfo1.retrigger", Bool, 0.0f, 1.0f, 0.0f, None)                                \
                                                                                                   \
  X(LFO2_RATE, "lfo2.rate", Float, 0.0f, 20.0f, 1.0f, None)                                        \
  X(LFO2_AMPLITUDE, "lfo2.amplitude", Float, 0.0f, 1.0f, 1.0f, None)                               \
  X(LFO2_RETRIGGER, "lfo2.retrigger", Bool, 0.0f, 1.0f, 0.0f, None)                                \
                                                                                                   \
  X(LFO3_RATE, "lfo3.rate", Float, 0.0f, 20.0f, 1.0f, None)                                        \
  X(LFO3_AMPLITUDE, "lfo3.amplitude", Float, 0.0f, 1.0f, 1.0f, None)                               \
  X(LFO3_RETRIGGER, "lfo3.retrigger", Bool, 0.0f, 1.0f, 0.0f, None)                                \
                                                                                                   \
  /* ==== Global / Voice modes ==== */                                                             \
  X(PITCH_BEND_RANGE, "pitchBend.range", Float, 0.0f, 48.0f, 2.0f, None)                           \
                                                                                                   \
  X(MONO_ENABLED, "mono.enabled", Bool, 0.0f, 1.0f, 0.0f, MonoEnable)                              \
  X(MONO_LEGATO, "mono.legato", Bool, 0.0f, 1.0f, 1.0f, None)                                      \
                                                                                                   \
  X(PORTA_TIME, "porta.time", Float, 0.0f, 5000.0f, 50.0f, PortaCoeff)                             \
  X(PORTA_LEGATO, "porta.legato", Bool, 0.0f, 1.0f, 1.0f, None)                                    \
  X(PORTA_ENABLED, "porta.enabled", Bool, 0.0f, 1.0f, 0.0f, None)                                  \
                                                                                                   \
  X(UNISON_VOICES, "unison.voices", Int8, 1.0f, 8.0f, 4.0f, UnisonDerived)                         \
  X(UNISON_DETUNE, "unison.detune", Float, 0.0f, 100.0f, 20.0f, UnisonDerived)                     \
  X(UNISON_SPREAD, "unison.spread", Float, 0.0f, 1.0f, 0.5f, UnisonSpread)                         \
  X(UNISON_ENABLED, "unison.enabled", Bool, 0.0f, 1.0f, 0.0f, UnisonDerived)                       \
                                                                                                   \
  X(MASTER_GAIN, "master.gain", Float, 0.0f, 2.0f, 1.0f, None)

// What kind of side effect does changing this param trigger?
enum class UpdateGroup : uint8_t {
  None = 0,
  OscEnable,        // recalc oscMixGain
  EnvTime,          // recalc envelope increments (needs sampleRate)
  EnvCurve,         // recalc curve lookup tables
  SVFCoeff,         // recalc SVF coefficients
  LadderCoeff,      // recalc Ladder coefficients
  SaturatorDerived, // recalc invDrive
  MonoEnable,       // kill poly voices or release mono
  PortaCoeff,       // recalc portamento coefficient
  UnisonDerived,    // recalc detune offsets, pan, gain comp
  UnisonSpread,     // recalc pan positions only
};

enum class ParamType : uint8_t {
  Float,
  Int8,
  Bool,
  FilterMode,
};

struct ParamDef {
  const char* name; // canonical name: "osc1.mixLevel"
  ParamType type;
  float min;
  float max;
  float defaultVal;
  UpdateGroup updateGroup;
};

enum ParamID {
#define X(id, ...) id,
  PARAM_LIST
#undef X
      UNKNOWN,
  PARAM_COUNT,
};

// One row per ParamID. Order MUST match the enum.
// This is enforced by static_assert at the bottom.
inline constexpr ParamDef PARAM_DEFS[] = {
#define X(id, name, type, min, max, def, group)                                                    \
  {name, ParamType::type, min, max, def, UpdateGroup::group},
    PARAM_LIST
#undef X
};

inline constexpr size_t PARAM_DEF_COUNT = sizeof(PARAM_DEFS) / sizeof(PARAM_DEFS[0]);

static_assert(PARAM_DEF_COUNT == PARAM_COUNT - 1,
              "PARAM_DEFS must have one entry per ParamID (excluding UNKNOWN)");

// ==== Lookup helpers ====
inline const ParamDef& getParamDef(ParamID id) {
  return PARAM_DEFS[static_cast<int>(id)];
}

// Clamp a denormalized value to a param's declared range
inline float clampParam(ParamID id, float value) {
  const auto& def = PARAM_DEFS[static_cast<int>(id)];
  if (value < def.min)
    return def.min;
  if (value > def.max)
    return def.max;
  return value;
}

} // namespace synth::param
