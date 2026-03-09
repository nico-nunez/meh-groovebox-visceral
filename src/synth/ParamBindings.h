#pragma once

#include "synth/Filters.h"
#include "synth/VoicePool.h"

#include <cstddef>
#include <cstdint>

namespace synth::param::bindings {
using filters::SVFMode;

enum ParamID {
  // Oscillator 1
  OSC1_MIX_LEVEL,
  OSC1_DETUNE_AMOUNT,
  OSC1_OCTAVE_OFFSET,
  OSC1_SCAN_POS,
  OSC1_FM_DEPTH,
  OSC1_FM_RATIO,
  OSC1_ENABLED,

  // Oscillator 2
  OSC2_MIX_LEVEL,
  OSC2_DETUNE_AMOUNT,
  OSC2_OCTAVE_OFFSET,
  OSC2_SCAN_POS,
  OSC2_FM_DEPTH,
  OSC2_FM_RATIO,
  OSC2_ENABLED,

  // Oscillator 3
  OSC3_MIX_LEVEL,
  OSC3_DETUNE_AMOUNT,
  OSC3_OCTAVE_OFFSET,
  OSC3_SCAN_POS,
  OSC3_FM_DEPTH,
  OSC3_FM_RATIO,
  OSC3_ENABLED,

  // Oscillator 4
  OSC4_MIX_LEVEL,
  OSC4_DETUNE_AMOUNT,
  OSC4_OCTAVE_OFFSET,
  OSC4_SCAN_POS,
  OSC4_FM_DEPTH,
  OSC4_FM_RATIO,
  OSC4_ENABLED,

  // Noise Oscillator
  NOISE_MIX_LEVEL,
  NOISE_ENABLED,

  // Amp Envelope
  AMP_ENV_ATTACK,
  AMP_ENV_DECAY,
  AMP_ENV_SUSTAIN_LEVEL,
  AMP_ENV_RELEASE,
  AMP_ENV_ATTACK_CURVE,
  AMP_ENV_DECAY_CURVE,
  AMP_ENV_RELEASE_CURVE,

  // Filter Envelope
  FILTER_ENV_ATTACK,
  FILTER_ENV_DECAY,
  FILTER_ENV_SUSTAIN_LEVEL,
  FILTER_ENV_RELEASE,
  FILTER_ENV_ATTACK_CURVE,
  FILTER_ENV_DECAY_CURVE,
  FILTER_ENV_RELEASE_CURVE,

  // Modulation Envelope
  MOD_ENV_ATTACK,
  MOD_ENV_DECAY,
  MOD_ENV_SUSTAIN_LEVEL,
  MOD_ENV_RELEASE,
  MOD_ENV_ATTACK_CURVE,
  MOD_ENV_DECAY_CURVE,
  MOD_ENV_RELEASE_CURVE,

  // SVF Filter
  SVF_MODE,
  SVF_CUTOFF,
  SVF_RESONANCE,
  SVF_ENABLED,

  // Ladder Filter
  LADDER_CUTOFF,
  LADDER_RESONANCE,
  LADDER_DRIVE,
  LADDER_ENABLED,

  // Saturator
  SATURATOR_DRIVE,
  SATURATOR_MIX,
  SATURATOR_ENABLED,

  // LFOs (global)
  LFO1_RATE,
  LFO1_AMPLITUDE,
  LFO1_RETRIGGER,

  LFO2_RATE,
  LFO2_AMPLITUDE,
  LFO2_RETRIGGER,

  LFO3_RATE,
  LFO3_AMPLITUDE,
  LFO3_RETRIGGER,

  PITCH_BEND_RANGE,

  MASTER_GAIN,

  MONO_ENABLED,
  MONO_LEGATO,

  PORTA_TIME,
  PORTA_LEGATO,
  PORTA_ENABLED,

  UNKOWN,
  PARAM_COUNT,
};

enum ParamValueFormat {
  NORMALIZED,
  DENORMALIZED,
};

enum ParamValueType { FLOAT, INT8, BOOL, FILTER_MODE };

struct ParamBinding {
  union {
    float* floatPtr;
    int8_t* int8Ptr;
    bool* boolPtr;
    SVFMode* svfModePtr;
  };
  ParamValueType type;
  float min, max;
};

// ==== Param Parsing ====
struct ParamMapping {
  ParamID id;
  const char* name;
  ParamValueType type;
};

// Used to find input param names
constexpr ParamMapping PARAM_NAMES[] = {
    {OSC1_MIX_LEVEL, "osc1.mixLevel", ParamValueType::FLOAT},
    {OSC1_DETUNE_AMOUNT, "osc1.detune", ParamValueType::FLOAT},
    {OSC1_OCTAVE_OFFSET, "osc1.octave", ParamValueType::INT8},
    {OSC1_SCAN_POS, "osc1.scanPos", ParamValueType::FLOAT},
    {OSC1_FM_DEPTH, "osc1.fmDepth", ParamValueType::FLOAT},
    {OSC1_FM_RATIO, "osc1.fmRatio", ParamValueType::FLOAT},
    {OSC1_ENABLED, "osc1.enabled", ParamValueType::BOOL},

    {OSC2_MIX_LEVEL, "osc2.mixLevel", ParamValueType::FLOAT},
    {OSC2_DETUNE_AMOUNT, "osc2.detune", ParamValueType::FLOAT},
    {OSC2_OCTAVE_OFFSET, "osc2.octave", ParamValueType::INT8},
    {OSC2_SCAN_POS, "osc2.scanPos", ParamValueType::FLOAT},
    {OSC2_FM_DEPTH, "osc2.fmDepth", ParamValueType::FLOAT},
    {OSC2_FM_RATIO, "osc2.fmRatio", ParamValueType::FLOAT},
    {OSC2_ENABLED, "osc2.enabled", ParamValueType::BOOL},

    {OSC3_MIX_LEVEL, "osc3.mixLevel", ParamValueType::FLOAT},
    {OSC3_DETUNE_AMOUNT, "osc3.detune", ParamValueType::FLOAT},
    {OSC3_OCTAVE_OFFSET, "osc3.octave", ParamValueType::INT8},
    {OSC3_SCAN_POS, "osc3.scanPos", ParamValueType::FLOAT},
    {OSC3_FM_DEPTH, "osc3.fmDepth", ParamValueType::FLOAT},
    {OSC3_FM_RATIO, "osc3.fmRatio", ParamValueType::FLOAT},
    {OSC3_ENABLED, "osc3.enabled", ParamValueType::BOOL},

    {OSC4_MIX_LEVEL, "osc4.mixLevel", ParamValueType::FLOAT},
    {OSC4_DETUNE_AMOUNT, "osc4.detune", ParamValueType::FLOAT},
    {OSC4_OCTAVE_OFFSET, "osc4.octave", ParamValueType::INT8},
    {OSC4_SCAN_POS, "osc4.scanPos", ParamValueType::FLOAT},
    {OSC4_FM_DEPTH, "osc4.fmDepth", ParamValueType::FLOAT},
    {OSC4_FM_RATIO, "osc4.fmRatio", ParamValueType::FLOAT},
    {OSC4_ENABLED, "osc4.enabled", ParamValueType::BOOL},

    {NOISE_MIX_LEVEL, "noise.mixLevel", ParamValueType::FLOAT},
    {NOISE_ENABLED, "noise.enabled", ParamValueType::BOOL},

    {AMP_ENV_ATTACK, "ampEnv.attack", ParamValueType::FLOAT},
    {AMP_ENV_DECAY, "ampEnv.decay", ParamValueType::FLOAT},
    {AMP_ENV_SUSTAIN_LEVEL, "ampEnv.sustain", ParamValueType::FLOAT},
    {AMP_ENV_RELEASE, "ampEnv.release", ParamValueType::FLOAT},
    {AMP_ENV_ATTACK_CURVE, "ampEnv.attackCurve", ParamValueType::FLOAT},
    {AMP_ENV_DECAY_CURVE, "ampEnv.decayCurve", ParamValueType::FLOAT},
    {AMP_ENV_RELEASE_CURVE, "ampEnv.releaseCurve", ParamValueType::FLOAT},

    {FILTER_ENV_ATTACK, "filterEnv.attack", ParamValueType::FLOAT},
    {FILTER_ENV_DECAY, "filterEnv.decay", ParamValueType::FLOAT},
    {FILTER_ENV_SUSTAIN_LEVEL, "filterEnv.sustain", ParamValueType::FLOAT},
    {FILTER_ENV_RELEASE, "filterEnv.release", ParamValueType::FLOAT},
    {FILTER_ENV_ATTACK_CURVE, "filterEnv.attackCurve", ParamValueType::FLOAT},
    {FILTER_ENV_DECAY_CURVE, "filterEnv.decayCurve", ParamValueType::FLOAT},
    {FILTER_ENV_RELEASE_CURVE, "filterEnv.releaseCurve", ParamValueType::FLOAT},

    {MOD_ENV_ATTACK, "modEnv.attack", ParamValueType::FLOAT},
    {MOD_ENV_DECAY, "modEnv.decay", ParamValueType::FLOAT},
    {MOD_ENV_SUSTAIN_LEVEL, "modEnv.sustain", ParamValueType::FLOAT},
    {MOD_ENV_RELEASE, "modEnv.release", ParamValueType::FLOAT},
    {MOD_ENV_ATTACK_CURVE, "modEnv.attackCurve", ParamValueType::FLOAT},
    {MOD_ENV_DECAY_CURVE, "modEnv.decayCurve", ParamValueType::FLOAT},
    {MOD_ENV_RELEASE_CURVE, "modEnv.releaseCurve", ParamValueType::FLOAT},

    {SVF_MODE, "svf.mode", ParamValueType::FILTER_MODE},
    {SVF_CUTOFF, "svf.cutoff", ParamValueType::FLOAT},
    {SVF_RESONANCE, "svf.resonance", ParamValueType::FLOAT},
    {SVF_ENABLED, "svf.enabled", ParamValueType::BOOL},

    {LADDER_CUTOFF, "ladder.cutoff", ParamValueType::FLOAT},
    {LADDER_RESONANCE, "ladder.resonance", ParamValueType::FLOAT},
    {LADDER_DRIVE, "ladder.drive", ParamValueType::FLOAT},
    {LADDER_ENABLED, "ladder.enabled", ParamValueType::BOOL},

    {SATURATOR_DRIVE, "saturator.drive", ParamValueType::FLOAT},
    {SATURATOR_MIX, "saturator.mix", ParamValueType::FLOAT},
    {SATURATOR_ENABLED, "saturator.enabled", ParamValueType::BOOL},

    {LFO1_RATE, "lfo1.rate", ParamValueType::FLOAT},
    {LFO1_AMPLITUDE, "lfo1.amplitude", ParamValueType::FLOAT},
    {LFO1_RETRIGGER, "lfo1.retrigger", ParamValueType::BOOL},

    {LFO2_RATE, "lfo2.rate", ParamValueType::FLOAT},
    {LFO2_AMPLITUDE, "lfo2.amplitude", ParamValueType::FLOAT},
    {LFO2_RETRIGGER, "lfo2.retrigger", ParamValueType::BOOL},

    {LFO3_RATE, "lfo3.rate", ParamValueType::FLOAT},
    {LFO3_AMPLITUDE, "lfo3.amplitude", ParamValueType::FLOAT},
    {LFO3_RETRIGGER, "lfo3.retrigger", ParamValueType::BOOL},

    {PITCH_BEND_RANGE, "pitchBend.range", ParamValueType::FLOAT},

    {MASTER_GAIN, "master.gain", ParamValueType::FLOAT},

    {MONO_ENABLED, "mono.enabled", ParamValueType::BOOL},
    {MONO_LEGATO, "mono.legato", ParamValueType::BOOL},

    {PORTA_TIME, "porta.time", ParamValueType::FLOAT},
    {PORTA_LEGATO, "porta.legato", ParamValueType::BOOL},
    {PORTA_ENABLED, "porta.enabled", ParamValueType::BOOL},

};

inline constexpr ParamMapping PARAM_MAPPING_NOT_FOUND{PARAM_COUNT,
                                                      "not.found",
                                                      ParamValueType::BOOL};

inline constexpr size_t PARAM_NAME_COUNT = sizeof(PARAM_NAMES) / sizeof(PARAM_NAMES[0]);

struct ParamRouter {
  ParamID midiBindings[128];
  ParamBinding paramBindings[PARAM_COUNT];
};

using voices::VoicePool;

// ==== API Methods ====
void initParamRouter(ParamRouter& router, VoicePool& pool);

void handleMIDICC(ParamRouter& router, VoicePool& pool, uint8_t cc, uint8_t value);

float getParamValueByID(const ParamRouter& router,
                        ParamID id,
                        ParamValueFormat valueFormat = ParamValueFormat::DENORMALIZED);

void printParamList(const char* optionalParam);

void setParamValueByID(ParamRouter& router,
                       VoicePool& pool,
                       ParamID id,
                       float value,
                       ParamValueFormat valueFormat = ParamValueFormat::DENORMALIZED);

// String parsing helpers
ParamMapping findParamByName(const char* name);
const char* getParamName(ParamID id);

// Helpers for dealing with param values that are strings
SVFMode getSVFModeType(const char* inputValue);

} // namespace synth::param::bindings
