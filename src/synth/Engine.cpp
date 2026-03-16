#include "Engine.h"

#include "synth/ParamBindings.h"
#include "synth/ParamDefs.h"
#include "synth/Preset.h"
#include "synth/PresetApply.h"
#include "synth/Tempo.h"
#include "synth/VoicePool.h"
#include "synth_io/Events.h"

#include "dsp/Buffers.h"
#include "dsp/Math.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>

namespace synth {
using synth_io::MIDIEvent;
using synth_io::ParamEvent;

namespace {
// Handle updates to params with derived values
void onParamUpdate(Engine& engine, param::ParamID id) {
  using param::UpdateGroup;
  namespace pb = param::bindings;
  namespace env = envelope;

  auto& pool = engine.voicePool;

  switch (getParamDef(id).updateGroup) {
  case UpdateGroup::OscEnable: {
    int count = pool.osc1.enabled + pool.osc2.enabled + pool.osc3.enabled + pool.osc4.enabled +
                pool.noise.enabled;
    pool.oscMixGain = (count > 0) ? 1.0f / static_cast<float>(count) : 1.0f;
    break;
  }

  case UpdateGroup::EnvTime:
  case UpdateGroup::EnvCurve: {
    const auto& ampIds = pb::ENV_PARAM_IDS[0];
    const auto& filterIds = pb::ENV_PARAM_IDS[1];

    env::Envelope* env;

    if (id >= ampIds.attack && id <= ampIds.releaseCurve)
      env = &pool.ampEnv;
    else if (id >= filterIds.attack && id <= filterIds.releaseCurve)
      env = &pool.filterEnv;
    else
      env = &pool.modEnv;

    if (getParamDef(id).updateGroup == UpdateGroup::EnvTime)
      env::updateIncrements(*env, engine.sampleRate);
    else
      env::updateCurveTables(*env);
    break;
  }

  case UpdateGroup::SVFCoeff:
    filters::updateSVFCoefficients(pool.svf, engine.invSampleRate);
    break;

  case UpdateGroup::LadderCoeff:
    filters::updateLadderCoefficient(pool.ladder, engine.invSampleRate);
    break;

  case UpdateGroup::SaturatorDerived:
    pool.saturator.invDrive = saturator::calcInvDrive(pool.saturator.drive);
    break;

  case UpdateGroup::MonoEnable:
    if (pool.mono.enabled) {
      for (uint32_t i = 0; i < pool.activeCount; i++) {
        uint32_t v = pool.activeIndices[i];
        env::triggerRelease(pool.ampEnv, v);
        env::triggerRelease(pool.filterEnv, v);
        env::triggerRelease(pool.modEnv, v);
      }
      pool.mono.voiceIndex = MAX_VOICES;
      pool.mono.stackDepth = 0;
    } else {
      voices::releaseMonoVoice(pool);
    }
    break;

  case UpdateGroup::PortaCoeff:
    pool.porta.coeff = dsp::math::calcPortamento(pool.porta.time, engine.sampleRate);
    break;

  case UpdateGroup::UnisonDerived:
    if (pool.unison.enabled) {
      unison::updateDetuneOffsets(pool.unison);
      unison::updatePanPositions(pool.unison);
      unison::updateGainComp(pool.unison);
    }
    break;

  case UpdateGroup::UnisonSpread:
    unison::updatePanPositions(pool.unison);
    break;

  case UpdateGroup::BPMSync: {
    auto& delay = engine.fxChain.delay;
    float bpm = engine.tempo.bpm;

    // Recalc each LFO only if it's synced
    for (lfo::LFO* lfo : {&pool.lfo1, &pool.lfo2, &pool.lfo3}) {
      if (lfo->tempoSync)
        lfo->effectiveRate = tempo::calcEffectiveRate(lfo->subdivision, bpm);
    }
    // Recalc delay only if it's synced
    if (delay.tempoSync)
      delay.delaySamples = static_cast<uint32_t>(
          tempo::subdivisionPeriodSeconds(delay.subdivision, bpm) * engine.sampleRate);
    break;
  }

  case UpdateGroup::LFOTempoSync: {
    lfo::LFO& lfo = (id == param::LFO1_TEMPO_SYNC)   ? pool.lfo1
                    : (id == param::LFO2_TEMPO_SYNC) ? pool.lfo2
                                                     : pool.lfo3;
    lfo.effectiveRate =
        lfo.tempoSync ? tempo::calcEffectiveRate(lfo.subdivision, engine.tempo.bpm) : lfo.rate;
    break;
  }

  case UpdateGroup::LFORate: {
    // Only updates effectiveRate when !tempoSync — synced LFOs ignore lfo.rate entirely
    lfo::LFO& lfo = (id == param::LFO1_RATE)   ? pool.lfo1
                    : (id == param::LFO2_RATE) ? pool.lfo2
                                               : pool.lfo3;
    if (!lfo.tempoSync)
      lfo.effectiveRate = lfo.rate;
    break;
  }

  case UpdateGroup::DistortionDerived: {
    using dsp::fx::distortion::calcDistortionInvNorm;
    engine.fxChain.distortion.invNorm = calcDistortionInvNorm(engine.fxChain.distortion.drive);
    break;
  }

  case UpdateGroup::DelayTime: {
    auto& d = engine.fxChain.delay;
    d.delaySamples = d.tempoSync
                         ? static_cast<uint32_t>(
                               tempo::subdivisionPeriodSeconds(d.subdivision, engine.tempo.bpm) *
                               engine.sampleRate)
                         : static_cast<uint32_t>(d.time * engine.sampleRate);
    break;
  }

  case UpdateGroup::None:
    break;
  }
}

} // anonymous namespace

Engine createEngine(const EngineConfig& config) {
  namespace pb = param::bindings;

  Engine engine{};

  engine.numFrames = config.numFrames;

  engine.sampleRate = config.sampleRate;
  engine.invSampleRate = 1.0f / config.sampleRate;

  dsp::buffers::initStereoBuffer(engine.poolBuffer, config.numFrames);

  voices::initVoicePool(engine.voicePool);

  pb::initParamRouter(engine.paramRouter, engine.voicePool, engine.tempo);
  pb::initFXParamBindings(engine.paramRouter, engine.fxChain);

  auto initPreset = preset::createInitPreset();
  preset::applyPreset(initPreset, engine);

  return engine;
}

void Engine::processParamEvent(const ParamEvent& event) {
  using param::bindings::setParamValue;

  auto id = static_cast<param::ParamID>(event.id);

  setParamValue(paramRouter, id, event.value);
  onParamUpdate(*this, id);
}

void Engine::processMIDIEvent(const synth_io::MIDIEvent& event) {
  using Type = MIDIEvent::Type;

  switch (event.type) {
  case Type::NoteOn:
    if (event.data.noteOn.velocity > 0)
      voices::handleNoteOn(voicePool,
                           event.data.noteOn.note,
                           event.data.noteOn.velocity,
                           ++noteCount,
                           sampleRate);
    else
      voices::releaseVoice(voicePool, event.data.noteOn.note, sampleRate);
    break;

  case Type::NoteOff:
    voices::handleNoteOff(voicePool, event.data.noteOff.note, sampleRate);
    break;

  case Type::ControlChange: {
    using param::bindings::handleMIDICC;

    ParamID id =
        handleMIDICC(paramRouter, voicePool, event.data.cc.number, event.data.cc.value, sampleRate);

    if (id != ParamID::UNKNOWN)
      onParamUpdate(*this, id);
    break;
  }

  case Type::PitchBend:
    // Normalize value [-8192, 8191] -> [-1.0, 1.0]
    voicePool.pitchBend.value = event.data.pitchBend.value / 8192.0f;
    break;

  // TODO(nico)...at some point
  case Type::Aftertouch:
    break;
  case Type::ChannelPressure:
    break;
  case Type::ProgramChange:
    break;

  default:
    break;
  }
}

void Engine::processAudioBlock(float** outputBuffer, size_t numChannels, size_t numFrames) {
  /* NOTE(nico): Use internal Engine block size to allow processing of
   * expensive calculation that need to occur more often than once per audio
   * buffer block but NOT on every sample either.  E.g. Modulation
   *
   * TODO(nico): mess with ENGINE_BLOCK_SIZE value (currently 64) to see
   * how it effects things
   */
  uint32_t offset = 0;
  while (offset < numFrames) {
    uint32_t blockSize = std::min(ENGINE_BLOCK_SIZE, static_cast<uint32_t>(numFrames) - offset);
    auto bufferSlice = dsp::buffers::createStereoBufferSlice(poolBuffer, offset);

    voices::processVoices(voicePool, bufferSlice, blockSize, invSampleRate);
    offset += blockSize;
  }

  for (size_t frame = 0; frame < numFrames; frame++) {
    if (numChannels == 0)
      continue;

    // Mono
    if (numChannels == 1)
      outputBuffer[0][frame] = (poolBuffer.left[frame] + poolBuffer.right[frame]) * 0.5f;

    // TODO(nico): handle for more than just stereo channels
    if (numChannels >= 2) {
      outputBuffer[0][frame] = poolBuffer.left[frame];
      outputBuffer[1][frame] = poolBuffer.right[frame];
    }
  }
}

} // namespace synth
