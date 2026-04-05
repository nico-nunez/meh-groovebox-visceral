#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

// This defines the "shape" of your 12 oscillator parameters
#define OSC_PARAMS(X, N)                                                                           \
  X(OSC##N##_BANK_ID, "osc" #N ".bank", OscBankID, 0.0f, 4.0f, 0.0f, OscBank)                      \
  X(OSC##N##_MIX_LEVEL, "osc" #N ".mixLevel", Float, 0.0f, 4.0f, 1.0f, None)                       \
  X(OSC##N##_DETUNE, "osc" #N ".detuneAmount", Float, -100.0f, 100.0f, 0.0f, None)                 \
  X(OSC##N##_OCTAVE, "osc" #N ".octaveOffset", Int8, -2.0f, 2.0f, 0.0f, None)                      \
  X(OSC##N##_SCAN_POS, "osc" #N ".scanPos", Float, 0.0f, 1.0f, 0.0f, None)                         \
  X(OSC##N##_PHASE_MODE, "osc" #N ".phaseMode", PhaseMode, 0.0f, 3.0f, 0.0f, None)                 \
  X(OSC##N##_RANDOM_RANGE, "osc" #N ".randomRange", Float, 0.0f, 1.0f, 1.0f, None)                 \
  X(OSC##N##_RESET_PHASE, "osc" #N ".resetPhase", Float, 0.0f, 1.0f, 0.0f, None)                   \
  X(OSC##N##_FM_DEPTH, "osc" #N ".fmDepth", Float, 0.0f, 2.0f, 1.0f, None)                         \
  X(OSC##N##_RATIO, "osc" #N ".ratio", Float, 0.5f, 16.0f, 1.0f, None)                             \
  X(OSC##N##_FIXED, "osc" #N ".fixed", Bool, 0.0f, 1.0f, 0.0f, None)                               \
  X(OSC##N##_FIXED_FREQ, "osc" #N ".fixedFreq", Float, 20.0f, 8000.0f, 440.0f, OscFreqFixed)       \
  X(OSC##N##_ENABLED, "osc" #N ".enabled", Bool, 0.0f, 1.0f, 1.0f, OscEnable)

#define LFO_PARAMS(X, N)                                                                           \
  X(LFO##N##_BANK_ID, "lfo" #N ".bank", OscBankID, 0.0f, 5.0, 0.0f, LFOBank)                       \
  X(LFO##N##_RATE, "lfo" #N ".rate", Float, 0.0f, 20.0f, 1.0f, LFORate)                            \
  X(LFO##N##_AMPLITUDE, "lfo" #N ".amplitude", Float, 0.0f, 1.0f, 1.0f, None)                      \
  X(LFO##N##_RETRIGGER, "lfo" #N ".retrigger", Bool, 0.0f, 1.0f, 0.0f, None)                       \
  X(LFO##N##_DELAY, "lfo" #N ".delayMs", Float, 0.0f, 5000.0f, 0.0f, LFOFadeIn)                    \
  X(LFO##N##_ATTACK, "lfo" #N ".attackMs", Float, 0.0f, 5000.0f, 0.0f, LFOFadeIn)                  \
  X(LFO##N##_SUBDIVISION, "lfo" #N ".subdivision", Subdivision, 0.0f, 14.0f, 2.0f, LFOTempoSync)   \
  X(LFO##N##_TEMPO_SYNC, "lfo" #N ".tempoSync", Bool, 0.0f, 1.0f, 0.0f, LFOTempoSync)

// X(enumId,           name,               type,       min,     max,      default, updateGroup)
#define PARAM_LIST                                                                                 \
  OSC_PARAMS(X, 1)                                                                                 \
  OSC_PARAMS(X, 2)                                                                                 \
  OSC_PARAMS(X, 3)                                                                                 \
  OSC_PARAMS(X, 4)                                                                                 \
                                                                                                   \
  LFO_PARAMS(X, 1)                                                                                 \
  LFO_PARAMS(X, 2)                                                                                 \
  LFO_PARAMS(X, 3)                                                                                 \
                                                                                                   \
  /* ==== Noise ==== */                                                                            \
  X(NOISE_TYPE, "noise.type", NoiseType, 0.0f, 1.0f, 0.0f, None)                                   \
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
  X(MASTER_GAIN, "master.gain", Float, 0.0f, 2.0f, 1.0f, None)                                     \
                                                                                                   \
  /* ==== Tempo ==== */                                                                            \
  X(BPM, "tempo.bpm", Float, 20.0f, 300.0f, 120.0f, BPMSync)                                       \
                                                                                                   \
  /* ======== Effects ======== */                                                                  \
  /* ==== Distortion ==== */                                                                       \
  X(FX_DISTORTION_DRIVE, "fx.distortion.drive", Float, 1.0f, 10.0f, 1.0f, DistortionDerived)       \
  X(FX_DISTORTION_MIX, "fx.distortion.mix", Float, 0.0f, 1.0f, 1.0f, None)                         \
  X(FX_DISTORTION_TYPE, "fx.distortion.type", DistortionType, 0.0f, 1.0f, 0.0f, None)              \
  X(FX_DISTORTION_ENABLED, "fx.distortion.enabled", Bool, 0.0f, 1.0f, 0.0f, None)                  \
                                                                                                   \
  /* ==== Chorus ==== */                                                                           \
  X(FX_CHORUS_RATE, "fx.chorus.rate", Float, 0.1f, 10.0f, 1.0f, ChorusDerived)                     \
  X(FX_CHORUS_DEPTH, "fx.chorus.depth", Float, 0.0f, 1.0f, 0.5f, ChorusDerived)                    \
  X(FX_CHORUS_MIX, "fx.chorus.mix", Float, 0.0f, 1.0f, 0.5f, ChorusDerived)                        \
  X(FX_CHORUS_FEEDBACK, "fx.chorus.feedback", Float, 0.0f, 0.95f, 0.0f, None)                      \
  X(FX_CHORUS_ENABLED, "fx.chorus.enabled", Bool, 0.0f, 1.0f, 0.0f, None)                          \
                                                                                                   \
  /* ==== Phaser ==== */                                                                           \
  X(FX_PHASER_STAGES, "fx.phaser.stages", Int8, 2.0f, 12.0f, 4.0f, None)                           \
  X(FX_PHASER_RATE, "fx.phaser.rate", Float, 0.1f, 10.0f, 0.5f, PhaserDerived)                     \
  X(FX_PHASER_DEPTH, "fx.phaser.depth", Float, 0.0f, 1.0f, 1.0f, PhaserDerived)                    \
  X(FX_PHASER_FEEDBACK, "fx.phaser.feedback", Float, 0.0f, 1.0f, 0.5f, None)                       \
  X(FX_PHASER_MIX, "fx.phaser.mix", Float, 0.0f, 1.0f, 0.5f, None)                                 \
  X(FX_PHASER_ENABLED, "fx.phaser.enabled", Bool, 0.0f, 1.0f, 0.0f, None)                          \
                                                                                                   \
  /* ==== Delay ==== */                                                                            \
  X(FX_DELAY_TIME, "fx.delay.time", Float, 0.01f, 4.0f, 0.5f, DelayDerived)                        \
  X(FX_DELAY_SUBDIVISION, "fx.delay.subdivision", Subdivision, 0.0f, 14.0f, 2.0f, DelayDerived)    \
  X(FX_DELAY_TEMPO_SYNC, "fx.delay.tempoSync", Bool, 0.0f, 1.0f, 1.0f, DelayDerived)               \
  X(FX_DELAY_FEEDBACK, "fx.delay.feedback", Float, 0.0f, 0.99f, 0.4f, None)                        \
  X(FX_DELAY_DAMPING, "fx.delay.damping", Float, 0.0f, 1.0f, 0.0f, DelayDamping)                   \
  X(FX_DELAY_HP_DAMPING, "fx.delay.hpDamping", Float, 0.0f, 1.0f, 0.0f, DelayDamping)              \
  X(FX_DELAY_PING_PONG, "fx.delay.pingPong", Bool, 0.0f, 1.0f, 0.0f, None)                         \
  X(FX_DELAY_MIX, "fx.delay.mix", Float, 0.0f, 1.0f, 0.5f, None)                                   \
  X(FX_DELAY_ENABLED, "fx.delay.enabled", Bool, 0.0f, 1.0f, 0.0f, None)                            \
                                                                                                   \
  /* ==== Reverb (Dattorro plate) ==== */                                                          \
  X(FX_REVERB_PRE_DELAY, "fx.reverb.preDelay", Float, 0.0f, 100.0f, 0.0f, ReverbDerived)           \
  X(FX_REVERB_DECAY, "fx.reverb.decay", Float, 0.1f, 20.0f, 4.0f, ReverbDerived)                   \
  X(FX_REVERB_DAMPING, "fx.reverb.damping", Float, 0.0f, 1.0f, 0.5f, ReverbDerived)                \
  X(FX_REVERB_LOW_DAMPING, "fx.reverb.lowDamping", Float, 0.0f, 1.0f, 0.5f, ReverbDerived)         \
  X(FX_REVERB_DIFFUSION, "fx.reverb.diffusion", Float, 0.0f, 1.0f, 0.75f, None)                    \
  X(FX_REVERB_BANDWIDTH, "fx.reverb.bandwidth", Float, 0.0f, 1.0f, 0.75f, None)                    \
  X(FX_REVERB_MOD_RATE, "fx.reverb.modRate", Float, 0.01f, 5.0f, 0.5f, ReverbDerived)              \
  X(FX_REVERB_MOD_DEPTH, "fx.reverb.modDepth", Float, 0.0f, 1.0f, 0.5f, ReverbDerived)             \
  X(FX_REVERB_MIX, "fx.reverb.mix", Float, 0.0f, 1.0f, 0.3f, None)                                 \
  X(FX_REVERB_ENABLED, "fx.reverb.enabled", Bool, 0.0f, 1.0f, 0.0f, None)

namespace synth::param {
// ================
// Params
// ================
enum class ParamType : uint8_t {
  Float,
  Int8,
  Bool,
  OscBankID,
  NoiseType,
  FilterMode,
  DistortionType,
  PhaseMode,
  Subdivision,
};

enum ParamID {
#define X(id, ...) id,
  PARAM_LIST
#undef X
      UNKNOWN,
  PARAM_COUNT,
};

// ===================
// Update Groups
// ===================
enum UpdateGroup {
  None = 0,

  OscBank,          // update bank
  OscFreqFixed,     // recalc fixedPhaseInc = fixedFreqHz / sampleRate
  OscEnable,        // recalc oscMixGain
                    //
  EnvTime,          // recalc envelope increments (needs sampleRate)
  EnvCurve,         // recalc curve lookup tables
                    //
  SVFCoeff,         // recalc SVF coefficients
  LadderCoeff,      // recalc Ladder coefficients
                    //
  SaturatorDerived, // recalc invDrive

  ChorusDerived,     // recalc  on rate, mix, or depth update
  DistortionDerived, // recalc on drive update
  PhaserDerived,     // recalc on rate or depth update
  DelayDerived,      // delay.time, delay.subdivision, or delay.tempoSync changed
  DelayDamping,      // recalc on damping or hpDamping update
  ReverbDerived,

  MonoEnable,    // kill poly voices or release mono
  PortaCoeff,    // recalc portamento coefficient
  UnisonDerived, // recalc detune offsets, pan, gain comp
  UnisonSpread,  // recalc pan positions only

  LFOBank,      // update lfo bank
  LFOTempoSync, // lfo.tempoSync or lfo.subdivision changed
  LFORate,      // lfo.rate changed — update effectiveRate when !tempoSync
  LFOFadeIn,    // lfo.delayMs or lfo.attackMs changed — recompute sample counts

  BPMSync, // BPM changed — recalc all synced components

  COUNT
};

struct UpdateGroupFlags {
  uint32_t flagBits{0};

  void mark(UpdateGroup grp) {
    if (grp == UpdateGroup::None || grp == UpdateGroup::COUNT)
      return;

    flagBits |= 1u << (grp - 1);
  }
  void markAll() {
    uint32_t mask;
    if (UpdateGroup::COUNT >= 32) {
      mask = 0xFFFFFFFF;
    } else {
      mask = (1u << UpdateGroup::COUNT) - 1u;
    }
    flagBits = mask;
  }
  bool isSet(const UpdateGroup grp) const { return (flagBits & (1u << (grp - 1))) != 0; }
  void clear(const UpdateGroup grp) { flagBits &= ~(1u << (grp - 1)); }
  void clearAll() { flagBits = 0; }
  bool any() const { return flagBits != 0; }
};

// =================
// Param Defs
// =================
struct ParamDef {
  const char* name; // canonical name: "osc1.mixLevel"
  ParamType type;
  float min;
  float max;
  float defaultVal;
  UpdateGroup updateGroup;
};

inline constexpr ParamDef PARAM_DEFS[] = {
#define X(id, name, type, min, max, def, group)                                                    \
  {name, ParamType::type, min, max, def, UpdateGroup::group},
    PARAM_LIST
#undef X
};

inline constexpr size_t PARAM_DEF_COUNT = sizeof(PARAM_DEFS) / sizeof(PARAM_DEFS[0]);
static_assert(PARAM_DEF_COUNT == PARAM_COUNT - 1,
              "PARAM_DEFS must have one entry per ParamID (excluding UNKNOWN)");

// ================
// Helpers
// ================
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

struct OscParamIDs {
  ParamID bankID, mixLevel, detune, octave, scanPos, phaseMode, randomRange, resetPhase, fmDepth,
      ratio, fixed, fixedFreq, enabled;
};

struct EnvParamIDs {
  ParamID attack, decay, sustain, release, attackCurve, decayCurve, releaseCurve;
};

struct LFOParamIDs {
  ParamID bankID, rate, amplitude, retrigger, delay, attack, subdivision, tempoSync;
};

// One bundle per instance, order matches the ParamID enum layout.
inline constexpr OscParamIDs OSC_PARAM_IDS[4] = {
    {OSC1_BANK_ID,
     OSC1_MIX_LEVEL,
     OSC1_DETUNE,
     OSC1_OCTAVE,
     OSC1_SCAN_POS,
     OSC1_PHASE_MODE,
     OSC1_RANDOM_RANGE,
     OSC1_RESET_PHASE,
     OSC1_FM_DEPTH,
     OSC1_RATIO,
     OSC1_FIXED,
     OSC1_FIXED_FREQ,
     OSC1_ENABLED},

    {OSC2_BANK_ID,
     OSC2_MIX_LEVEL,
     OSC2_DETUNE,
     OSC2_OCTAVE,
     OSC2_SCAN_POS,
     OSC2_PHASE_MODE,
     OSC2_RANDOM_RANGE,
     OSC2_RESET_PHASE,
     OSC2_FM_DEPTH,
     OSC2_RATIO,
     OSC2_FIXED,
     OSC2_FIXED_FREQ,
     OSC2_ENABLED},

    {OSC3_BANK_ID,
     OSC3_MIX_LEVEL,
     OSC3_DETUNE,
     OSC3_OCTAVE,
     OSC3_SCAN_POS,
     OSC3_PHASE_MODE,
     OSC3_RANDOM_RANGE,
     OSC3_RESET_PHASE,
     OSC3_FM_DEPTH,
     OSC3_RATIO,
     OSC3_FIXED,
     OSC3_FIXED_FREQ,
     OSC3_ENABLED},

    {OSC4_BANK_ID,
     OSC4_MIX_LEVEL,
     OSC4_DETUNE,
     OSC4_OCTAVE,
     OSC4_SCAN_POS,
     OSC4_PHASE_MODE,
     OSC4_RANDOM_RANGE,
     OSC4_RESET_PHASE,
     OSC4_FM_DEPTH,
     OSC4_RATIO,
     OSC4_FIXED,
     OSC4_FIXED_FREQ,
     OSC4_ENABLED},
};

inline constexpr EnvParamIDs ENV_PARAM_IDS[3] = {
    {AMP_ENV_ATTACK,
     AMP_ENV_DECAY,
     AMP_ENV_SUSTAIN,
     AMP_ENV_RELEASE,
     AMP_ENV_ATTACK_CURVE,
     AMP_ENV_DECAY_CURVE,
     AMP_ENV_RELEASE_CURVE},
    {FILTER_ENV_ATTACK,
     FILTER_ENV_DECAY,
     FILTER_ENV_SUSTAIN,
     FILTER_ENV_RELEASE,
     FILTER_ENV_ATTACK_CURVE,
     FILTER_ENV_DECAY_CURVE,
     FILTER_ENV_RELEASE_CURVE},
    {MOD_ENV_ATTACK,
     MOD_ENV_DECAY,
     MOD_ENV_SUSTAIN,
     MOD_ENV_RELEASE,
     MOD_ENV_ATTACK_CURVE,
     MOD_ENV_DECAY_CURVE,
     MOD_ENV_RELEASE_CURVE},
};

inline constexpr LFOParamIDs LFO_PARAM_IDS[3] = {
    {LFO1_BANK_ID,
     LFO1_RATE,
     LFO1_SUBDIVISION,
     LFO1_AMPLITUDE,
     LFO1_RETRIGGER,
     LFO1_TEMPO_SYNC,
     LFO1_DELAY,
     LFO1_ATTACK},

    {LFO2_BANK_ID,
     LFO2_RATE,
     LFO2_SUBDIVISION,
     LFO2_AMPLITUDE,
     LFO2_RETRIGGER,
     LFO2_TEMPO_SYNC,
     LFO2_DELAY,
     LFO2_ATTACK},

    {LFO3_BANK_ID,
     LFO3_RATE,
     LFO3_SUBDIVISION,
     LFO3_AMPLITUDE,
     LFO3_RETRIGGER,
     LFO3_TEMPO_SYNC,
     LFO3_DELAY,
     LFO3_ATTACK},
};

} // namespace synth::param
