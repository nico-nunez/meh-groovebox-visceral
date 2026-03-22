#include "ParamSync.h"

#include "dsp/FX/Reverb.h"
#include "synth/Engine.h"
#include "synth/Envelope.h"
#include "synth/FXChain.h"
#include "synth/Filters.h"
#include "synth/ParamDefs.h"
#include "synth/Saturator.h"
#include "synth/Unison.h"
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
namespace reverb = dsp::fx::reverb;

void updateOscMixGain(VoicePool& pool) {
  int count = pool.osc1.enabled + pool.osc2.enabled + pool.osc3.enabled + pool.osc4.enabled +
              pool.noise.enabled;
  pool.oscMixGain = (count > 0) ? 1.0f / static_cast<float>(count) : 1.0f;
}

void updateOscFixedPhaseIncs(VoicePool& pool, float invSampleRate) {
  pool.osc1.fixedPhaseInc = pool.osc1.fixedFreq * invSampleRate;
  pool.osc2.fixedPhaseInc = pool.osc2.fixedFreq * invSampleRate;
  pool.osc3.fixedPhaseInc = pool.osc3.fixedFreq * invSampleRate;
  pool.osc4.fixedPhaseInc = pool.osc4.fixedFreq * invSampleRate;
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

void updateSVFCoeff(filters::SVFilter& svf, const float invSampleRate) {
  filters::updateSVFCoefficients(svf, invSampleRate);
}

void updateLadderCoeff(filters::LadderFilter& ladder, const float invSampleRate) {
  filters::updateLadderCoefficient(ladder, invSampleRate);
}

void updateSaturatorDerived(saturator::Saturator& sat) {
  sat.invDrive = saturator::calcInvDrive(sat.drive);
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

void updatePortaCoeff(voices::Portamento& porta, const float sampleRate) {
  porta.coeff = dsp::math::calcPortamento(porta.time, sampleRate);
}

void updateUnisonDerived(unison::UnisonState& uni) {
  if (uni.enabled) {
    unison::updateDetuneOffsets(uni);
    unison::updatePanPositions(uni);
    unison::updateGainComp(uni);
  }
}

void updateUnisonSpread(unison::UnisonState& uni) {
  unison::updatePanPositions(uni);
}

void updateLFOEffectiveRates(VoicePool& pool, const float bpm) {
  for (lfo::LFO* lfo : {&pool.lfo1, &pool.lfo2, &pool.lfo3}) {
    lfo->effectiveRate =
        lfo->tempoSync ? tempo::calcEffectiveRate(lfo->subdivision, bpm) : lfo->rate;
  }
}

void updateLFOFadeCounts(VoicePool& pool, float sampleRate) {
  for (lfo::LFO* lfo : {&pool.lfo1, &pool.lfo2, &pool.lfo3}) {
    lfo->delayCount = lfo->delayMs * sampleRate / 1000.0f;
    lfo->attackCount = lfo->attackMs * sampleRate / 1000.0f;
  }
}

void updateChorusDerived(chorus::ChorusFX& chorus, const float sampleRate) {
  chorus::recalcChorusDerivedVals(chorus, sampleRate);
}

void updateDistortionDerived(dist::DistortionFX& dist) {
  dist.invNorm = dist::calcDistortionInvNorm(dist.drive);
}

void updatePhaserDerived(phaser::PhaserFX& phaser, const float invSampleRate) {
  phaser::recalcPhaseDerivedVals(phaser, invSampleRate);
}

void updateDelayDerived(delay::DelayFX& delay, const float bpm, const float sampleRate) {
  delay::recalcTargetDelaySamples(delay, bpm, sampleRate);
}

void updateDelayDamping(delay::DelayFX& delay) {
  delay::recalcDerivedDampCoeff(delay);
}

void updateReverdDerived(reverb::ReverbFX& reverb, float sampleRate) {
  reverb::recalcReverbDerivedVals(reverb, sampleRate);
}

void updateBPMSync(VoicePool& pool, FXChain& fxChain, const float bpm, const float sampleRate) {
  updateLFOEffectiveRates(pool, bpm);

  if (fxChain.delay.tempoSync)
    updateDelayDerived(fxChain.delay, bpm, sampleRate);
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
  if (flags.isSet(UpdateGroup::OscFreqFixed))
    updateOscFixedPhaseIncs(pool, invSampleRate);

  // ==== Envelopes ====
  if (flags.isSet(UpdateGroup::EnvTime))
    updateAllEnvIncrements(pool, sampleRate);

  if (flags.isSet(UpdateGroup::EnvCurve))
    updateAllEnvCurves(pool);

  // ==== Filters ====
  if (flags.isSet(UpdateGroup::SVFCoeff))
    updateSVFCoeff(pool.svf, invSampleRate);

  if (flags.isSet(UpdateGroup::LadderCoeff))
    updateLadderCoeff(pool.ladder, invSampleRate);

  // ==== Saturator ====
  if (flags.isSet(UpdateGroup::SaturatorDerived))
    updateSaturatorDerived(pool.saturator);

  // ==== Mono/Portamento ====
  if (flags.isSet(UpdateGroup::MonoEnable))
    updateMonoEnabled(pool);

  if (flags.isSet(UpdateGroup::PortaCoeff))
    updatePortaCoeff(pool.porta, sampleRate);

  // ==== Unison ====
  if (flags.isSet(UpdateGroup::UnisonDerived))
    updateUnisonDerived(pool.unison);

  if (flags.isSet(UpdateGroup::UnisonSpread))
    updateUnisonSpread(pool.unison);

  // ==== LFOs ====
  if (flags.isSet(UpdateGroup::LFORate) || flags.isSet(UpdateGroup::LFOTempoSync))
    updateLFOEffectiveRates(engine.voicePool, bpm);

  if (flags.isSet(UpdateGroup::LFOFadeIn))
    updateLFOFadeCounts(pool, sampleRate);

  // ==== Tempo ====
  // BPMSync runs after LFO flags because if both are set during a preset
  // load, updateBPMSync correctly overwrites synced LFO rates with BPM-relative values
  if (flags.isSet(UpdateGroup::BPMSync))
    updateBPMSync(pool, fxChain, bpm, sampleRate);

  // ==== FX ====
  if (flags.isSet(UpdateGroup::ChorusDerived))
    updateChorusDerived(fxChain.chorus, sampleRate);

  if (flags.isSet(UpdateGroup::DistortionDerived))
    updateDistortionDerived(fxChain.distortion);

  if (flags.isSet(UpdateGroup::PhaserDerived))
    updatePhaserDerived(fxChain.phaser, invSampleRate);

  if (flags.isSet(UpdateGroup::DelayDerived))
    updateDelayDerived(fxChain.delay, bpm, sampleRate);

  if (flags.isSet(UpdateGroup::DelayDamping))
    updateDelayDamping(fxChain.delay);

  if (flags.isSet(UpdateGroup::ReverbDerived))
    updateReverdDerived(fxChain.reverb, sampleRate);

  engine.dirtyFlags.clearAll();
}
} // namespace synth::param::sync
