#include "Unison.h"

#include "dsp/Math.h"

namespace synth::unison {

void initUnisonSubPhases(UnisonState& unison,
                         const PhaseMode phaseModes[NUM_OSCS],
                         const float randomRanges[NUM_OSCS],
                         const float resetPhases[NUM_OSCS],
                         uint32_t voiceIndex) {

  for (uint8_t osc = 0; osc < NUM_OSCS; osc++) {
    PhaseMode mode = phaseModes[osc];

    if (mode == PhaseMode::Free)
      continue; // don't reset any sub-voice phases for this oscillator

    for (uint8_t uni = 0; uni < MAX_UNISON_VOICES; uni++) {
      float ph = 0.0f;

      switch (mode) {
      case PhaseMode::Reset:
      case PhaseMode::Unknown:
        ph = resetPhases[osc];
        break;

      case PhaseMode::Random:
        ph = (dsp::math::randNoiseValue() + 1.0f) * 0.5f * randomRanges[osc];
        break;

      case PhaseMode::Spread:
        // Evenly distribute sub-voices across [resetPhase, resetPhase + 1)
        ph = std::fmod(resetPhases[osc] +
                           static_cast<float>(uni) / static_cast<float>(unison.voices),
                       1.0f);
        break;

      default:
        break;
      }
      unison.subPhases[osc][uni][voiceIndex] = ph;
    }
  }
}

void updateDetuneOffsets(unison::UnisonState& u) {
  dsp::math::computeDetuneOffsets(u.detuneOffsets, u.voices, u.detune);
  for (int8_t i = 0; i < u.voices; i++) {
    u.detuneRatios[i] = dsp::math::semitonesToFreqRatio(u.detuneOffsets[i]);
  }
}

void updatePanPositions(unison::UnisonState& u) {
  dsp::math::computePanPositions(u.panPositions, u.voices, u.spread);
  for (int8_t i = 0; i < u.voices; i++) {
    dsp::math::panToLR(u.panPositions[i], u.panGainsL[i], u.panGainsR[i]);
  }
}

void updateGainComp(unison::UnisonState& u) {
  u.gainComp = dsp::math::unisonGainComp(u.voices);
}

} // namespace synth::unison
