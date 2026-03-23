#pragma once

#include "synth/Envelope.h"
#include "synth/Filters.h"
#include "synth/LFO.h"
#include "synth/ModMatrix.h"
#include "synth/MonoMode.h"
#include "synth/Noise.h"
#include "synth/ParamRanges.h"
#include "synth/Saturator.h"
#include "synth/SignalChain.h"
#include "synth/Types.h"
#include "synth/Unison.h"
#include "synth/WavetableOsc.h"

#include "dsp/Buffers.h"

#include <cstddef>
#include <cstdint>

namespace synth::voices {
using envelope::Envelope;

using lfo::LFO;
using lfo::LFOModState;

using wavetable::osc::WavetableOsc;
using wavetable::osc::WavetableOscModState;

using noise::Noise;

using filters::LadderFilter;
using filters::SVFilter;

using saturator::Saturator;

using signal_chain::SignalChain;
using signal_chain::SignalProcessor;

using mod_matrix::ModMatrix;

using mono::MonoState;

using unison::UnisonState;

using dsp::buffers::StereoBuffer;

// Used with osc phase modes
inline constexpr float DE_CLICK_INCREMENT = 1.0f / 32.0f;

struct PitchBend {
  float value = 0.0f; // [-1.0, 1.0]
  float range = param::ranges::pitch::BEND_RANGE_DEFAULT;
};

struct Portamento {
  float time = 50.0f;
  float coeff = 0.0f;
  bool legato = true;
  uint8_t lastNote = 0; // MIDI note

  float offsets[MAX_VOICES]{};

  bool enabled = false;
};

struct Sustain {
  bool held = false;
  bool notes[MAX_VOICES]{};
};

// VoicePool - top-level container (universal synth)
struct VoicePool {
  // ==== Oscillators (4 oscillators) ====
  WavetableOsc osc1;
  WavetableOsc osc2;
  WavetableOsc osc3;
  WavetableOsc osc4;

  float deClickGain[MAX_VOICES]{}; // 0.0 at note-on, ramps to 1.0 over DE_CLICK_SAMPLES

  // TODO(nico): this needs to be tide to number of active oscs
  // Reduce gain for multiple oscillators
  float oscMixGain = 1.0f / 4.0;

  Noise noise;

  // Stereo
  float panL[MAX_VOICES];
  float panR[MAX_VOICES];

  LFO lfo1;
  LFO lfo2;
  LFO lfo3;

  LFOModState lfoModState;

  ModMatrix modMatrix;

  Envelope ampEnv;
  Envelope filterEnv;
  Envelope modEnv;

  SVFilter svf;
  LadderFilter ladder;

  Saturator saturator;

  SignalChain signalChain;

  PitchBend pitchBend;

  Portamento porta;

  Sustain sustain;

  UnisonState unison;

  float modWheelValue = 0.0f; // [0.0, 1.0]

  float masterGain = 1.0f; // range [0.0 - 2.0]
                           // range [-inf - +6DB]

  MonoState mono;

  // ==== Voice metadata ====
  uint8_t midiNotes[MAX_VOICES];    // Which MIDI note (0-127)
  float velocities[MAX_VOICES];     // Note-on velocity (0.0-1.0)
  uint32_t noteOnTimes[MAX_VOICES]; // NoteOn counter ( 1 is older than 2)
  uint8_t isActive[MAX_VOICES];     // 1 = active, 0 = free

  // FM Modulation
  WavetableOscModState oscModState;

  // float invSampleRate;

  // ==== Active voice tracking ====
  uint32_t activeCount = 0;
  uint32_t activeIndices[MAX_VOICES]; // Dense array of active indices
};

// ===========================
// Voice Pool Management
// ===========================
// Initialize VoicePool (once upon engin creation)
void initVoicePool(VoicePool& pool);

// ===========================
// MIDI Event Handlers
// ===========================
void handleNoteOn(VoicePool& pool,
                  uint8_t midiNote,
                  uint8_t velocity,
                  uint32_t noteOnTime,
                  float sampleRate);

void handleNoteOff(VoicePool& pool, uint8_t midiNote, float sampleRate);

// ====================================
// Voice Alloaction & Initialization
// ====================================
// Find free or oldest voice index for voice Initialization
uint32_t allocateVoiceIndex(VoicePool& pool, bool& outStolen);

// Initial voice state for noteOn event
void initVoice(VoicePool& pool,
               uint32_t index,
               uint8_t midiNote,
               uint8_t velocity,
               uint32_t noteOnTime,
               bool retrigger,
               float sampleRate);

// Mono Legato (adjust pitch without reseting phases if legato)
void redirectVoicePitch(VoicePool& pool, uint32_t voiceIndex, uint8_t midiNote, float sampleRate);

void releaseVoice(VoicePool& pool, uint8_t midiNote, float sampleRate);

void panicVoicePool(VoicePool& pool);

// Same as above but for mono voice
void releaseMonoVoice(VoicePool& pool);

// Add newly active voice (noteOn)
void addActiveIndex(VoicePool& pool, uint32_t voiceIndex);

// Remove an inactive voice (noteOff)
void removeInactiveIndex(VoicePool& pool, uint32_t voiceIndex);

void processVoices(VoicePool& pool,
                   StereoBuffer poolBuffer,
                   size_t numSamples,
                   float invSampleRate);

// ===================
// Parsing Helpers
// ===================
inline WavetableOsc* getOscByName(VoicePool& pool, const std::string& name) {
  if (name == "osc1")
    return &pool.osc1;
  if (name == "osc2")
    return &pool.osc2;
  if (name == "osc3")
    return &pool.osc3;
  if (name == "osc4")
    return &pool.osc4;
  return nullptr;
}

} // namespace synth::voices
