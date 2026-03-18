#pragma once

#include "dsp/Buffers.h"
#include "dsp/FX/Chorus.h"
#include "dsp/FX/Delay.h"
#include "dsp/FX/Distortion.h"
#include "dsp/FX/Phaser.h"
#include "dsp/FX/Reverb.h"

#include <cstdint>

namespace synth::fx_chain {

using dsp::fx::chorus::ChorusFX;
using dsp::fx::delay::DelayFX;
using dsp::fx::distortion::DistortionFX;
using dsp::fx::phaser::PhaserFX;
using dsp::fx::reverb::ReverbFX;

using dsp::buffers::StereoBuffer;

constexpr uint8_t MAX_EFFECT_SLOTS = 8;

enum FXProcessor : uint8_t {
  None = 0,
  Distortion,
  Chorus,
  Phaser,
  Delay,
  ReverbPlate, // Dattorro plate — named for future ReverbRoom (FDN) alongside it
};

struct FXChain {
  DistortionFX distortion{};
  ChorusFX chorus;
  PhaserFX phaser;
  DelayFX delay;
  ReverbFX reverb;

  // Ordered processing slot array
  FXProcessor slots[MAX_EFFECT_SLOTS];
  uint8_t length = 0;
};

// --- String table for serialization (mirrors SignalProcessorMapping) ---
struct FXProcessorMapping {
  const char* name;
  FXProcessor proc;
};

inline constexpr FXProcessorMapping effectProcessorMappings[] = {
    {"distortion", FXProcessor::Distortion},
    {"chorus", FXProcessor::Chorus},
    {"phaser", FXProcessor::Phaser},
    {"delay", FXProcessor::Delay},
    {"reverb", FXProcessor::ReverbPlate},
};

inline FXProcessor parseFXProcessor(const char* name) {
  for (const auto& m : effectProcessorMappings)
    if (std::strcmp(m.name, name) == 0)
      return m.proc;
  return FXProcessor::None;
}

inline const char* fxProcessorToString(FXProcessor proc) {
  for (const auto& m : effectProcessorMappings)
    if (m.proc == proc)
      return m.name;
  return "unknown";
}

void initFXChain(FXChain& fxChain, float bpm, float sampleRate);
void destroyFXChain(FXChain& fxChain);
void processFXChain(FXChain& fxChain, StereoBuffer buf, size_t numSamples, float sampleRate);

} // namespace synth::fx_chain
