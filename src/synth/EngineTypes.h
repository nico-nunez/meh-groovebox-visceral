#pragma once

#include "dsp/Tempo.h"
#include "dsp/fx/Distortion.h"
#include "synth/Filters.h"
#include "synth/Noise.h"
#include "synth/WavetableBanks.h"
#include "synth/WavetableOsc.h"

namespace synth::types {
using dsp::fx::distortion::DistortionType;
using dsp::tempo::Subdivision;
using filters::SVFMode;
using noise::NoiseType;
using wavetable::banks::BankID;
using wavetable::osc::PhaseMode;

} // namespace synth::types
