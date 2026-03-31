#pragma once

#include "synth/events/EventQueues.h"
#include "synth/events/Events.h"

#include <cstddef>
#include <cstdint>

namespace app::session {
using synth::events::MIDIEvent;
using synth::events::MIDIEventQueue;
using synth::events::ParamEvent;
using synth::events::ParamEventQueue;

struct SynthSession;
using hSynthSession = SynthSession*;

// --- Constants ---
inline constexpr uint32_t DEFAULT_SAMPLE_RATE = 48000;
inline constexpr uint32_t DEFAULT_FRAMES = 512;
inline constexpr uint16_t DEFAULT_CHANNELS = 2;

struct DeviceInfo {
  uint32_t sampleRate = DEFAULT_SAMPLE_RATE;
  uint32_t bufferFrameSize = DEFAULT_FRAMES;
  uint16_t numChannels = DEFAULT_CHANNELS;
};

DeviceInfo queryDefaultDevice();

enum class BufferFormat {
  NonInterleaved, // channels in separate arrays [LLLL] [RRRR]
  Interleaved,    // channels interwoven in single array [LRLRLRLR]
};

struct SessionConfig {
  uint32_t sampleRate = DEFAULT_SAMPLE_RATE;
  uint32_t numFrames = DEFAULT_FRAMES;
  uint16_t numChannels = DEFAULT_CHANNELS;
  BufferFormat bufferFormat = BufferFormat::NonInterleaved;
};

typedef void (*MIDIEventHandler)(MIDIEvent midiEvent, void* userContext);
typedef void (*AudioBufferHandler)(float** outputBuffer,
                                   size_t numChannels,
                                   size_t numFrames,
                                   void* userContext);

typedef void (*ParamEventHandler)(ParamEvent paramEvent, void* userContext);

struct SynthCallbacks {
  MIDIEventHandler processMIDIEvent = nullptr;
  ParamEventHandler processParamEvent = nullptr;
  AudioBufferHandler processAudioBlock = nullptr;
};

// ==== Session Handlers ====
hSynthSession initSession(SessionConfig userConfig,
                          SynthCallbacks userCallbacks,
                          void* userContext = NULL);

int startSession(hSynthSession sessionPtr);
int stopSession(hSynthSession sessionPtr);
int disposeSession(hSynthSession sessionPtr);

// ==== MIDI Event Handler ====
bool pushMIDIEvent(hSynthSession sessionPtr, MIDIEvent event);

// ==== Parameter Event Handlers ====
bool setParam(hSynthSession sessionPtr, uint8_t id, float value);

} // namespace app::session
