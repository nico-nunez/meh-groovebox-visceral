#pragma once

#include "ParamBindings.h"
#include "VoicePool.h"

#include "dsp/Buffers.h"
#include "dsp/Waveforms.h"

#include "synth_io/Events.h"
#include "synth_io/SynthIO.h"

#include <cstdint>

namespace synth {
using synth_io::MIDIEvent;
using synth_io::ParamEvent;

using voices::VoicePool;
using voices::VoicePoolConfig;

using dsp::buffers::StereoBuffer;
using dsp::waveforms::WaveformType;

using param::ParamID;
using param::bindings::ParamRouter;

struct EngineConfig : VoicePoolConfig {
  uint32_t numFrames = synth_io::DEFAULT_FRAMES;
};

struct Engine {
  uint32_t numFrames = synth_io::DEFAULT_FRAMES;

  VoicePool voicePool;

  ParamRouter paramRouter;

  StereoBuffer poolBuffer{};

  uint32_t noteCount = 0;

  void processMIDIEvent(const MIDIEvent& event);
  void processParamEvent(const ParamEvent& event);
  void processAudioBlock(float** outputBuffer, size_t numChannels, size_t numFrames);
};

Engine createEngine(const EngineConfig& config);

} // namespace synth
