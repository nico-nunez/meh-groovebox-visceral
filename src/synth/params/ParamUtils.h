#pragma once

#include "ParamDefs.h"
#include "synth/Engine.h"
#include "synth/ModMatrix.h"
#include "synth/SignalChain.h"
#include "synth/WavetableOsc.h"

#include "dsp/fx/FXChain.h"
#include "synth/params/ParamRouter.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace synth::param::utils {

using wavetable::osc::FMCarrier;
using wavetable::osc::FMSource;

inline constexpr uint8_t MAX_SIGNAL_CHAIN_SLOTS = signal_chain::MAX_CHAIN_SLOTS;

// =====================
// Param Event Helpers
// =====================
inline ParamID getParamIDByName(const char* name) {
  for (int i = 0; i < PARAM_COUNT; ++i) {
    if (strcmp(PARAM_DEFS[i].name, name) == 0)
      return static_cast<ParamID>(i);
  }
  return ParamID::UNKNOWN;
}

inline float getParamValueByID(const Engine* eng, ParamID id) {
  return router::getParamValueByID(eng->paramRouter, id);
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

struct ParseResult {
  bool ok{false};
  uint8_t value{0};
  const char* error{nullptr};
};

struct VoidResult {
  bool ok{false};
  const char* error{nullptr};
};

const char* enumToString(ParamType pType, uint8_t pEnum);
ParseResult parseEnum(ParamType pType, const char* token);

// =====================
// Engine Event Helpers
// =====================

inline ParseResult parseEnumFM(const char* token) {
  using namespace wavetable::osc;

  for (auto& map : FM_MAPPINGS) {
    if (strcmp(token, map.name) == 0)
      return {true, map.id, nullptr};
  }
  return {false, FMSource::None, "unknown FM oscillator: default is 'none'"};
}

inline ParseResult parseModSrc(const char* srcName) {
  auto src = mod_matrix::parseModSrc(srcName);
  if (src == mod_matrix::ModSrc::NoSrc)
    return {false, src, "unknown mod source"};
  return {true, src, nullptr};
}

inline ParseResult parseModDest(const char* destName) {
  auto dest = mod_matrix::parseModDest(destName);
  if (dest == mod_matrix::ModDest::NoDest)
    return {false, dest, "unknown mod destination"};
  return {true, dest, nullptr};
}

inline VoidResult printModList(const Engine* eng) {
  mod_matrix::printRoutes(eng->voicePool.modMatrix);
  return {true, nullptr};
}

inline ParseResult parseFXProcessor(const char* name) {
  auto proc = dsp::fx::chain::parseFXProcessor(name);
  if (proc == dsp::fx::chain::FXProcessor::None) {
    return {false, proc, "uknown fx processor"};
  }
  return {true, proc, nullptr};
}

inline VoidResult printFMList(Engine* eng, const char* carrierName) {
  auto carrier = voices::getOscByName(eng->voicePool, carrierName);
  if (!carrier)
    return {false, "unknown fm carrier"};
  wavetable::osc::printFMRoutes(*carrier, carrierName);
  return {true, nullptr};
}

inline ParseResult parseSignalProcessor(const char* name) {
  auto sigProc = signal_chain::parseSignalProcessor(name);
  if (sigProc == signal_chain::SignalProcessor::None) {
    return {false, sigProc, "uknown signal processor"};
  }
  return {true, sigProc, nullptr};
}

inline VoidResult printSignalChain(const Engine* eng) {
  signal_chain::printSigChain(eng->voicePool.signalChain);
  return {true, nullptr};
}

} // namespace synth::param::utils
