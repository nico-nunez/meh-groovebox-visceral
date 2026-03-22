#pragma once

#include "synth/ModMatrix.h"
#include "synth/Tempo.h"
#include "synth/WavetableBanks.h"

namespace synth::lfo {
using mod_matrix::ModDest;
using Subdivision = tempo::Subdivision;
using WavetableBank = wavetable::banks::WavetableBank;

struct LFO {
  // null = S&H mode; set to getBankByID(BankID::Sine) at init
  const WavetableBank* bank = nullptr;
  float phase = 0.0f;     // normalized [0.0, 1.0)
  float rate = 1.0f;      // Hz, no DSP ceiling; UI default range [0.0, 20.0]
  float amplitude = 1.0f; // output range [-amplitude, +amplitude]
  float shHeld = 0.0f;    // S&H: last held random value
  bool retrigger = false; // reset phase on note-on?

  float delayMs = 0.0f;  // silence before LFO onset (0–5000 ms)
  float attackMs = 0.0f; // linear ramp from 0 → amplitude (0–5000 ms)

  // Precomputed from ms + sampleRate (updated via LFOFadeIn group)
  float delayCount = 0.0f;  // samples
  float attackCount = 0.0f; // samples

  // Per-note runtime state (reset on retrigger)
  float delayTimer = 0.0f;  // samples remaining in delay
  float attackTimer = 0.0f; // samples remaining in attack ramp

  // Tempo Sync
  bool tempoSync = false;
  Subdivision subdivision = Subdivision::Quarter;
  float effectiveRate = 1.0f; // precomputed; hot path reads this
};

struct LFOModState {
  float lfo1;
  float lfo2;
  float lfo3;

  float contribs[ModDest::DEST_COUNT];
};

void clearContribs(LFOModState& modState);

float advanceLFO(LFO& lfo, float invSampleRate, float effectiveRate, float effectiveAmplitude);

} // namespace synth::lfo
