#include "Engine.h"

#include "synth/ParamBindings.h"
#include "synth/ParamDefs.h"
#include "synth/ParamSync.h"
#include "synth/Preset.h"
#include "synth/PresetApply.h"
#include "synth/VoicePool.h"

#include "dsp/Buffers.h"
#include "dsp/FX/FXChain.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>

namespace synth {
using synth_io::MIDIEvent;
using synth_io::ParamEvent;

Engine createEngine(const EngineConfig& config) {
  namespace pb = param::bindings;

  Engine engine{};

  engine.numFrames = config.numFrames;

  engine.sampleRate = config.sampleRate;
  engine.invSampleRate = 1.0f / config.sampleRate;

  dsp::buffers::initStereoBuffer(engine.poolBuffer, config.numFrames);

  voices::initVoicePool(engine.voicePool);

  pb::initParamRouter(engine.paramRouter, engine.voicePool, engine.bpm);
  pb::initFXParamBindings(engine.paramRouter, engine.fxChain);

  dsp::fx::chain::initFXChain(engine.fxChain, engine.bpm, engine.sampleRate);

  auto initPreset = preset::createInitPreset();
  preset::applyPreset(initPreset, engine);

  return engine;
}

void Engine::processParamEvent(const ParamEvent& event) {
  using param::bindings::setParamValue;

  auto id = static_cast<param::ParamID>(event.id);

  setParamValue(paramRouter, id, event.value);

  dirtyFlags.mark(param::getParamDef(id).updateGroup);
  param::sync::syncDirtyParams(*this);
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
