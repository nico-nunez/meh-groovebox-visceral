#pragma once

#include "Chorus.h"
#include "Delay.h"
#include "Distortion.h"
#include "Phaser.h"
#include "Reverb.h"

#include "dsp/Buffers.h"

#include <cstdint>
#include <sstream>

namespace dsp::fx::chain {

using chorus::ChorusFX;
using delay::DelayFX;
using distortion::DistortionFX;
using phaser::PhaserFX;
using reverb::ReverbFX;

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

void processFXChain(FXChain& fxChain, StereoBuffer buf, size_t numSamples);

// ====================
// Command Helpers
// ====================

void parseFXChainCmd(std::istringstream& iss, FXChain& fxChain);

void setFXChain(FXChain& fxChain, const FXProcessor* procs, uint8_t count);
void clearFXChain(FXChain& fxChain);

inline void printFXChain(const FXChain& chain) {
  if (chain.length == 0) {
    printf("fx chain: empty\n");
    return;
  }
  printf("fx chain:");
  for (uint8_t i = 0; i < chain.length; i++)
    printf(" %s", fxProcessorToString(chain.slots[i]));
  printf("\n");
}
} // namespace dsp::fx::chain
