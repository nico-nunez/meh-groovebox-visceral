#include "Engine.h"

#include "synth/SignalChain.h"
#include "synth/VoicePool.h"
#include "synth/WavetableOsc.h"
#include "synth/events/Events.h"
#include "synth/params/ParamDefs.h"
#include "synth/params/ParamSync.h"
#include "synth/preset/Preset.h"
#include "synth/preset/PresetApply.h"

#include "dsp/Buffers.h"
#include "dsp/fx/FXChain.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>

namespace synth {

Engine createEngine(const EngineConfig& config) {

  Engine engine{};

  engine.numFrames = config.numFrames;

  engine.sampleRate = config.sampleRate;
  engine.invSampleRate = 1.0f / config.sampleRate;

  dsp::buffers::initStereoBuffer(engine.poolBuffer, config.numFrames);

  voices::initVoicePool(engine.voicePool);

  param::router::initParamRouter(engine.paramRouter, engine.voicePool, engine.bpm);
  param::router::initFXParamRouter(engine.paramRouter, engine.fxChain);

  dsp::fx::chain::initFXChain(engine.fxChain, engine.bpm, engine.sampleRate);

  auto initPreset = preset::createInitPreset();
  preset::applyPreset(initPreset, engine);

  return engine;
}

void Engine::processParamEvent(const ParamEvent& event) {
  using namespace param;

  auto id = static_cast<ParamID>(event.id);

  router::setParamValue(paramRouter, id, event.value);

  dirtyFlags.mark(param::getParamDef(id).updateGroup);
  sync::syncDirtyParams(*this);
}

void Engine::processMIDIEvent(const MIDIEvent& event) {
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
    using param::router::handleMIDICC;

    ParamID id =
        handleMIDICC(paramRouter, voicePool, event.data.cc.number, event.data.cc.value, sampleRate);

    if (id != ParamID::UNKNOWN) {
      dirtyFlags.mark(param::getParamDef(id).updateGroup);
      param::sync::syncDirtyParams(*this);
    }
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

void Engine::processEngineEvent(const EngineEvent& event) {
  namespace osc = wavetable::osc;
  namespace mm = mod_matrix;
  namespace sc = signal_chain;
  namespace fx = dsp::fx::chain;

  switch (event.type) {

  case EngineEvent::Type::SetNoiseType: {
    voicePool.noise.type = static_cast<noise::NoiseType>(event.data.setNoiseType.noiseType);
    return;
  }

  case EngineEvent::Type::AddFMRoute: {
    auto* carrier =
        voices::getOscByEnum(voicePool, static_cast<osc::FMCarrier>(event.data.addFMRoute.carrier));
    if (!carrier)
      return;
    osc::addFMRoute(*carrier,
                    static_cast<osc::FMSource>(event.data.addFMRoute.source),
                    event.data.addFMRoute.depth);
    return;
  }

  case EngineEvent::Type::RemoveFMRoute: {
    auto* carrier =
        voices::getOscByEnum(voicePool,
                             static_cast<osc::FMCarrier>(event.data.removeFMRoute.carrier));
    if (!carrier)
      return;
    osc::removeFMRoute(*carrier, static_cast<osc::FMSource>(event.data.removeFMRoute.source));
    return;
  }

  case EngineEvent::Type::ClearFMRoutes: {
    auto* carrier =
        voices::getOscByEnum(voicePool,
                             static_cast<osc::FMCarrier>(event.data.clearFMRoutes.carrier));
    if (!carrier)
      return;
    osc::clearFMRoutes(*carrier);
    return;
  }

  case EngineEvent::Type::AddModRoute: {
    mm::addRoute(voicePool.modMatrix,
                 static_cast<mm::ModSrc>(event.data.addModRoute.source),
                 static_cast<mm::ModDest>(event.data.addModRoute.destination),
                 event.data.addModRoute.amount);
    return;
  }

  case EngineEvent::Type::RemoveModRoute: {
    mm::removeRoute(voicePool.modMatrix, event.data.removeModRoute.routeIndex);
    return;
  }

  case EngineEvent::Type::ClearModRoutes: {
    mm::clearRoutes(voicePool.modMatrix);
    return;
  }

  case EngineEvent::Type::SetSignalChain: {
    sc::SignalProcessor procs[sc::MAX_CHAIN_SLOTS];
    uint8_t count = event.data.setSignalChain.count;
    for (uint8_t i = 0; i < count; ++i)
      procs[i] = static_cast<sc::SignalProcessor>(event.data.setSignalChain.processors[i]);
    sc::setSigChain(voicePool.signalChain, procs, count);
    return;
  }

  case EngineEvent::Type::ClearSignalChain: {
    sc::clearSigChain(voicePool.signalChain);
    return;
  }

  case EngineEvent::Type::SetFXChain: {
    fx::FXProcessor procs[fx::MAX_EFFECT_SLOTS];
    uint8_t count = event.data.setFXChain.count;
    for (uint8_t i = 0; i < count; ++i)
      procs[i] = static_cast<fx::FXProcessor>(event.data.setFXChain.processors[i]);
    fx::setFXChain(fxChain, procs, count);
    return;
  }

  case EngineEvent::Type::ClearFXChain: {
    fx::clearFXChain(fxChain);
    return;
  }

  case EngineEvent::Type::ApplyPreset: {
    if (!event.data.applyPreset.preset)
      return;
    preset::applyPreset(*event.data.applyPreset.preset, *this);
    return;
  }

  case EngineEvent::Type::Panic:
    voices::panicVoicePool(voicePool);
    return;
  }
}

/* NOTE: Use internal Engine block size to allow processing of
 * expensive calculation that need to occur more often than once per audio
 * buffer block but NOT on every sample either.  E.g. Modulation
 */
void Engine::processAudioBlock(float** outputBuffer, size_t numChannels, size_t numFrames) {
  uint32_t offset = 0;

  while (offset < numFrames) {
    uint32_t blockSize = std::min(ENGINE_BLOCK_SIZE, static_cast<uint32_t>(numFrames) - offset);
    auto bufferSlice = dsp::buffers::createStereoBufferSlice(poolBuffer, offset);

    voices::processVoices(voicePool, bufferSlice, blockSize, invSampleRate);
    offset += blockSize;
  }

  dsp::fx::chain::processFXChain(fxChain, poolBuffer, numFrames);

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
