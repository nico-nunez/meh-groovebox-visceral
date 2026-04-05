#pragma once

#include "ParamDefs.h"

#include "synth/Filters.h"
#include "synth/Noise.h"
#include "synth/VoicePool.h"
#include "synth/WavetableBanks.h"
#include "synth/WavetableOsc.h"

#include "dsp/Tempo.h"
#include "dsp/fx/Distortion.h"
#include "dsp/fx/FXChain.h"

#include <cstddef>
#include <cstdint>

namespace synth::param::router {
using dsp::fx::chain::FXChain;
using dsp::fx::distortion::DistortionType;

using dsp::tempo::Subdivision;
using filters::SVFMode;
using noise::NoiseType;
using wavetable::banks::BankID;
using wavetable::osc::PhaseMode;

struct ParamBinding {
  union {
    float* floatPtr;
    int8_t* int8Ptr;
    bool* boolPtr;
    BankID* oscBankPtr;
    PhaseMode* phaseModePtr;
    NoiseType* noiseType;
    SVFMode* svfModePtr;
    DistortionType* distortionTypePtr;
    Subdivision* subdivision;
  };
};

struct ParamRouter {
  ParamID midiBindings[128];
  ParamBinding paramBindings[PARAM_COUNT];
};

// ==== API ====
void initParamRouter(ParamRouter& router, voices::VoicePool& pool, float& bpm);

void initFXParamRouter(ParamRouter& router, FXChain& fxChain);

ParamID handleMIDICC(ParamRouter& router,
                     voices::VoicePool& pool,
                     uint8_t cc,
                     uint8_t value,
                     float sampleRate);

float getParamValueByID(const ParamRouter& router, ParamID id);
void setParamValue(ParamRouter& router, ParamID id, float value);

} // namespace synth::param::router
