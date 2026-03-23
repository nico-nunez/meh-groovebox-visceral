// Preset.h
#pragma once

#include "synth/FXChain.h"
#include "synth/Filters.h"
#include "synth/ModMatrix.h"
#include "synth/Noise.h"
#include "synth/ParamDefs.h"
#include "synth/SignalChain.h"
#include "synth/Tempo.h"
#include "synth/Types.h"
#include "synth/WavetableBanks.h"
#include "synth/WavetableOsc.h"

#include <cstdint>
#include <string>

namespace synth::preset {

using wavetable::banks::BankID;
using wavetable::osc::FMRoute;

using noise::NoiseType;

using filters::SVFMode;
using signal_chain::SignalProcessor;

using fx_chain::FXProcessor;
using tempo::Subdivision;

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

  // All bound params, indexed by ParamID.
  // Defaults filled from PARAM_DEFS[].defaultVal.
  float paramValues[param::PARAM_COUNT];

  // Enum fields — the serializer converts these to/from JSON strings at the boundary.
  // Internally everything speaks enum; no strings in the data model.
  BankID oscBanks[NUM_OSCS] = {}; // BankID(0) = Sine

  FMRoute oscFmRoutes[NUM_OSCS][NUM_OSCS] = {};
  uint8_t oscFmRouteCounts[NUM_OSCS] = {};

  NoiseType noiseType = NoiseType::White;

  BankID lfoBanks[NUM_LFOS] = {}; // BankID(0) = Sine
  Subdivision lfoSubdivisions[NUM_LFOS] = {Subdivision::Quarter,
                                           Subdivision::Quarter,
                                           Subdivision::Quarter};

  ModRoutePreset modMatrix[mod_matrix::MAX_MOD_ROUTES]{};
  uint8_t modMatrixCount = 0;

  SVFMode svfMode = SVFMode::LP;
  SignalProcessor signalChain[signal_chain::MAX_CHAIN_SLOTS] = {SignalProcessor::SVF,
                                                                SignalProcessor::Ladder,
                                                                SignalProcessor::Saturator};

  // ==== Effects ====
  Subdivision delaySubdivision = Subdivision::Quarter;
  // Non-param subsystems

  FXProcessor fxChain[fx_chain::MAX_EFFECT_SLOTS] = {
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
