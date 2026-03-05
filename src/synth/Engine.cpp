#include "Engine.h"
#include "ParamBindings.h"
#include "VoicePool.h"

#include "synth_io/Events.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>

namespace synth {
using synth_io::MIDIEvent;
using synth_io::ParamEvent;

Engine createEngine(const EngineConfig& config) {
  using param::bindings::initParamRouter;
  using voices::initVoicePool;

  Engine engine{};
  initVoicePool(engine.voicePool, config);
  initParamRouter(engine.paramRouter, engine.voicePool);

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
                           sampleRate);
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
    // TODO
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
    voices::processVoices(voicePool, poolBuffer + offset, blockSize);
    offset += blockSize;
  }

  for (size_t frame = 0; frame < numFrames; frame++) {
    for (size_t ch = 0; ch < numChannels; ch++) {
      outputBuffer[ch][frame] = poolBuffer[frame];
    }
  }
}

} // namespace synth
