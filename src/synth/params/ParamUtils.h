#pragma once

#include "ParamDefs.h"

#include "synth/Filters.h"
#include "synth/Noise.h"
#include "synth/WavetableBanks.h"
#include "synth/WavetableOsc.h"

#include "dsp/Tempo.h"
#include "dsp/fx/Distortion.h"

#include <cstdio>

namespace synth::param::utils {

using dsp::fx::distortion::parseDistortionType;
using dsp::tempo::parseSubdivision;
using filters::parseSVFMode;
using noise::parseNoiseType;
using wavetable::banks::parseBankID;
using wavetable::osc::parsePhaseMode;

inline ParamID getParamIDByName(const char* name) {
  for (int i = 0; i < PARAM_COUNT; ++i) {
    if (strcmp(PARAM_DEFS[i].name, name) == 0)
      return static_cast<ParamID>(i);
  }
  return synth::param::ParamID::UNKNOWN;
}

inline const char* getParamName(const ParamID id) {
  if (id >= 0 && id < PARAM_COUNT)
    return PARAM_DEFS[id].name;
  return nullptr;
}

inline void printParamList(const char* optionalFilter) {
  if (optionalFilter != nullptr) {
    for (const auto& def : PARAM_DEFS) {
      if (strstr(def.name, optionalFilter) != nullptr)
        printf("  %s\n", def.name);
    }
    return;
  }

  for (const auto& def : PARAM_DEFS)
    printf("  %s\n", def.name);
}

} // namespace synth::param::utils
