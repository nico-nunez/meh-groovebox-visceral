#include "FXChain.h"

#include "dsp/Buffers.h"
#include "dsp/FX/Delay.h"
#include "dsp/FX/Distortion.h"
#include "dsp/FX/Phaser.h"

namespace synth::fx_chain {
using namespace dsp::fx;
using dsp::buffers::StereoBuffer;

void initFXChain(FXChain& fxChain, float sampleRate) {
  using namespace dsp::fx;
  chorus::initChorusState(fxChain.chorus.state, sampleRate);
  phaser::initPhaserState(fxChain.phaser.state, sampleRate);
  delay::initDelayState(fxChain.delay.state, sampleRate);
  reverb::initReverbState(fxChain.reverb.state, sampleRate);

  fxChain.length = 5;
  fxChain.slots[0] = FXProcessor::Distortion;
  fxChain.slots[1] = FXProcessor::Chorus;
  fxChain.slots[2] = FXProcessor::Phaser;
  fxChain.slots[3] = FXProcessor::Delay;
  fxChain.slots[4] = FXProcessor::ReverbPlate;
  // All effects disabled by default — applyPreset sets enabled flags
}

void destroyFXChain(FXChain& fxChain) {
  using namespace dsp::fx;
  chorus::destroyChorusState(fxChain.chorus.state);
  phaser::destroyPhaserState(fxChain.phaser.state);
  delay::destroyDelayState(fxChain.delay.state);
  reverb::destroyReverbState(fxChain.reverb.state);
}

void processFXChain(FXChain& fxChain, StereoBuffer buf, size_t numSamples, float sampleRate) {
  using namespace dsp::fx;

  for (uint8_t i = 0; i < fxChain.length; i++) {
    FXProcessor& fx = fxChain.slots[i];

    switch (fx) {
    case FXProcessor::Chorus:
      if (fxChain.chorus.enabled)
        chorus::processChorus(fxChain.chorus, buf, numSamples, sampleRate);
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
        phaser::processPhaser(fxChain.phaser, buf, numSamples, sampleRate);
      break;

    case FXProcessor::ReverbPlate:
      if (fxChain.reverb.enabled)
        reverb::processReverb(fxChain.reverb, buf, numSamples, sampleRate);
      break;

    default:
      break;
    }
  }
}
} // namespace synth::fx_chain
