#include "WavetableOsc.h"

#include "utils/Utils.h"

#include "dsp/Math.h"

#include <cmath>
#include <cstdint>
#include <cstdio>

namespace synth::wavetable::osc {
namespace dsp_wt = dsp::wavetable;

// ================================
// Initialization
// ================================
void initOsc(WavetableOsc& osc, uint32_t voiceIndex, uint8_t midiNote, float sampleRate) {
  float offsetExp = static_cast<float>(osc.octaveOffset) + (osc.detuneAmount / 1200.0f);
  float freq = utils::midiToFrequency(midiNote) * std::exp2f(offsetExp);

  osc.phaseIncrements[voiceIndex] = freq / sampleRate;

  switch (osc.phaseMode) {
  case PhaseMode::Reset:
  case PhaseMode::Spread: // spread only differs for unison sub-voices; main voice = resetPhase
  case PhaseMode::Unknown:
    osc.phases[voiceIndex] = osc.resetPhase; // default
    break;

  case PhaseMode::Random:
    // randNoiseValue() returns [-1, 1]; scale to [0, 1) then apply range
    osc.phases[voiceIndex] = (dsp::math::randNoiseValue() + 1.0f) * 0.5f * osc.randomRange;
    break;

  case PhaseMode::Free:
    // Don't reset — phase continues from wherever it is.
    // Note: osc.phases[voiceIndex] is not read by processOscsUnison when unison is active.
    // Sub-voice phases are managed separately in UnisonState.subPhases.
    // This call is a no-op for the active-unison path but kept in case unison is disabled mid-session.
    break;
  }
}

// Mono (no retrigger/legato)
void updateOscPitch(WavetableOsc& osc, uint32_t voiceIndex, uint8_t midiNote, float sampleRate) {
  float offsetExp = static_cast<float>(osc.octaveOffset) + (osc.detuneAmount / 1200.0f);
  float freq = utils::midiToFrequency(midiNote) * std::exp2f(offsetExp);

  osc.phaseIncrements[voiceIndex] = freq / sampleRate;
}

// =========================
// FM Modulation
// =========================
void resetOscModState(WavetableOscModState& modState, uint32_t voiceIndex) {
  modState.osc1[voiceIndex] = 0.0f;
  modState.osc2[voiceIndex] = 0.0f;
  modState.osc3[voiceIndex] = 0.0f;
  modState.osc4[voiceIndex] = 0.0f;

  modState.osc1Feedback[voiceIndex] = 0.0f;
  modState.osc2Feedback[voiceIndex] = 0.0f;
  modState.osc3Feedback[voiceIndex] = 0.0f;
  modState.osc4Feedback[voiceIndex] = 0.0f;
}

float getFmInputValue(WavetableOscModState& modState,
                      uint32_t voiceIndex,
                      FMSource src,
                      FMSource carrier) {
  switch (src) {
  case FMSource::Osc1: {
    if (carrier == FMSource::Osc1) {
      float fb = 0.5f * (modState.osc1[voiceIndex] + modState.osc1Feedback[voiceIndex]);
      modState.osc1Feedback[voiceIndex] = modState.osc1[voiceIndex];
      return fb;
    }
    return modState.osc1[voiceIndex];
  }
  case FMSource::Osc2: {
    if (carrier == FMSource::Osc2) {
      float fb = 0.5f * (modState.osc2[voiceIndex] + modState.osc2Feedback[voiceIndex]);
      modState.osc2Feedback[voiceIndex] = modState.osc2[voiceIndex];
      return fb;
    }
    return modState.osc2[voiceIndex];
  }
  case FMSource::Osc3: {
    if (carrier == FMSource::Osc3) {
      float fb = 0.5f * (modState.osc3[voiceIndex] + modState.osc3Feedback[voiceIndex]);
      modState.osc3Feedback[voiceIndex] = modState.osc3[voiceIndex];
      return fb;
    }
    return modState.osc3[voiceIndex];
  }
  case FMSource::Osc4: {
    if (carrier == FMSource::Osc4) {
      float fb = 0.5f * (modState.osc4[voiceIndex] + modState.osc4Feedback[voiceIndex]);
      modState.osc4Feedback[voiceIndex] = modState.osc4[voiceIndex];
      return fb;
    }
    return modState.osc4[voiceIndex];
  }
  default:
    return 0.0f;
  }
}

void addFMRoute(WavetableOsc& osc, FMSource src, float depth) {
  for (uint8_t r = 0; r < osc.fmRouteCount; r++) {
    if (osc.fmRoutes[r].source == src) {
      osc.fmRoutes[r].depth = depth;
      return;
    }
  }
  if (osc.fmRouteCount < NUM_OSCS)
    osc.fmRoutes[osc.fmRouteCount++] = {src, depth};
}

void removeFMRoute(WavetableOsc& osc, FMSource src) {
  for (uint8_t r = 0; r < osc.fmRouteCount; r++) {
    if (osc.fmRoutes[r].source == src) {
      for (uint8_t i = r; i < osc.fmRouteCount - 1; i++)
        osc.fmRoutes[i] = osc.fmRoutes[i + 1];
      osc.fmRouteCount--;
      return;
    }
  }
}

void clearFMRoutes(WavetableOsc& osc) {
  osc.fmRouteCount = 0;
}

void printFMRoutes(const WavetableOsc& osc, const char* carrierName) {
  if (osc.fmRouteCount == 0) {
    printf("%s: no routes\n", carrierName);
    return;
  }
  for (uint8_t r = 0; r < osc.fmRouteCount; r++)
    printf("  %s -> %s  depth=%.3f\n",
           fmSourceToString(osc.fmRoutes[r].source),
           carrierName,
           osc.fmRoutes[r].depth);
}

float readWavetable(const WavetableOsc& osc,
                    uint32_t voiceIndex,
                    float mipF,
                    float effectiveScanPos,
                    float fmPhaseOffset) {
  if (!osc.enabled || osc.bankPtr == nullptr)
    return 0.0f;
  return dsp_wt::readWavetable(osc.bankPtr,
                               osc.phases[voiceIndex],
                               mipF,
                               effectiveScanPos,
                               fmPhaseOffset);
}

float processOsc(WavetableOsc& osc,
                 uint32_t voiceIndex,
                 float mipF,
                 float effectiveScanPos,
                 float fmPhaseOffset,
                 float pitchIncrement) {
  if (!osc.enabled || osc.bankPtr == nullptr)
    return 0.0f;
  return dsp_wt::processWavetableOsc(osc.bankPtr,
                                     osc.phases[voiceIndex],
                                     mipF,
                                     effectiveScanPos,
                                     fmPhaseOffset,
                                     pitchIncrement);
}

} // namespace synth::wavetable::osc
