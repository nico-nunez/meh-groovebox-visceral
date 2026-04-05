#pragma once

#include "synth/SignalChain.h"
#include "synth/preset/Preset.h"

#include "dsp/fx/FXChain.h"

#include <cstddef>
#include <cstdint>

namespace synth::events {

// ========================
// MIDI Event Queue
// ========================
struct MIDIEvent {
  enum class Type : uint8_t {
    NoteOn,
    NoteOff,
    ControlChange,
    PitchBend,
    ProgramChange,
    Aftertouch,
    ChannelPressure,
    Unknown
  };

  Type type = Type::Unknown;
  uint8_t channel = 0;
  uint64_t timestamp = 0;

  union {
    struct {
      uint8_t note;
      uint8_t velocity;
    } noteOn;
    struct {
      uint8_t note;
      uint8_t velocity;
    } noteOff;
    struct {
      uint8_t number;
      uint8_t value;
    } cc;
    struct {
      int16_t value;
    } pitchBend;
    struct {
      uint8_t number;
    } programChange;
    struct {
      uint8_t note;
      uint8_t pressure;
    } aftertouch;
    struct {
      uint8_t pressure;
    } channelPressure;
  } data;
};

// ============================
// Param Event Queue (scalar)
// ============================

struct ParamEvent {
  uint8_t id = 0;
  float value = 0.0f; // Normalized [0, 1]
};

// ============================
// Engine Events (non-scalar)
// ============================

struct EngineEvent {
  enum class Type : uint8_t {
    SetNoiseType,

    AddFMRoute,
    RemoveFMRoute,
    ClearFMRoutes,

    AddModRoute,
    RemoveModRoute,
    ClearModRoutes,

    SetSignalChain,
    SetFXChain,

    ApplyPreset,
    Panic,
  };

  Type type{};

  union {
    struct {
      uint8_t noiseType;
    } setNoiseType;

    struct {
      uint8_t carrierIndex; // 0..3
      uint8_t source;       // synth-defined FM source enum as uint8_t
      float depth;
    } addFMRoute;

    struct {
      uint8_t carrierIndex;
      uint8_t source;
    } removeFMRoute;

    struct {
      uint8_t carrierIndex;
    } clearFMRoutes;

    struct {
      uint8_t source;      // synth-defined mod source enum as uint8_t
      uint8_t destination; // synth-defined mod destination enum as uint8_t
      float amount;
    } addModRoute;

    struct {
      uint8_t routeIndex;
    } removeModRoute;

    struct {
      uint8_t reserved;
    } clearModRoutes;

    struct {
      uint8_t processors[signal_chain::MAX_CHAIN_SLOTS];
      uint8_t count;
    } setSignalChain;

    struct {
      uint8_t processors[dsp::fx::chain::MAX_EFFECT_SLOTS];
      uint8_t count;
    } setFXChain;

    struct {
      const preset::Preset* preset;
    } applyPreset;

    struct {
      uint8_t reserved;
    } panic;
  } data;
};

} // namespace synth::events
