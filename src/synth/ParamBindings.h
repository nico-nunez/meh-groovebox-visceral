#pragma once

#include "dsp/FX/Distortion.h"
#include "synth/FXChain.h"
#include "synth/Filters.h"
#include "synth/ParamDefs.h"
#include "synth/Tempo.h"
#include "synth/VoicePool.h"
#include "synth/WavetableOsc.h"

#include <cstddef>
#include <cstdint>

namespace synth::param::bindings {
using dsp::fx::distortion::DistortionType;
using filters::SVFMode;
using wavetable::osc::PhaseMode;

struct ParamBinding {
  union {
    float* floatPtr;
    int8_t* int8Ptr;
    bool* boolPtr;
    SVFMode* svfModePtr;
    DistortionType* distortionTypePtr;
    PhaseMode* phaseModePtr;
  };
};

struct ParamRouter {
  ParamID midiBindings[128];
  ParamBinding paramBindings[PARAM_COUNT];
};

struct OscParamIDs {
  ParamID mixLevel, detune, octave, scanPos, phaseMode, randomRange, resetPhase, fmDepth, ratio,
      fixed, fixedFreq, enabled;
};

struct EnvParamIDs {
  ParamID attack, decay, sustain, release, attackCurve, decayCurve, releaseCurve;
};

struct LFOParamIDs {
  ParamID rate, amplitude, retrigger, tempoSync, delay, attack;
};

// One bundle per instance, order matches the ParamID enum layout.
inline constexpr OscParamIDs OSC_PARAM_IDS[4] = {
    {OSC1_MIX_LEVEL,
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

    {OSC2_MIX_LEVEL,
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

    {OSC3_MIX_LEVEL,
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

    {OSC4_MIX_LEVEL,
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
    {LFO1_RATE, LFO1_AMPLITUDE, LFO1_RETRIGGER, LFO1_TEMPO_SYNC, LFO1_DELAY, LFO1_ATTACK},
    {LFO2_RATE, LFO2_AMPLITUDE, LFO2_RETRIGGER, LFO2_TEMPO_SYNC, LFO2_DELAY, LFO2_ATTACK},
    {LFO3_RATE, LFO3_AMPLITUDE, LFO3_RETRIGGER, LFO3_TEMPO_SYNC, LFO3_DELAY, LFO3_ATTACK},
};

// ==== API ====
void initParamRouter(ParamRouter& router, voices::VoicePool& pool, tempo::TempoState& tempo);

void initFXParamBindings(ParamRouter& router, fx_chain::FXChain& fxChain);

ParamID handleMIDICC(ParamRouter& router,
                     voices::VoicePool& pool,
                     uint8_t cc,
                     uint8_t value,
                     float sampleRate);

float getParamValueByID(const ParamRouter& router, ParamID id);
void setParamValue(ParamRouter& router, ParamID id, float value);

// String parsing helpers
ParamID getParamIDByName(const char* paramName);
const char* getParamName(ParamID id);

void printParamList(const char* optionalParam);

} // namespace synth::param::bindings
