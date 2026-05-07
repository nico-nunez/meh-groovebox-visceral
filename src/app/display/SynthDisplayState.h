#pragma once

#include "synth/Engine.h"
#include "synth/params/ParamDefs.h"

#include <cstdint>

namespace app::display {

namespace {
using synth::param::ParamDef;
using synth::param::ParamID;
} // namespace

inline constexpr uint8_t DISPLAY_SYNTH_PARAM_CAPACITY = 64;

struct SynthParamTelemetry {
  ParamID id = ParamID::PARAM_UNKNOWN;
  float value = 0.0f;
};

struct SynthRuntimeTelemetry {
  uint32_t noteCount = 0;
  SynthParamTelemetry params[DISPLAY_SYNTH_PARAM_CAPACITY]{};
  uint8_t paramCount = 0;
};

struct SynthParamDisplayValue {
  ParamID id = ParamID::PARAM_UNKNOWN;
  const ParamDef* def = nullptr;
  float value = 0.0f;
};

struct SynthSummarySnapshot {
  uint32_t noteCount = 0;
  SynthParamDisplayValue params[DISPLAY_SYNTH_PARAM_CAPACITY]{};
  uint8_t paramCount = 0;
};

const ParamID* curatedSynthParamIDs();
uint8_t curatedSynthParamCount();

void fillSynthRuntimeTelemetry(SynthRuntimeTelemetry& out, const synth::Engine& engine);
SynthSummarySnapshot makeSynthSummarySnapshot(const SynthRuntimeTelemetry& telemetry);

const SynthParamDisplayValue* findSynthParam(const SynthSummarySnapshot& snapshot,
                                             synth::param::ParamID id);

} // namespace app::display
