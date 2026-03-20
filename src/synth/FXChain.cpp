#include "FXChain.h"

#include "dsp/Buffers.h"
#include "dsp/FX/Chorus.h"
#include "dsp/FX/Delay.h"
#include "dsp/FX/Distortion.h"
#include "dsp/FX/Phaser.h"
#include <algorithm>

namespace synth::fx_chain {
using namespace dsp::fx;
using dsp::buffers::StereoBuffer;

void initFXChain(FXChain& fxChain, float bpm, float sampleRate) {
  using namespace dsp::fx;

  chorus::initChorus(fxChain.chorus, sampleRate);
  phaser::initPhaser(fxChain.phaser, sampleRate);
  delay::initDelay(fxChain.delay, bpm, sampleRate);
  reverb::initReverb(fxChain.reverb, sampleRate);

  // Presets disabled as default
  fxChain.length = 5;
  fxChain.slots[0] = FXProcessor::Distortion;
  fxChain.slots[1] = FXProcessor::Chorus;
  fxChain.slots[2] = FXProcessor::Phaser;
  fxChain.slots[3] = FXProcessor::Delay;
  fxChain.slots[4] = FXProcessor::ReverbPlate;
}

void destroyFXChain(FXChain& fxChain) {
  using namespace dsp::fx;
  chorus::destroyChorus(fxChain.chorus);
  phaser::destroyPhaser(fxChain.phaser);
  delay::destroyDelay(fxChain.delay);
  reverb::destroyReverb(fxChain.reverb);
}

void processFXChain(FXChain& fxChain, StereoBuffer buf, size_t numSamples) {
  using namespace dsp::fx;

  for (uint8_t i = 0; i < fxChain.length; i++) {
    FXProcessor& fx = fxChain.slots[i];

    switch (fx) {
    case FXProcessor::Chorus:
      if (fxChain.chorus.enabled)
        chorus::processChorus(fxChain.chorus, buf, numSamples);
      break;

    case FXProcessor::Delay:
      if (fxChain.delay.enabled)
        delay::processDelay(fxChain.delay, buf, numSamples);
      break;

    case FXProcessor::Distortion:
      if (fxChain.distortion.enabled)
        distortion::processDistortion(fxChain.distortion, buf, numSamples);
      break;

    case FXProcessor::Phaser:
      if (fxChain.phaser.enabled)
        phaser::processPhaser(fxChain.phaser, buf, numSamples);
      break;

    case FXProcessor::ReverbPlate:
      if (fxChain.reverb.enabled)
        reverb::processReverb(fxChain.reverb, buf, numSamples);
      break;

    default:
      break;
    }
  }
}

// ====================
// Command Helpers
// ====================

void setFXChain(FXChain& fxChain, const FXProcessor* procs, uint8_t count) {
  fxChain.length = std::min(count, MAX_EFFECT_SLOTS);
  for (uint8_t i = 0; i < fxChain.length; i++)
    fxChain.slots[i] = procs[i];
}

void clearFXChain(FXChain& fxChain) {
  for (uint8_t i = 0; i < MAX_EFFECT_SLOTS; i++)
    fxChain.slots[i] = FXProcessor::None;
  fxChain.length = 0;
}

void parseFXChainCmd(std::istringstream& iss, FXChain& fxChain) {
  std::string subcmd;
  iss >> subcmd;

  if (subcmd == "set") {
    FXProcessor procs[MAX_EFFECT_SLOTS];
    uint8_t count = 0;
    std::string name;

    while (iss >> name && count < MAX_EFFECT_SLOTS) {
      FXProcessor p = parseFXProcessor(name.c_str());
      if (p == FXProcessor::None) {
        printf("fxChain: unknown effect '%s'\n", name.c_str());
        return;
      }
      procs[count++] = p;
    }

    setFXChain(fxChain, procs, count);
    printf("OK\n");

  } else if (subcmd == "list") {
    if (fxChain.length == 0) {
      printf("fx chain: (empty)\n");
      return;
    }
    printf("fx chain: ");
    for (uint8_t i = 0; i < fxChain.length; i++) {
      if (i > 0)
        printf(" -> ");
      printf("%s", fxProcessorToString(fxChain.slots[i]));
    }
    printf("\n");

  } else if (subcmd == "clear") {
    clearFXChain(fxChain);
    printf("OK\n");

  } else {
    printf("fxChain subcommands: set <effect...>, list, clear\n");
    printf("valid effects: distortion, chorus, phaser, delay, reverb\n");
  }
}

} // namespace synth::fx_chain
