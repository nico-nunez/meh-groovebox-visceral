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
