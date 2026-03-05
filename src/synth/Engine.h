#pragma once

#include "ParamBindings.h"
#include "VoicePool.h"

#include "dsp/Waveforms.h"

#include "synth_io/Events.h"
#include "synth_io/SynthIO.h"

#include <cstdint>

namespace synth {
using synth_io::MIDIEvent;
using synth_io::ParamEvent;

using voices::VoicePool;
using voices::VoicePoolConfig;

using dsp::waveforms::WaveformType;

using param::bindings::ParamID;
using param::bindings::ParamRouter;

struct EngineConfig : VoicePoolConfig {
  float sampleRate = synth_io::DEFAULT_SAMPLE_RATE;
  uint32_t numFrames = synth_io::DEFAULT_FRAMES;
};

struct Engine {
  static constexpr uint32_t NUM_FRAMES = synth_io::DEFAULT_FRAMES;
  float sampleRate = synth_io::DEFAULT_SAMPLE_RATE;

  VoicePool voicePool;

  ParamRouter paramRouter;

  // TODO(nico): this probably needs to live on heap
  // since the number of frames won't be known at compile time
  float poolBuffer[NUM_FRAMES];

  uint32_t noteCount = 0;

  void processMIDIEvent(const MIDIEvent& event);
  void processParamEvent(const ParamEvent& event);
  void processAudioBlock(float** outputBuffer, size_t numChannels, size_t numFrames);
};

Engine createEngine(const EngineConfig& config);

} // namespace synth
