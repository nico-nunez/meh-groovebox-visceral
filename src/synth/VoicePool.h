#pragma once

#include "Envelope.h"
#include "Filters.h"
#include "ModMatrix.h"
#include "Noise.h"
#include "Types.h"
#include "WavetableOsc.h"
#include "synth/LFO.h"
#include "synth/ParamRanges.h"
#include "synth/Saturator.h"
#include "synth/SignalChain.h"

#include <cstddef>
#include <cstdint>

namespace synth::voices {
using envelope::Envelope;

using lfo::LFO;
using lfo::LFOModState;

using wavetable::osc::WavetableOsc;
using wavetable::osc::WavetableOscConfig;
using wavetable::osc::WavetableOscModState;

using noise::Noise;
using noise::NoiseConfig;

using filters::LadderFilter;
using filters::SVFilter;

using saturator::Saturator;

using signal_chain::SignalChain;
using signal_chain::SignalProcessor;

using mod_matrix::ModMatrix;

struct PitchBend {
  float value = 0.0f; // [-1.0, 1.0]
  float range = param::ranges::pitch::BEND_RANGE_DEFAULT;
};

struct VoicePoolConfig {
  WavetableOscConfig osc1{};
  WavetableOscConfig osc2{};
  WavetableOscConfig osc3{};
  WavetableOscConfig osc4{};

  NoiseConfig noise{};

  float pitchBendRange = 2.0f;

  float masterGain = 1.0f;
  float sampleRate = 48000.0f;
};

// VoicePool - top-level container (universal synth)
struct VoicePool {
  // ==== Oscillators (4 oscillators) ====
  WavetableOsc osc1;
  WavetableOsc osc2;
  WavetableOsc osc3;
  WavetableOsc osc4;

  Noise noise;

  LFO lfo1;
  LFO lfo2;
  LFO lfo3;

  LFOModState lfoModState;

  // Reduce gain for multiple oscillators
  // TODO(nico): this needs to be tide to number of active oscs
  float oscMixGain = 1.0f / 4.0;

  ModMatrix modMatrix;

  Envelope ampEnv;    // Amplitude envelope
  Envelope filterEnv; // Filter modulation
  Envelope modEnv;    // General-purpose modulation

  SVFilter svf;
  LadderFilter ladder;

  Saturator saturator;

  SignalChain signalChain;

  PitchBend pitchBend;

  // TODO(nico) add/handle sustained notes
  bool sustainHeld = false;

  float masterGain = 1.0f; // range [0.0 - 2.0]
                           // range [-inf - +6DB]

  // ==== Voice metadata ====
  uint8_t midiNotes[MAX_VOICES];    // Which MIDI note (0-127)
  float velocities[MAX_VOICES];     // Note-on velocity (0.0-1.0)
  uint32_t noteOnTimes[MAX_VOICES]; // NoteOn counter ( 1 is older than 2)
  uint8_t isActive[MAX_VOICES];     // 1 = active, 0 = free

  // FM Modulation
  WavetableOscModState oscModState;

  float sampleRate;
  float invSampleRate;

  // ==== Active voice tracking ====
  uint32_t activeCount = 0;
  uint32_t activeIndices[MAX_VOICES]; // Dense array of active indices
};

// ===========================
// Voice Pool Management
// ===========================
// Initialize VoicePool (once upon engin creation)
void initVoicePool(VoicePool& pool, const VoicePoolConfig& config);

// updating existing Engine member
void updateVoicePoolConfig(VoicePool& pool, const VoicePoolConfig& config);

// ===========================
// MIDI Event Handlers
// ===========================
void handleNoteOn(VoicePool& pool,
                  uint8_t midiNote,
                  uint8_t velocity,
                  uint32_t noteOnTime,
                  float sampleRate);

void handleNoteOff(VoicePool& pool, uint8_t midiNote);

// ====================================
// Voice Alloaction & Initialization
// ====================================
// Find free or oldest voice index for voice Initialization
uint32_t allocateVoiceIndex(VoicePool& pool);

// Initial voice state for noteOn event
void initializeVoice(VoicePool& pool,
                     uint32_t index,
                     uint8_t midiNote,
                     uint8_t velocity,
                     uint32_t noteOnTime,
                     float sampleRate);

// Trigger envelope release for voice playing midiNote
void releaseVoice(VoicePool& pool, uint8_t midiNote);

// Add newly active voice (noteOn)
void addActiveIndex(VoicePool& pool, uint32_t voiceIndex);

// Remove an inactive voice (noteOff)
void removeInactiveIndex(VoicePool& pool, uint32_t voiceIndex);

void processVoices(VoicePool& pool, float* output, size_t numSamples);

} // namespace synth::voices
