#pragma once

#include "synth/Types.h"

#include <cstdint>
#include <sstream>

namespace synth::mod_matrix {
// ==== Modulation Sources ====
enum ModSrc {
  NoSrc = 0,

  // Envelopes — all three, whether or not they're "doing anything" currently
  AmpEnv,    // 0.0–1.0, always running per voice
  FilterEnv, // 0.0–1.0, always running per voice
  ModEnv,    // 0.0–1.0, general-purpose, not hardwired to anything

  LFO1,
  LFO2,
  LFO3,

  // Per-note values — available from the moment a voice is allocated
  Velocity, // 0.0–1.0 from MIDI note-on velocity
  Noise,

  ModWheel,
  KeyTrack, // MIDI Note offset from A4 (69 - 440hz)

  SRC_COUNT // used to size arrays, not a valid source
};

// ==== Modulation Destinations ====
enum ModDest {
  NoDest = 0,

  // Filter cutoff — units are octaves (bipolar ±4)
  SVFCutoff,
  LadderCutoff,

  // Filter resonance — units are linear ±1.0
  SVFResonance,
  LadderResonance,

  // Oscillator pitch — units are semitones (bipolar ±24)
  Osc1Pitch,
  Osc2Pitch,
  Osc3Pitch,
  Osc4Pitch,

  // Oscillator mix level — units are linear ±1.0
  Osc1Mix,
  Osc2Mix,
  Osc3Mix,
  Osc4Mix,

  // Wavetable scan position — 0.0–1.0 offset
  Osc1ScanPos,
  Osc2ScanPos,
  Osc3ScanPos,
  Osc4ScanPos,

  // FM depth modulation — DX7-style "envelope controls FM intensity"
  Osc1FMDepth,
  Osc2FMDepth,
  Osc3FMDepth,
  Osc4FMDepth,

  // LFO Rate (Hz)
  LFO1Rate,
  LFO2Rate,
  LFO3Rate,

  // LFO Amplitude [-amplitude, +amplitude]
  LFO1Amplitude,
  LFO2Amplitude,
  LFO3Amplitude,

  DEST_COUNT
};

// ==== Modulation Routing ====
constexpr int MAX_MOD_ROUTES = 16;

using ModDest2D = float[ModDest::DEST_COUNT][MAX_VOICES];

struct ModRoute {
  ModSrc src = ModSrc::NoSrc;
  ModDest dest = ModDest::NoDest;
  float amount = 0.0f;
};

struct ModMatrix {
  ModRoute routes[MAX_MOD_ROUTES];
  uint8_t count = 0;

  // engine block-rate output of pre-pass
  ModDest2D destValues = {};

  // interpolation state, persists between engine blocks
  ModDest2D prevDestValues = {};

  // Stack-local inside processVoices
  // Need to zero out each loop if I keep it here
  ModDest2D destStepValues = {};
};

bool addRoute(ModMatrix& matrix, ModSrc src, ModDest dest, float amount);
bool addRoute(ModMatrix& matrix, const ModRoute& route);
bool removeRoute(ModMatrix& matrix, uint8_t index);
bool clearRoutes(ModMatrix& matrix);

void clearPrevModDests(ModMatrix& matrix);
void clearModDestSteps(ModMatrix& matrix);
void setModDestStep(ModMatrix& matrix, ModDest dest, uint32_t voiceIndex, float invNumSamples);

// ==== Parsing Helpers ====
struct ModSrcMapping {
  const char* name;
  const ModSrc src;
};

inline constexpr ModSrcMapping modSrcMappings[ModSrc::SRC_COUNT - 1] = // Not including "NoSrc"
    {
        // Envelopes
        {"ampEnv", ModSrc::AmpEnv},
        {"filterEnv", ModSrc::FilterEnv},
        {"modEnv", ModSrc::ModEnv},

        // LFOs
        {"lfo1", ModSrc::LFO1},
        {"lfo2", ModSrc::LFO2},
        {"lfo3", ModSrc::LFO3},

        // Per-note
        {"velocity", ModSrc::Velocity},
        {"noise", ModSrc::Noise},

        {"modWheel", ModSrc::ModWheel},
        {"keyTrack", ModSrc::KeyTrack},
};

struct ModDestMapping {
  const char* name;
  const ModDest dest;
};
inline constexpr ModDestMapping modDestMappings[ModDest::DEST_COUNT - 1] = // Not including "NoDest"
    {
        // Filters
        {"svf.cutoff", ModDest::SVFCutoff},
        {"ladder.cutoff", ModDest::LadderCutoff},

        {"svf.resonance", ModDest::SVFResonance},
        {"ladder.resonance", ModDest::LadderResonance},

        // Oscillators
        {"osc1.pitch", ModDest::Osc1Pitch},
        {"osc2.pitch", ModDest::Osc2Pitch},
        {"osc3.pitch", ModDest::Osc3Pitch},
        {"osc4.pitch", ModDest::Osc4Pitch},

        {"osc1.mixLevel", ModDest::Osc1Mix},
        {"osc2.mixLevel", ModDest::Osc2Mix},
        {"osc3.mixLevel", ModDest::Osc3Mix},
        {"osc4.mixLevel", ModDest::Osc4Mix},

        {"osc1.scanPos", ModDest::Osc1ScanPos},
        {"osc2.scanPos", ModDest::Osc2ScanPos},
        {"osc3.scanPos", ModDest::Osc3ScanPos},
        {"osc4.scanPos", ModDest::Osc4ScanPos},

        {"osc1.fmDepth", ModDest::Osc1FMDepth},
        {"osc2.fmDepth", ModDest::Osc2FMDepth},
        {"osc3.fmDepth", ModDest::Osc3FMDepth},
        {"osc4.fmDepth", ModDest::Osc4FMDepth},

        // LFOs
        {"lfo1.rate", ModDest::LFO1Rate},
        {"lfo2.rate", ModDest::LFO2Rate},
        {"lfo3.rate", ModDest::LFO3Rate},

        {"lfo1.amplitude", ModDest::LFO1Amplitude},
        {"lfo2.amplitude", ModDest::LFO2Amplitude},
        {"lfo3.amplitude", ModDest::LFO3Amplitude},
};

// ============================================================
// Parsing Helpers
// ============================================================

void parseModCommand(std::istringstream& iss, ModMatrix& modMatrix);

inline mod_matrix::ModSrc parseModSrc(const char* name) {
  for (const auto& m : mod_matrix::modSrcMappings)
    if (std::strcmp(m.name, name) == 0)
      return m.src;
  return mod_matrix::ModSrc::NoSrc;
}

inline const char* modSrcToString(mod_matrix::ModSrc src) {
  for (const auto& m : mod_matrix::modSrcMappings)
    if (m.src == src)
      return m.name;
  return "unknown";
}

inline mod_matrix::ModDest parseModDest(const char* name) {
  for (const auto& m : mod_matrix::modDestMappings)
    if (std::strcmp(m.name, name) == 0)
      return m.dest;
  return mod_matrix::ModDest::NoDest;
}

inline const char* modDestToString(mod_matrix::ModDest dest) {
  for (const auto& m : mod_matrix::modDestMappings)
    if (m.dest == dest)
      return m.name;
  return "unknown";
}

inline void printRoutes(const ModMatrix& matrix) {
  if (matrix.count == 0) {
    printf("no mod routes\n");
    return;
  }
  for (uint8_t i = 0; i < matrix.count; i++)
    printf("  [%d] %s -> %s  amount=%.3f\n",
           i,
           modSrcToString(matrix.routes[i].src),
           modDestToString(matrix.routes[i].dest),
           matrix.routes[i].amount);
}

} // namespace synth::mod_matrix
