#include "Engine.h"
#include "ParamBindings.h"
#include "Preset.h"
#include "PresetApply.h"
#include "VoicePool.h"

#include "dsp/Buffers.h"
#include "synth_io/Events.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>

namespace synth {
using synth_io::MIDIEvent;
using synth_io::ParamEvent;

Engine createEngine(const EngineConfig& config) {
  Engine engine{};
  engine.numFrames = config.numFrames;

  dsp::buffers::initStereoBuffer(engine.poolBuffer, config.numFrames);

  voices::initVoicePool(engine.voicePool, config.sampleRate);

  param::bindings::initParamRouter(engine.paramRouter, engine.voicePool);

  auto initPreset = preset::createInitPreset();
  preset::applyPreset(initPreset, engine);

  return engine;
}

void Engine::processParamEvent(const ParamEvent& event) {
  using param::bindings::setParamValueByID;
  setParamValueByID(paramRouter, voicePool, static_cast<ParamID>(event.id), event.value);
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
                           voicePool.sampleRate);
    else
      voices::releaseVoice(voicePool, event.data.noteOn.note);
    break;

  case Type::NoteOff:
    voices::handleNoteOff(voicePool, event.data.noteOff.note);
    break;

  case Type::ControlChange:
    param::bindings::handleMIDICC(paramRouter,
                                  voicePool,
                                  event.data.cc.number,
                                  event.data.cc.value);
    break;

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

    voices::processVoices(voicePool, bufferSlice, blockSize);
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
