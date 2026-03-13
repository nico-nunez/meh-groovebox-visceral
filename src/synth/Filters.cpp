#include "Filters.h"

#include "dsp/Math.h"

#include <algorithm>
#include <cmath>

namespace synth::filters {

namespace {

float normalizeCutoff(float cutoffFreq, float invSampleRate) {
  // keep below Nyquist
  return std::min(cutoffFreq * invSampleRate, 0.49f);
}

} // namespace

// ==== SVF Helpers ====
void enableSVFilter(SVFilter& filter, bool enable) {
  if (enable && !filter.enabled) {
    // Reset all voice states
    for (uint32_t v = 0; v < MAX_VOICES; v++) {
      filter.statesL[v] = dsp::filters::SVFState{};
      filter.statesR[v] = dsp::filters::SVFState{};
    }
  }
  filter.enabled = enable;
}

void initSVFilter(SVFilter& filter, size_t voiceIndex) {
  filter.statesL[voiceIndex] = SVFState{};
  filter.statesR[voiceIndex] = SVFState{};
}

void updateSVFCoefficients(SVFilter& filter, float invSampleRate) {
  float Q = 0.5f + filter.resonance * 20.0f;
  float normalizedCutoff = normalizeCutoff(filter.cutoff, invSampleRate);
  filter.coeffs = dsp::filters::computeSVFCoeffs(normalizedCutoff, Q);
}

// Use when NOT passing modulation values (cutoff and/or resonance)
void processSVFilter(SVFilter& filter, float& inOutL, float& inOutR, uint32_t voiceIndex) {
  if (!filter.enabled)
    return;

  SVFOutputs outL = dsp::filters::processSVF(inOutL, filter.coeffs, filter.statesL[voiceIndex]);
  SVFOutputs outR = dsp::filters::processSVF(inOutR, filter.coeffs, filter.statesR[voiceIndex]);

  switch (filter.mode) {
  case SVFMode::COUNT:
  case SVFMode::Unknown:
  case SVFMode::LP:
    inOutL = outL.lp;
    inOutR = outR.lp;
    break;
  case SVFMode::HP:
    inOutL = outL.hp;
    inOutR = outR.hp;
    break;
  case SVFMode::BP:
    inOutL = outL.bp;
    inOutR = outR.bp;
    break;
  case SVFMode::Notch:
    inOutL = outL.lp + outL.hp;
    inOutR = outR.lp + outR.hp;
    break;
  }
}

void processSVFilter(SVFilter& filter,
                     float& inOutL,
                     float& inOutR,
                     uint32_t voiceIndex,
                     float cutoffHz,
                     float resonance,
                     float invSampleRate) {
  if (!filter.enabled)
    return;

  float normalizedCutoff = normalizeCutoff(cutoffHz, invSampleRate);

  bool isModulated = std::abs(filter.cutoff - cutoffHz) > 0.001f ||
                     std::abs(filter.resonance - resonance) > 0.001f;

  SVFCoeffs coeffs =
      isModulated ? dsp::filters::computeSVFCoeffs(normalizedCutoff, 0.5f + resonance * 20.0f)
                  : filter.coeffs;

  SVFOutputs outL = dsp::filters::processSVF(inOutL, coeffs, filter.statesL[voiceIndex]);
  SVFOutputs outR = dsp::filters::processSVF(inOutR, coeffs, filter.statesR[voiceIndex]);

  switch (filter.mode) {
  case SVFMode::COUNT:
  case SVFMode::Unknown:
  case SVFMode::LP:
    inOutL = outL.lp;
    inOutR = outR.lp;
    break;
  case SVFMode::HP:
    inOutL = outL.hp;
    inOutR = outR.hp;
    break;
  case SVFMode::BP:
    inOutL = outL.bp;
    inOutR = outR.bp;
    break;
  case SVFMode::Notch:
    inOutL = outL.lp + outL.hp;
    inOutR = outR.lp + outR.hp;
    break;
  }
}

// ==== Ladder Helpers ====
void enableLadderFilter(LadderFilter& filter, bool enable) {
  if (enable && !filter.enabled) {
    for (uint32_t i = 0; i < MAX_VOICES; i++) {
      filter.statesL[i] = dsp::filters::LadderState{};
      filter.statesR[i] = dsp::filters::LadderState{};
      filter.y4EstimateL[i] = 0.0f;
      filter.y4EstimateR[i] = 0.0f;
    }
  }
  filter.enabled = enable;
}

void initLadderFilter(LadderFilter& filter, size_t voiceIndex) {
  filter.statesL[voiceIndex] = LadderState{};
  filter.statesR[voiceIndex] = LadderState{};
  filter.y4EstimateL[voiceIndex] = 0.0f;
  filter.y4EstimateR[voiceIndex] = 0.0f;
}

void updateLadderCoefficient(LadderFilter& filter, float invSampleRate) {
  float normalizedCutoff = normalizeCutoff(filter.cutoff, invSampleRate);
  filter.coeff = dsp::math::fastTan(dsp::math::PI_F * normalizedCutoff);
}

// Use when NOT passing modulation values (cutoff and/or resonance)
void processLadderFilter(LadderFilter& filter, float& inOutL, float& inOutR, uint32_t voiceIndex) {
  if (!filter.enabled)
    return;

  float res = filter.resonance * 4.0f;

  if (filter.drive > 1.001f) {
    inOutL = dsp::filters::processLadderNonlinear(inOutL,
                                                  filter.coeff,
                                                  res,
                                                  filter.drive,
                                                  filter.y4EstimateL[voiceIndex],
                                                  filter.statesL[voiceIndex]);
    inOutR = dsp::filters::processLadderNonlinear(inOutR,
                                                  filter.coeff,
                                                  res,
                                                  filter.drive,
                                                  filter.y4EstimateR[voiceIndex],
                                                  filter.statesR[voiceIndex]);
  } else {
    inOutL = dsp::filters::processLadder(inOutL, filter.coeff, res, filter.statesL[voiceIndex]);
    inOutR = dsp::filters::processLadder(inOutR, filter.coeff, res, filter.statesR[voiceIndex]);
  }
}

// With Modulation
void processLadderFilter(LadderFilter& filter,
                         float& inOutL,
                         float& inOutR,
                         uint32_t voiceIndex,
                         float cutoffHz,
                         float resonance,
                         float invSampleRate) {
  if (!filter.enabled)
    return;

  float normalizedCutoff = normalizeCutoff(cutoffHz, invSampleRate);
  float coeff = std::abs(filter.cutoff - cutoffHz) > 0.001f
                    ? dsp::math::fastTan(dsp::math::PI_F * normalizedCutoff)
                    : filter.coeff;

  float res = resonance * 4.0f;

  if (filter.drive > 1.001f) {
    inOutL = dsp::filters::processLadderNonlinear(inOutL,
                                                  coeff,
                                                  res,
                                                  filter.drive,
                                                  filter.y4EstimateL[voiceIndex],
                                                  filter.statesL[voiceIndex]);
    inOutR = dsp::filters::processLadderNonlinear(inOutR,
                                                  coeff,
                                                  res,
                                                  filter.drive,
                                                  filter.y4EstimateR[voiceIndex],
                                                  filter.statesR[voiceIndex]);
  } else {
    inOutL = dsp::filters::processLadder(inOutL, coeff, res, filter.statesL[voiceIndex]);
    inOutR = dsp::filters::processLadder(inOutR, coeff, res, filter.statesR[voiceIndex]);
  }
}
} // namespace synth::filters
