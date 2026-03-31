#pragma once

#include "synth/ParamBindings.h"
#include "synth/ParamDefs.h"
#include "synth/VoicePool.h"

#include "dsp/Buffers.h"
#include "dsp/FX/FXChain.h"
#include "dsp/Waveforms.h"

#include "synth_io/SynthIO.h"

#include <cstdint>

namespace synth {
using synth_io::MIDIEvent;
using synth_io::ParamEvent;

using dsp::fx::chain::FXChain;
using voices::VoicePool;

using dsp::buffers::StereoBuffer;
using dsp::waveforms::WaveformType;

using param::ParamID;
using param::UpdateGroup;
using param::UpdateGroupFlags;
using param::bindings::ParamRouter;

struct EngineConfig {
  uint32_t numFrames = synth_io::DEFAULT_FRAMES;
  float sampleRate = synth_io::DEFAULT_SAMPLE_RATE;
};

struct Engine {
  uint32_t numFrames = synth_io::DEFAULT_FRAMES;

  float sampleRate = synth_io::DEFAULT_SAMPLE_RATE;
  float invSampleRate = 1.0f / sampleRate;

  float bpm;

  VoicePool voicePool;

  FXChain fxChain;

  ParamRouter paramRouter;

  StereoBuffer poolBuffer{};

  uint32_t noteCount = 0;

  UpdateGroupFlags dirtyFlags;

  void processMIDIEvent(const MIDIEvent& event);
  void processParamEvent(const ParamEvent& event);
  void processAudioBlock(float** outputBuffer, size_t numChannels, size_t numFrames);
};

Engine createEngine(const EngineConfig& config);

} // namespace synth
