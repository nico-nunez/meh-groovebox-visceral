#include "PresetApply.h"

#include "synth/Engine.h"
#include "synth/ModMatrix.h"
#include "synth/ParamBindings.h"
#include "synth/ParamDefs.h"
#include "synth/ParamRanges.h"
#include "synth/ParamSync.h"
#include "synth/SignalChain.h"
#include "synth/WavetableBanks.h"
#include "synth/WavetableOsc.h"

#include "dsp/FX/FXChain.h"

#include <cstdint>
#include <cstdio>

namespace synth::preset {

namespace pb = param::bindings;

namespace osc = wavetable::osc;
namespace banks = wavetable::banks;

// ============================================================
// Helpers (anonymous)
// ============================================================
namespace {

// Clamp mod route amount based on destination type.
float clampModAmount(mod_matrix::ModDest dest, float amount) {
  using namespace mod_matrix;
  using namespace param::ranges::mod;

  switch (dest) {
  case SVFCutoff:
  case LadderCutoff:
    return clampCutoffMod(amount);
  case SVFResonance:
  case LadderResonance:
    return clampResonanceMod(amount);
  case Osc1Pitch:
  case Osc2Pitch:
  case Osc3Pitch:
  case Osc4Pitch:
    return clampPitchMod(amount);
  case Osc1Mix:
  case Osc2Mix:
  case Osc3Mix:
  case Osc4Mix:
    return clampMixLevelMod(amount);
  case Osc1ScanPos:
  case Osc2ScanPos:
  case Osc3ScanPos:
  case Osc4ScanPos:
    return clampScanPosMod(amount);
  case Osc1FMDepth:
  case Osc2FMDepth:
  case Osc3FMDepth:
  case Osc4FMDepth:
    return clampFMDepthMod(amount);
  case LFO1Rate:
  case LFO2Rate:
  case LFO3Rate:
    return clampLFORateMod(amount);
  case LFO1Amplitude:
  case LFO2Amplitude:
  case LFO3Amplitude:
    return clampLFOAmplitudeMod(amount);
  default:
    printf("Unknown mod matrix dest: %u", dest);
    return 0; // unknown dest — pass through
  }
}

} // namespace

// ============================================================
// Apply Preset
// ============================================================
ApplyResult applyPreset(const Preset& preset, Engine& engine) {
  ApplyResult result;
  auto& pool = engine.voicePool;
  auto& router = engine.paramRouter;

  // ==== All bound params in one loop ====
  for (int i = 0; i < param::PARAM_COUNT - 1; i++) {
    auto id = static_cast<param::ParamID>(i);
    pb::setParamValue(router, id, preset.paramValues[i]);
  }

  // ==== Enum fields — direct resolution, no string parsing ====
  osc::WavetableOsc* oscs[] = {&pool.osc1, &pool.osc2, &pool.osc3, &pool.osc4};
  for (int i = 0; i < NUM_OSCS; i++) {
    oscs[i]->bank = banks::getBankByID(preset.oscBanks[i]);
    oscs[i]->fmRouteCount = preset.oscFmRouteCounts[i];
    for (uint8_t r = 0; r < preset.oscFmRouteCounts[i]; r++)
      oscs[i]->fmRoutes[r] = preset.oscFmRoutes[i][r];
  }

  pool.noise.type = preset.noiseType;

  // SVF mode: paramValues[SVF_MODE] (set above via param loop) is authoritative.
  // preset.svfMode keeps the enum in sync for capturePreset round-trips.

  lfo::LFO* lfos[] = {&pool.lfo1, &pool.lfo2, &pool.lfo3};
  for (int i = 0; i < NUM_LFOS; i++) {
    lfos[i]->bank = wavetable::banks::getBankByID(preset.lfoBanks[i]);
    lfos[i]->subdivision = preset.lfoSubdivisions[i];
  }

  // ==== Mod matrix: rebuild from scratch ====
  mod_matrix::clearRoutes(pool.modMatrix);
  for (uint8_t i = 0; i < preset.modMatrixCount; i++) {
    const auto& r = preset.modMatrix[i];

    auto src = mod_matrix::parseModSrc(r.source.c_str());
    auto dest = mod_matrix::parseModDest(r.destination.c_str());

    if (src == mod_matrix::ModSrc::NoSrc || dest == mod_matrix::ModDest::NoDest) {
      result.warnings.push_back("mod route " + std::to_string(i) + ": invalid src/dest ('" +
                                r.source + "' -> '" + r.destination + "'), skipped");
      continue;
    }

    float amount = clampModAmount(dest, r.amount);
    mod_matrix::addRoute(pool.modMatrix, src, dest, amount);
  }
  mod_matrix::clearPrevModDests(pool.modMatrix);

  // ==== Signal chain ====
  pool.signalChain.length = 0;
  for (int i = 0; i < signal_chain::MAX_CHAIN_SLOTS; i++) {
    pool.signalChain.slots[i] = preset.signalChain[i];

    if (preset.signalChain[i] != signal_chain::SignalProcessor::None)
      pool.signalChain.length++;
  }

  engine.fxChain.delay.subdivision = preset.delaySubdivision;

  // ==== FX chain ordering ====
  engine.fxChain.length = preset.fxChainLength;
  for (uint8_t i = 0; i < dsp::fx::chain::MAX_EFFECT_SLOTS; i++)
    engine.fxChain.slots[i] = preset.fxChain[i];

  engine.dirtyFlags.markAll();
  param::sync::syncDirtyParams(engine);

  engine.fxChain.delay.state.currentDelaySamples = engine.fxChain.delay.targetDelaySamples;

  return result;
}

// ============================================================
// capturePreset
// ============================================================

Preset capturePreset(const Engine& engine) {
  const auto& pool = engine.voicePool;
  const auto& router = engine.paramRouter;

  Preset p;
  p.version = CURRENT_PRESET_VERSION;

  // ==== All bound params ====
  for (int i = 0; i < param::PARAM_COUNT - 1; i++) {
    auto id = static_cast<param::ParamID>(i);
    p.paramValues[i] = pb::getParamValueByID(router, id);
  }

  // ==== Enum fields — read directly from engine ====
  const osc::WavetableOsc* oscs[] = {&pool.osc1, &pool.osc2, &pool.osc3, &pool.osc4};
  for (int i = 0; i < NUM_OSCS; i++) {
    p.oscBanks[i] = oscs[i]->bank ? banks::parseBankID(oscs[i]->bank->name) : BankID::Sine;
    p.oscFmRouteCounts[i] = oscs[i]->fmRouteCount;
    for (uint8_t r = 0; r < oscs[i]->fmRouteCount; r++)
      p.oscFmRoutes[i][r] = oscs[i]->fmRoutes[r];
  }

  p.noiseType = pool.noise.type;
  p.svfMode = pool.svf.mode;

  const lfo::LFO* lfos[] = {&pool.lfo1, &pool.lfo2, &pool.lfo3};
  for (int i = 0; i < NUM_LFOS; i++) {
    p.lfoBanks[i] = lfos[i]->bank ? banks::parseBankID(lfos[i]->bank->name) : BankID::SampleAndHold;
    p.lfoSubdivisions[i] = lfos[i]->subdivision;
  }

  // ==== Mod matrix ====
  p.modMatrixCount = pool.modMatrix.count;
  for (uint8_t i = 0; i < pool.modMatrix.count; i++) {
    const auto& route = pool.modMatrix.routes[i];
    p.modMatrix[i].source = mod_matrix::modSrcToString(route.src);
    p.modMatrix[i].destination = mod_matrix::modDestToString(route.dest);
    p.modMatrix[i].amount = route.amount;
  }

  // ==== Signal chain ====
  for (uint8_t i = 0; i < pool.signalChain.length && i < signal_chain::MAX_CHAIN_SLOTS; i++) {
    p.signalChain[i] = i < pool.signalChain.length ? pool.signalChain.slots[i]
                                                   : signal_chain::SignalProcessor::None;
  }

  // ==== FX chain ordering ====
  p.fxChainLength = engine.fxChain.length;
  for (uint8_t i = 0; i < dsp::fx::chain::MAX_EFFECT_SLOTS; i++)
    p.fxChain[i] = engine.fxChain.slots[i];

  // ==== Delay subdivision ====
  p.delaySubdivision = engine.fxChain.delay.subdivision;

  return p;
}

void printPreset(const Preset& p) {
  // --- Metadata ---
  printf("[Preset: %s]\n", p.metadata.name.c_str());
  if (!p.metadata.category.empty())
    printf("  category:  %s\n", p.metadata.category.c_str());
  if (!p.metadata.author.empty())
    printf("  author:    %s\n", p.metadata.author.c_str());
  printf("  version:   %u\n", p.version);

  // --- Params ---
  printf("[Params]\n");
  for (int i = 0; i < param::PARAM_COUNT - 1; i++) {
    const auto& def = param::PARAM_DEFS[i];
    float v = p.paramValues[i];
    switch (def.type) {

    case param::ParamType::Float:
      printf("  %-28s %.3f\n", def.name, v);
      break;

    case param::ParamType::Bool:
      printf("  %-28s %s\n", def.name, v >= 0.5f ? "true" : "false");
      break;

    case param::ParamType::Int8:
    case param::ParamType::PhaseMode:
    case param::ParamType::FilterMode:
    case param::ParamType::DistortionType:
      printf("  %-28s %d\n", def.name, static_cast<int>(v));
      break;
    }
  }

  // --- Enum Fields ---
  printf("[Enum Fields]\n");
  const char* oscKeys[] = {"osc1", "osc2", "osc3", "osc4"};
  for (int i = 0; i < NUM_OSCS; i++) {
    printf("  %s.bank  %s\n", oscKeys[i], banks::bankIDToString(p.oscBanks[i]));
    if (p.oscFmRouteCounts[i] == 0) {
      printf("  %s.fmRoutes  (none)\n", oscKeys[i]);
    } else {
      for (uint8_t r = 0; r < p.oscFmRouteCounts[i]; r++)
        printf("  %s.fmRoutes[%u]  source=%-6s depth=%.3f\n",
               oscKeys[i],
               r,
               osc::fmSourceToString(p.oscFmRoutes[i][r].source),
               p.oscFmRoutes[i][r].depth);
    }
  }
  printf("  noise.type              %s\n", noise::noiseTypeToString(p.noiseType));
  printf("  svf.mode (enum)         %s\n", filters::svfModeToString(p.svfMode));

  const char* lfoKeys[] = {"lfo1", "lfo2", "lfo3"};
  for (int i = 0; i < NUM_LFOS; i++) {
    printf("  %s.bank     %s\n", lfoKeys[i], banks::bankIDToString(p.lfoBanks[i]));
  }

  // --- Mod Matrix ---
  printf("[Mod Matrix]  (%u routes)\n", p.modMatrixCount);
  for (uint8_t i = 0; i < p.modMatrixCount; i++) {
    const auto& r = p.modMatrix[i];
    printf("  %u: %-12s -> %-20s amount=%.3f\n",
           i,
           r.source.c_str(),
           r.destination.c_str(),
           r.amount);
  }

  // --- Signal Chain ---
  printf("[Signal Chain]\n");
  for (int i = 0; i < signal_chain::MAX_CHAIN_SLOTS; i++) {
    if (p.signalChain[i] == signal_chain::SignalProcessor::None)
      break;
    printf("  %d: %s\n", i, signal_chain::signalProcessorToString(p.signalChain[i]));
  }
}
} // namespace synth::preset
