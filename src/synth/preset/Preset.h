#pragma once

#include "synth/ModMatrix.h"
#include "synth/SignalChain.h"
#include "synth/WavetableOsc.h"
#include "synth/params/ParamDefs.h"

#include "dsp/Tempo.h"
#include "dsp/fx/FXChain.h"

#include <cstdint>
#include <string>

namespace synth::preset {

using wavetable::osc::FMRoute;

using signal_chain::SignalProcessor;

using dsp::fx::chain::FXProcessor;

using dsp::tempo::Subdivision;

inline constexpr uint32_t CURRENT_PRESET_VERSION = 1;

inline constexpr uint8_t NUM_OSCS = 4;
inline constexpr const char* OSC_KEYS[] = {"osc1", "osc2", "osc3", "osc4"};

inline constexpr uint8_t NUM_LFOS = 3;
inline constexpr const char* LFO_KEYS[] = {"lfo1", "lfo2", "lfo3"};

// ============================================================
// Mod routes + signal chain (no ParamID equivalent)
// ============================================================

struct ModRoutePreset {
  std::string source;
  std::string destination;
  float amount = 0.0f;
};

struct PresetMetadata {
  std::string name = "Init";
  std::string author;
  std::string category;
  std::string description;
};

// ============================================================
// String fields — values that are strings in JSON but
// pointers/enums in the engine. Resolved at apply time.
// ============================================================

struct Preset {
  uint32_t version = CURRENT_PRESET_VERSION;
  PresetMetadata metadata;

  // Defaults filled from PARAM_DEFS[].defaultVal.
  float paramValues[param::PARAM_COUNT];

  FMRoute oscFmRoutes[NUM_OSCS][NUM_OSCS] = {};
  uint8_t oscFmRouteCounts[NUM_OSCS] = {};

  ModRoutePreset modMatrix[mod_matrix::MAX_MOD_ROUTES]{};
  uint8_t modMatrixCount = 0;

  SignalProcessor signalChain[signal_chain::MAX_CHAIN_SLOTS] = {SignalProcessor::SVF,
                                                                SignalProcessor::Ladder,
                                                                SignalProcessor::Saturator};

  FXProcessor fxChain[dsp::fx::chain::MAX_EFFECT_SLOTS] = {
      FXProcessor::Distortion,
      FXProcessor::Chorus,
      FXProcessor::Phaser,
      FXProcessor::Delay,
      FXProcessor::ReverbPlate,
  };
  uint8_t fxChainLength = 5;
};

inline Preset createInitPreset() {
  Preset p;
  p.metadata.name = "Init";
  p.metadata.category = "Init";
  p.metadata.description = "Clean starting point";

  for (int i = 0; i < param::PARAM_COUNT - 1; i++) {
    p.paramValues[i] = param::PARAM_DEFS[i].defaultVal;
  }

  return p;
}

inline float getPresetValue(const Preset& p, param::ParamID id) {
  return p.paramValues[id];
}

} // namespace synth::preset
