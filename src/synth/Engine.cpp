#include "Engine.h"
#include "ParamBindings.h"
#include "VoicePool.h"

#include "synth_io/Events.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>

namespace synth {
using ParamEvent = synth_io::ParamEvent;

Engine createEngine(const EngineConfig& config) {
  Engine engine{};

  voices::initVoicePool(engine.voicePool, config);

  param::bindings::initParamBindings(engine);

  return engine;
}

void Engine::processParamEvent(const ParamEvent& event) {
  param::bindings::setParamValueByID(*this, static_cast<ParamID>(event.id), event.value);
}

void Engine::processNoteEvent(const synth_io::NoteEvent& event) {
  if (!event.midiNote)
    return;

  if (event.type == synth_io::NoteEventType::NoteOff) {
    voices::releaseVoice(voicePool, event.midiNote);
  } else {
    voices::handleNoteOn(voicePool, event.midiNote, event.velocity, noteCount++, sampleRate);
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
