#include "dsp/FX/Chorus.h"
#include "dsp/Buffers.h"

#include <cstddef>

namespace dsp::fx::chorus {

void initChorusState(ChorusState& state, float sampleRate) {
  state.bufSize = static_cast<size_t>(30.0f / 1000.0f * sampleRate); // TODO: fix
  dsp::buffers::initStereoBuffer(state.buffer, state.bufSize);
}

void destroyChorusState(ChorusState& state) {
  dsp::buffers::destroyStereoBuffer(state.buffer);
  state.bufSize = 0;
}

void processChorus(ChorusFX&, buffers::StereoBuffer, size_t, float) {
  // TODO....
}

} // namespace dsp::fx::chorus
