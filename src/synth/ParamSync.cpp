#include "ParamSync.h"

#include "synth/Engine.h"
#include "synth/Envelope.h"
#include "synth/FXChain.h"
#include "synth/VoicePool.h"

#include "dsp/FX/Chorus.h"
#include "dsp/FX/Delay.h"
#include "dsp/FX/Distortion.h"
#include "dsp/FX/Phaser.h"
#include "dsp/Math.h"

namespace synth::param::sync {
namespace {
using fx_chain::FXChain;
using voices::VoicePool;

namespace dist = dsp::fx::distortion;
namespace chorus = dsp::fx::chorus;
namespace phaser = dsp::fx::phaser;
namespace delay = dsp::fx::delay;

void updateOscMixGain(VoicePool& pool) {
  int count = pool.osc1.enabled + pool.osc2.enabled + pool.osc3.enabled + pool.osc4.enabled +
              pool.noise.enabled;
  pool.oscMixGain = (count > 0) ? 1.0f / static_cast<float>(count) : 1.0f;
}

void updateAllEnvIncrements(VoicePool& pool, const float sampleRate) {
  envelope::updateIncrements(pool.ampEnv, sampleRate);
  envelope::updateIncrements(pool.filterEnv, sampleRate);
  envelope::updateIncrements(pool.modEnv, sampleRate);
}

void updateAllEnvCurves(VoicePool& pool) {
  envelope::updateCurveTables(pool.ampEnv);
  envelope::updateCurveTables(pool.filterEnv);
  envelope::updateCurveTables(pool.modEnv);
}

void updateSVFCoeff(VoicePool& pool, const float invSampleRate) {
  filters::updateSVFCoefficients(pool.svf, invSampleRate);
}

void updateLadderCoeff(VoicePool& pool, const float invSampleRate) {
  filters::updateLadderCoefficient(pool.ladder, invSampleRate);
}

void updateSaturatorDerived(VoicePool& pool) {
  pool.saturator.invDrive = saturator::calcInvDrive(pool.saturator.drive);
}

void updateMonoEnabled(VoicePool& pool) {
  if (pool.mono.enabled) {
    for (uint32_t i = 0; i < pool.activeCount; i++) {
      uint32_t v = pool.activeIndices[i];
      envelope::triggerRelease(pool.ampEnv, v);
      envelope::triggerRelease(pool.filterEnv, v);
      envelope::triggerRelease(pool.modEnv, v);
    }
    pool.mono.voiceIndex = MAX_VOICES;
    pool.mono.stackDepth = 0;
  } else {
    voices::releaseMonoVoice(pool);
  }
}

void updatePortaCoeff(VoicePool& pool, const float sampleRate) {
  pool.porta.coeff = dsp::math::calcPortamento(pool.porta.time, sampleRate);
}

void updateUnisonDerived(VoicePool& pool) {
  if (pool.unison.enabled) {
    unison::updateDetuneOffsets(pool.unison);
    unison::updatePanPositions(pool.unison);
    unison::updateGainComp(pool.unison);
  }
}

void updateUnisonSpread(VoicePool& pool) {
  unison::updatePanPositions(pool.unison);
}

void updateAllLFOEffectiveRates(VoicePool& pool, const float bpm) {
  for (lfo::LFO* lfo : {&pool.lfo1, &pool.lfo2, &pool.lfo3}) {
    lfo->effectiveRate =
        lfo->tempoSync ? tempo::calcEffectiveRate(lfo->subdivision, bpm) : lfo->rate;
  }
}

void updateChorusDerived(FXChain& fxChain, const float sampleRate) {
  chorus::recalcChorusDerivedVals(fxChain.chorus, sampleRate);
}

void updateDistortionDerived(FXChain& fxChain) {
  fxChain.distortion.invNorm = dist::calcDistortionInvNorm(fxChain.distortion.drive);
}

void updatePhaserDerived(FXChain& fxChain, const float invSampleRate) {
  phaser::recalcPhaseDerivedVals(fxChain.phaser, invSampleRate);
}

void updateDelayDerived(FXChain& fxChain, const float bpm, const float sampleRate) {
  delay::recalcTargetDelaySamples(fxChain.delay, bpm, sampleRate);
}

void updateDelayDamping(FXChain& fxChain) {
  delay::recalcDerivedDampCoeff(fxChain.delay);
}

void updateBPMSync(VoicePool& pool, FXChain& fxChain, const float bpm, const float sampleRate) {
  auto& delay = fxChain.delay;

  updateAllLFOEffectiveRates(pool, bpm);

  if (delay.tempoSync)
    updateDelayDerived(fxChain, bpm, sampleRate);
}

} // anonymous namespace

void syncDirtyParams(Engine& engine) {
  using param::UpdateGroup;
  auto& flags = engine.dirtyFlags;
  auto& pool = engine.voicePool;
  auto& fxChain = engine.fxChain;

  auto const bpm = engine.tempo.bpm;
  auto const sampleRate = engine.sampleRate;
  auto const invSampleRate = engine.invSampleRate;

  if (!flags.any())
    return;

  // ==== Oscillators ====
  if (flags.isSet(UpdateGroup::OscEnable))
    updateOscMixGain(pool);

  // ==== Envelopes ====
  if (flags.isSet(UpdateGroup::EnvTime))
    updateAllEnvIncrements(pool, sampleRate);

  if (flags.isSet(UpdateGroup::EnvCurve))
    updateAllEnvCurves(pool);

  // ==== Filters ====
  if (flags.isSet(UpdateGroup::SVFCoeff))
    updateSVFCoeff(pool, invSampleRate);

  if (flags.isSet(UpdateGroup::LadderCoeff))
    updateLadderCoeff(pool, invSampleRate);

  // ==== Saturator ====
  if (flags.isSet(UpdateGroup::SaturatorDerived))
    updateSaturatorDerived(pool);

  // ==== Mono/Portamento ====
  if (flags.isSet(UpdateGroup::MonoEnable))
    updateMonoEnabled(pool);

  if (flags.isSet(UpdateGroup::PortaCoeff))
    updatePortaCoeff(engine.voicePool, sampleRate);

  // ==== Unison ====
  if (flags.isSet(UpdateGroup::UnisonDerived))
    updateUnisonDerived(pool);

  if (flags.isSet(UpdateGroup::UnisonSpread))
    updateUnisonSpread(pool);

  // ==== LFOs ====
  if (flags.isSet(UpdateGroup::LFORate) || flags.isSet(UpdateGroup::LFOTempoSync))
    updateAllLFOEffectiveRates(engine.voicePool, bpm);

  // ==== Tempo ====
  // BPMSync runs after LFO flags because if both are set during a preset
  // load, updateBPMSync correctly overwrites synced LFO rates with BPM-relative values
  if (flags.isSet(UpdateGroup::BPMSync))
    updateBPMSync(pool, fxChain, bpm, sampleRate);

  // ==== FX ====
  if (flags.isSet(UpdateGroup::ChorusDerived))
    updateChorusDerived(fxChain, sampleRate);

  if (flags.isSet(UpdateGroup::DistortionDerived))
    updateDistortionDerived(fxChain);

  if (flags.isSet(UpdateGroup::PhaserDerived))
    updatePhaserDerived(fxChain, invSampleRate);

  if (flags.isSet(UpdateGroup::DelayDerived))
    updateDelayDerived(fxChain, bpm, sampleRate);

  if (flags.isSet(UpdateGroup::DelayDamping))
    updateDelayDamping(fxChain);

  engine.dirtyFlags.clearAll();
}
} // namespace synth::param::sync
