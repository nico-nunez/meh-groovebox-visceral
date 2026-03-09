#include "VoicePool.h"

#include "dsp/Buffers.h"
#include "synth/Envelope.h"
#include "synth/LFO.h"
#include "synth/MonoMode.h"
#include "synth/Noise.h"
#include "synth/ParamRanges.h"
#include "synth/SignalChain.h"
#include "synth/WavetableBanks.h"
#include "synth/WavetableOsc.h"

#include "dsp/Filters.h"
#include "dsp/Math.h"
#include "dsp/Wavetable.h"

#include <cstddef>
#include <cstdint>

namespace synth::voices {
using mod_matrix::ModDest;
using mod_matrix::ModDest2D;
using mod_matrix::ModRoute;
using mod_matrix::ModSrc;

namespace osc = wavetable::osc;
namespace env = envelope;

using dsp::buffers::StereoBuffer;

// =========================
// VoicePool Configuration
// =========================
void updateVoicePoolConfig(VoicePool& pool, const VoicePoolConfig& config) {
  pool.sampleRate = config.sampleRate;
  pool.invSampleRate = 1.0f / config.sampleRate;

  pool.mono.enabled = config.mono;

  pool.masterGain = config.masterGain;

  osc::updateConfig(pool.osc1, config.osc1);
  osc::updateConfig(pool.osc2, config.osc2);
  osc::updateConfig(pool.osc3, config.osc3);
  osc::updateConfig(pool.osc4, config.osc4);

  noise::updateConfig(pool.noise, config.noise);

  int enabledCount = pool.osc1.enabled + pool.osc2.enabled + pool.osc3.enabled + pool.osc4.enabled +
                     pool.noise.enabled;
  pool.oscMixGain = (enabledCount > 0) ? 1.0f / static_cast<float>(enabledCount) : 1.0f;

  filters::updateSVFCoefficients(pool.svf, pool.invSampleRate);
  filters::updateLadderCoefficient(pool.ladder, pool.invSampleRate);

  mod_matrix::addRoute(pool.modMatrix, ModSrc::FilterEnv, ModDest::SVFCutoff, 0.0f);
  mod_matrix::addRoute(pool.modMatrix, ModSrc::FilterEnv, ModDest::LadderCutoff, 0.0f);
}

void initVoicePool(VoicePool& pool, const VoicePoolConfig& config) {
  using namespace wavetable::banks;
  using signal_chain::SignalProcessor;

  // TODO(nico): should this be part of VoicePoolConfig?
  static constexpr uint8_t SIGNAL_CHAIN_LEN = 3;
  static constexpr SignalProcessor SIGNAL_CHAIN[SIGNAL_CHAIN_LEN] = {SignalProcessor::SVF,
                                                                     SignalProcessor::Ladder,
                                                                     SignalProcessor::Saturator};
  initFactoryBanks();

  updateVoicePoolConfig(pool, config);

  // Initialize curve tables (empty to start)
  env::updateCurveTables(pool.ampEnv);
  env::updateCurveTables(pool.filterEnv);
  env::updateCurveTables(pool.modEnv);

  // Initialization Stereo (balanced)
  for (uint8_t i = 0; i < MAX_VOICES; i++) {
    pool.panL[i] = 1.0f;
    pool.panR[i] = 1.0f;
  }

  // Initialize sustaion
  for (uint8_t i = 0; i < MAX_VOICES; i++)
    pool.sustain.notes[i] = false;

  pool.porta.coeff = dsp::math::calcPortamento(pool.porta.time, config.sampleRate);

  // TODO(nico-nunez): find a better way!!!
  pool.osc1.bank = getBankByID(BankID::Saw);
  pool.osc1.detuneAmount = 10.0f;

  pool.osc2.bank = getBankByID(BankID::Saw);
  pool.osc2.mixLevel = 0.7f;
  pool.osc2.octaveOffset = -1;
  pool.osc2.detuneAmount = -10.0f;

  pool.osc3.bank = getBankByID(BankID::Sine);
  pool.osc3.mixLevel = 0.5f;

  pool.osc4.bank = getBankByID(BankID::Sine);
  pool.osc4.octaveOffset = -2;
  pool.osc4.mixLevel = 0.5f;

  signal_chain::setChain(pool.signalChain, SIGNAL_CHAIN, SIGNAL_CHAIN_LEN);
}

// =========================
//  Voice Allocation
// =========================
// Find free or oldest voice index for voice Initialization
uint32_t allocateVoiceIndex(VoicePool& pool, bool& outStolen) {
  uint32_t oldestIndex = MAX_VOICES; // out of range
  uint32_t oldestNoteOnTime = UINT32_MAX;
  outStolen = false;

  for (uint32_t i = 0; i < MAX_VOICES; i++) {
    if (!pool.isActive[i]) {
      return i; // return available voice index
    } else {
      if (pool.noteOnTimes[i] < oldestNoteOnTime) {
        oldestNoteOnTime = pool.noteOnTimes[i];
        oldestIndex = i;
      }
    }
  }

  outStolen = true;
  removeInactiveIndex(pool, oldestIndex);

  return oldestIndex;
}

void addActiveIndex(VoicePool& pool, uint32_t voiceIndex) {
  pool.activeIndices[pool.activeCount] = voiceIndex;
  pool.activeCount++;
}

// TODO(nico): this is confusing.  there must be a better way
void removeInactiveIndex(VoicePool& pool, uint32_t voiceIndex) {
  uint32_t removeIndex = pool.activeCount; // out of range of active

  for (uint32_t i = 0; i < pool.activeCount; i++) {
    if (pool.activeIndices[i] == voiceIndex) {
      removeIndex = i;
      break;
    }
  }

  if (removeIndex == pool.activeCount)
    return;

  // Swap current inactive with most recent active
  pool.activeCount--;
  pool.activeIndices[removeIndex] = pool.activeIndices[pool.activeCount];
  pool.isActive[voiceIndex] = 0;
}

// =========================
// Voice Initialization
// =========================

// ==== <Initialization Helpers> ====
namespace {

bool isValidActiveIndex(uint32_t index) {
  return index < MAX_VOICES;
}

uint32_t findVoiceRelease(VoicePool& pool, uint8_t midiNote) {
  for (uint32_t i = 0; i < pool.activeCount; i++) {
    uint32_t voiceIndex = pool.activeIndices[i];
    if (pool.midiNotes[voiceIndex] == midiNote &&
        pool.ampEnv.states[voiceIndex] != env::EnvelopeStatus::Release &&
        pool.ampEnv.states[voiceIndex] != env::EnvelopeStatus::Idle) {

      return voiceIndex;
    }
  }
  return MAX_VOICES;
}

// Compute portamento offset in semitones from lastNote to midiNote
float calcPortaOffset(VoicePool& pool, uint8_t midiNote) {
  bool legatoActive = pool.porta.legato && pool.activeCount > 0;
  bool alwaysActive = !pool.porta.legato;
  bool doPorta = pool.porta.enabled && (legatoActive || alwaysActive) && (pool.porta.lastNote > 0);

  if (doPorta) {
    return static_cast<float>(pool.porta.lastNote) - static_cast<float>(midiNote);
  }
  return 0.0f;
}

} // namespace
// ==== </Initialization Helpers> ====

// Mono Legato (adjust pitch without reseting phases if legato)
void redirectVoicePitch(VoicePool& pool, uint32_t voiceIndex, uint8_t midiNote, float sampleRate) {
  pool.midiNotes[voiceIndex] = midiNote;
  osc::updateOscPitch(pool.osc1, voiceIndex, midiNote, sampleRate);
  osc::updateOscPitch(pool.osc2, voiceIndex, midiNote, sampleRate);
  osc::updateOscPitch(pool.osc3, voiceIndex, midiNote, sampleRate);
  osc::updateOscPitch(pool.osc4, voiceIndex, midiNote, sampleRate);
}

void initVoice(VoicePool& pool,
               uint32_t voiceIndex,
               uint8_t midiNote,
               uint8_t velocity,
               uint32_t noteOnTime,
               bool retrigger,
               float sampleRate) {

  pool.isActive[voiceIndex] = 1;
  pool.sustain.notes[voiceIndex] = false;
  pool.midiNotes[voiceIndex] = midiNote;
  pool.noteOnTimes[voiceIndex] = noteOnTime;
  pool.velocities[voiceIndex] = velocity / 127.0f;

  pool.sampleRate = sampleRate;
  pool.invSampleRate = 1.0f / sampleRate;

  // Reset Stereo (balanced)
  pool.panL[voiceIndex] = 1.0f;
  pool.panR[voiceIndex] = 1.0f;

  for (int d = 0; d < ModDest::DEST_COUNT; d++) {
    pool.modMatrix.prevDestValues[d][voiceIndex] = 0.0f;
    pool.modMatrix.destValues[d][voiceIndex] = 0.0f;
  }
  osc::resetOscModState(pool.oscModState, voiceIndex);

  osc::initOsc(pool.osc1, voiceIndex, midiNote, sampleRate);
  osc::initOsc(pool.osc2, voiceIndex, midiNote, sampleRate);
  osc::initOsc(pool.osc3, voiceIndex, midiNote, sampleRate);
  osc::initOsc(pool.osc4, voiceIndex, midiNote, sampleRate);

  if (retrigger) {
    env::retriggerEnvelope(pool.ampEnv, voiceIndex, sampleRate);
    env::retriggerEnvelope(pool.filterEnv, voiceIndex, sampleRate);
    env::retriggerEnvelope(pool.modEnv, voiceIndex, sampleRate);
  } else {
    env::initEnvelope(pool.ampEnv, voiceIndex, sampleRate);
    env::initEnvelope(pool.filterEnv, voiceIndex, sampleRate);
    env::initEnvelope(pool.modEnv, voiceIndex, sampleRate);

    filters::initSVFilter(pool.svf, voiceIndex);
    filters::initLadderFilter(pool.ladder, voiceIndex);
  }
}

void releaseMonoVoice(VoicePool& pool) {
  uint32_t v = pool.mono.voiceIndex;
  if (v < MAX_VOICES && pool.isActive[v]) {
    envelope::triggerRelease(pool.ampEnv, v);
    envelope::triggerRelease(pool.filterEnv, v);
    envelope::triggerRelease(pool.modEnv, v);
  }
  pool.mono.voiceIndex = MAX_VOICES;
  pool.mono.stackDepth = 0;
}

void releaseVoice(VoicePool& pool, uint8_t midiNote) {
  uint32_t voiceIndex = pool.mono.enabled && pool.mono.voiceIndex < MAX_VOICES
                            ? pool.mono.voiceIndex
                            : findVoiceRelease(pool, midiNote);

  if (!isValidActiveIndex(voiceIndex))
    return;

  // Mono: revert to previous held note instead of releasing
  if (pool.mono.enabled && pool.mono.stackDepth > 0) {
    uint8_t prevNote = pool.mono.noteStack[pool.mono.stackDepth - 1];

    if (pool.porta.enabled) {
      pool.porta.offsets[voiceIndex] = static_cast<float>(midiNote) - static_cast<float>(prevNote);
      pool.porta.lastNote = prevNote;
    }

    redirectVoicePitch(pool, voiceIndex, prevNote, pool.sampleRate);
    return; // don't release envs below
  }

  envelope::triggerRelease(pool.ampEnv, voiceIndex);
  envelope::triggerRelease(pool.filterEnv, voiceIndex);
  envelope::triggerRelease(pool.modEnv, voiceIndex);
}

// ========================
// Note Event Handers
// ========================
void handleNoteOn(VoicePool& pool,
                  uint8_t midiNote,
                  uint8_t velocity,
                  uint32_t noteOnTime,
                  float sampleRate) {
  // Handle portamento
  float portaOffset = calcPortaOffset(pool, midiNote);

  // ==== Mono Path(s) ====
  if (pool.mono.enabled) {
    pool.mono.heldNotes[midiNote] = true;
    mono::pushNoteToStack(pool.mono, midiNote);

    // Handle existing note(s) being held/active
    if (pool.mono.voiceIndex < MAX_VOICES) {
      uint32_t current = pool.mono.voiceIndex;
      pool.porta.offsets[current] = portaOffset;

      bool isPlaying = pool.ampEnv.states[current] != envelope::EnvelopeStatus::Release &&
                       pool.ampEnv.states[current] != envelope::EnvelopeStatus::Idle;

      // Path 2: Legato (adjust pitch/redirect)
      if (pool.mono.legato && isPlaying) {
        redirectVoicePitch(pool, current, midiNote, sampleRate);
        pool.porta.lastNote = midiNote;
        return;
      }
      // Path 3: No legato or note is releasing/idle (full retrigger)
      if (!pool.isActive[current])
        addActiveIndex(pool, current);

      initVoice(pool, current, midiNote, velocity, noteOnTime, true, sampleRate);
      pool.porta.lastNote = midiNote;
      return;
    }
    // Path 1: initial/no existing notes (proceed as normal)
  }

  // ==== Normal/Poly Path ====
  bool isStolen = false; // retrigger if stolen
  uint32_t voiceIndex = allocateVoiceIndex(pool, isStolen);

  initVoice(pool, voiceIndex, midiNote, velocity, noteOnTime, isStolen, sampleRate);
  pool.porta.offsets[voiceIndex] = portaOffset;

  if (pool.mono.enabled)
    pool.mono.voiceIndex = voiceIndex;

  pool.porta.lastNote = midiNote;

  if (pool.activeCount == 0) {
    if (pool.lfo1.retrigger)
      pool.lfo1.phase = 0.0f;
    if (pool.lfo2.retrigger)
      pool.lfo2.phase = 0.0f;
    if (pool.lfo3.retrigger)
      pool.lfo3.phase = 0.0f;
  }

  addActiveIndex(pool, voiceIndex);
}

void handleNoteOff(VoicePool& pool, uint8_t midiNote) {
  if (pool.mono.enabled) {
    pool.mono.heldNotes[midiNote] = false;
    mono::removeNoteFromStack(pool.mono, midiNote);
  }

  // Sustain & don't release voice
  if (pool.sustain.held) {
    for (uint32_t i = 0; i < pool.activeCount; i++) {
      uint32_t v = pool.activeIndices[i]; // voiceIndex
      if (pool.midiNotes[v] == midiNote)
        pool.sustain.notes[v] = true;
    }
    return;
  }

  releaseVoice(pool, midiNote);
}

// ===========================
// Voice Processing
// ===========================

namespace {
namespace osc = wavetable::osc;
namespace dsp_wt = dsp::wavetable;

// ==== <Processing Helpers> ====

/* ==== Pre-pass: once per block, once per active voice ====
 * Advance block-rate envelopes (filterEnv, modEnv).
 * ampEnv is NOT advanced here; it runs per-sample in the hot loop below.
 * ==================================================================== */
void preProcessBlock(VoicePool& pool, size_t numSamples) {
  float invNumSamples = 1.0f / static_cast<float>(numSamples);

  mod_matrix::clearModDestSteps(pool.modMatrix);

  for (uint32_t i = pool.activeCount; i > 0; i--) {
    uint32_t v = pool.activeIndices[i - 1]; // voiceIndex

    float modSrcs[ModSrc::SRC_COUNT] = {};

    // NOTE(nico): since this is processed in the main loop it's setting the
    // last value of the PRIOR block on first run; should be fine for now
    modSrcs[ModSrc::AmpEnv] = pool.ampEnv.levels[v]; // processed in main loop
    modSrcs[ModSrc::FilterEnv] = envelope::processEnvelope(pool.filterEnv, v);
    modSrcs[ModSrc::ModEnv] = envelope::processEnvelope(pool.modEnv, v);
    modSrcs[ModSrc::Velocity] = pool.velocities[v];
    modSrcs[ModSrc::ModWheel] = pool.modWheelValue;

    float modDests[ModDest::DEST_COUNT] = {};
    for (uint8_t r = 0; r < pool.modMatrix.count; r++) {
      const ModRoute& route = pool.modMatrix.routes[r];

      if (route.src == ModSrc::NoSrc || route.dest == ModDest::NoDest)
        continue;

      // LFO sources are sample-rate — handled in the hot loop, not here
      if (route.src == ModSrc::LFO1 || route.src == ModSrc::LFO2 || route.src == ModSrc::LFO3)
        continue;

      modDests[route.dest] += modSrcs[route.src] * route.amount;
    }

    // Set modulation destination values
    for (int d = 0; d < ModDest::DEST_COUNT; d++)
      pool.modMatrix.destValues[d][v] = modDests[d];

    // Interpolation setup for fast destinations (pitch)
    mod_matrix::setModDestStep(pool.modMatrix, ModDest::Osc1Pitch, v, invNumSamples);
    mod_matrix::setModDestStep(pool.modMatrix, ModDest::Osc2Pitch, v, invNumSamples);
    mod_matrix::setModDestStep(pool.modMatrix, ModDest::Osc3Pitch, v, invNumSamples);
    mod_matrix::setModDestStep(pool.modMatrix, ModDest::Osc4Pitch, v, invNumSamples);
  }
};

// Process LFOs (once per sample - global)
void processLFOs(VoicePool& pool) {
  float effectiveRate1 = pool.lfo1.rate + pool.modMatrix.destValues[ModDest::LFO1Rate][0];
  float effectiveRate2 = pool.lfo2.rate + pool.modMatrix.destValues[ModDest::LFO2Rate][0];
  float effectiveRate3 = pool.lfo3.rate + pool.modMatrix.destValues[ModDest::LFO3Rate][0];

  float effectiveAmp1 = pool.lfo1.amplitude + pool.modMatrix.destValues[ModDest::LFO1Amplitude][0];
  float effectiveAmp2 = pool.lfo2.amplitude + pool.modMatrix.destValues[ModDest::LFO2Amplitude][0];
  float effectiveAmp3 = pool.lfo3.amplitude + pool.modMatrix.destValues[ModDest::LFO3Amplitude][0];

  // Apply LFO-to-LFO contributions — the previous sample's LFO outputs.
  // On the first sample it's zero-initialized so has no effect.
  // From sample 1 onward it holds real values written in Step 3 last iteration.
  for (uint8_t r = 0; r < pool.modMatrix.count; r++) {

    const ModRoute& route = pool.modMatrix.routes[r];

    if (route.src == ModSrc::LFO1 || route.src == ModSrc::LFO2 || route.src == ModSrc::LFO3) {
      float srcVal = 0;
      switch (route.src) {
      case ModSrc::LFO1:
        srcVal = pool.lfoModState.lfo1;
        break;
      case ModSrc::LFO2:
        srcVal = pool.lfoModState.lfo2;
        break;
      case ModSrc::LFO3:
        srcVal = pool.lfoModState.lfo3;
        break;
      default:
        break;
      }

      switch (route.dest) {
      case ModDest::LFO1Rate:
        effectiveRate1 += srcVal * route.amount;
        break;
      case ModDest::LFO2Rate:
        effectiveRate2 += srcVal * route.amount;
        break;
      case ModDest::LFO3Rate:
        effectiveRate3 += srcVal * route.amount;
        break;

      case ModDest::LFO1Amplitude:
        effectiveAmp1 += srcVal * route.amount;
        break;
      case ModDest::LFO2Amplitude:
        effectiveAmp2 += srcVal * route.amount;
        break;
      case ModDest::LFO3Amplitude:
        effectiveAmp3 += srcVal * route.amount;
        break;

      default:
        break;
      }
    }
  }

  pool.lfoModState.lfo1 = advanceLFO(pool.lfo1, pool.invSampleRate, effectiveRate1, effectiveAmp1);
  pool.lfoModState.lfo2 = advanceLFO(pool.lfo2, pool.invSampleRate, effectiveRate2, effectiveAmp2);
  pool.lfoModState.lfo3 = advanceLFO(pool.lfo3, pool.invSampleRate, effectiveRate3, effectiveAmp3);
}

// Calculate interpolation for pitch increment (hot-loop)
float interpolatePitchInc(const WavetableOsc& osc,
                          const ModMatrix& matrix,
                          const float* lfoContribs,
                          const float* portaOffsets,
                          float semitoneBend,
                          ModDest dest,
                          uint32_t voiceIndex,
                          uint32_t sampleIndex) {
  const uint32_t v = voiceIndex;
  const uint32_t s = sampleIndex;

  float pitchMod = matrix.prevDestValues[dest][v] +
                   (matrix.destStepValues[dest][v] * static_cast<float>(s)) + lfoContribs[dest] +
                   semitoneBend + portaOffsets[v];

  // Calculate and return modulated phase increment
  return osc.phaseIncrements[v] * dsp::math::semitonesToFreqRatio(pitchMod) * osc.fmRatio;
}

// TODO(nico): refactor this!!!
// Process Oscillators with interpolation and mix (sum) values
float processAndMixOscillators(VoicePool& pool, uint32_t voiceIndex, uint32_t sampleIndex) {
  using dsp::wavetable::selectMipLevel;
  using param::ranges::osc::clampScanPos;
  using std::max;
  using wavetable::osc::FMSource;

  const uint32_t v = voiceIndex;
  const uint32_t s = sampleIndex;

  auto& modMatrix = pool.modMatrix;
  auto& dest = pool.modMatrix.destValues;

  auto& osc1 = pool.osc1;
  auto& osc2 = pool.osc2;
  auto& osc3 = pool.osc3;
  auto& osc4 = pool.osc4;
  auto& oscModState = pool.oscModState;

  auto& lfo = pool.lfoModState.contribs;

  auto& portaOffsets = pool.porta.offsets;

  float semitoneBend = pool.pitchBend.value * pool.pitchBend.range;

  float pitchInc1 = interpolatePitchInc(osc1,
                                        modMatrix,
                                        lfo,
                                        portaOffsets,
                                        semitoneBend,
                                        ModDest::Osc1Pitch,
                                        v,
                                        s);

  float pitchInc2 = interpolatePitchInc(osc2,
                                        modMatrix,
                                        lfo,
                                        portaOffsets,
                                        semitoneBend,
                                        ModDest::Osc2Pitch,
                                        v,
                                        s);

  float pitchInc3 = interpolatePitchInc(osc3,
                                        modMatrix,
                                        lfo,
                                        portaOffsets,
                                        semitoneBend,
                                        ModDest::Osc3Pitch,
                                        v,
                                        s);

  float pitchInc4 = interpolatePitchInc(osc4,
                                        modMatrix,
                                        lfo,
                                        portaOffsets,
                                        semitoneBend,
                                        ModDest::Osc4Pitch,
                                        v,
                                        s);

  float mip1 = selectMipLevel(pitchInc1 * dsp_wt::TABLE_SIZE_F);
  float mip2 = selectMipLevel(pitchInc2 * dsp_wt::TABLE_SIZE_F);
  float mip3 = selectMipLevel(pitchInc3 * dsp_wt::TABLE_SIZE_F);
  float mip4 = selectMipLevel(pitchInc4 * dsp_wt::TABLE_SIZE_F);

  float scan1 =
      clampScanPos(osc1.scanPos + dest[ModDest::Osc1ScanPos][v] + lfo[ModDest::Osc1ScanPos]);
  float scan2 =
      clampScanPos(osc2.scanPos + dest[ModDest::Osc2ScanPos][v] + lfo[ModDest::Osc2ScanPos]);
  float scan3 =
      clampScanPos(osc3.scanPos + dest[ModDest::Osc3ScanPos][v] + lfo[ModDest::Osc3ScanPos]);
  float scan4 =
      clampScanPos(osc4.scanPos + dest[ModDest::Osc4ScanPos][v] + lfo[ModDest::Osc4ScanPos]);

  float fmDepth1 =
      max(0.0f, osc1.fmDepth + dest[ModDest::Osc1FMDepth][v] + lfo[ModDest::Osc1FMDepth]);
  float fmDepth2 =
      max(0.0f, osc2.fmDepth + dest[ModDest::Osc2FMDepth][v] + lfo[ModDest::Osc2FMDepth]);
  float fmDepth3 =
      max(0.0f, osc3.fmDepth + dest[ModDest::Osc3FMDepth][v] + lfo[ModDest::Osc3FMDepth]);
  float fmDepth4 =
      max(0.0f, osc4.fmDepth + dest[ModDest::Osc4FMDepth][v] + lfo[ModDest::Osc4FMDepth]);

  float fm1 = osc::getFmInputValue(oscModState, v, osc1.fmSource, FMSource::Osc1) * fmDepth1;
  float fm2 = osc::getFmInputValue(oscModState, v, osc2.fmSource, FMSource::Osc2) * fmDepth2;
  float fm3 = osc::getFmInputValue(oscModState, v, osc3.fmSource, FMSource::Osc3) * fmDepth3;
  float fm4 = osc::getFmInputValue(oscModState, v, osc4.fmSource, FMSource::Osc4) * fmDepth4;

  float out1 = osc::processOsc(osc1, v, mip1, scan1, fm1, pitchInc1);
  float out2 = osc::processOsc(osc2, v, mip2, scan2, fm2, pitchInc2);
  float out3 = osc::processOsc(osc3, v, mip3, scan3, fm3, pitchInc3);
  float out4 = osc::processOsc(osc4, v, mip4, scan4, fm4, pitchInc4);

  oscModState.osc1[v] = out1;
  oscModState.osc2[v] = out2;
  oscModState.osc3[v] = out3;
  oscModState.osc4[v] = out4;

  float mix1 = max(0.0f, osc1.mixLevel + dest[ModDest::Osc1Mix][v] + lfo[ModDest::Osc1Mix]);
  float mix2 = max(0.0f, osc2.mixLevel + dest[ModDest::Osc2Mix][v] + lfo[ModDest::Osc2Mix]);
  float mix3 = max(0.0f, osc3.mixLevel + dest[ModDest::Osc3Mix][v] + lfo[ModDest::Osc3Mix]);
  float mix4 = max(0.0f, osc4.mixLevel + dest[ModDest::Osc4Mix][v] + lfo[ModDest::Osc4Mix]);

  float noiseOut = noise::processNoise(pool.noise);

  // no separate mix multiplier for noiseOut — already scaled inside processNoise
  return (out1 * mix1 + out2 * mix2 + out3 * mix3 + out4 * mix4 + noiseOut) * pool.oscMixGain;
}

// Signal Chain processing
float processSignalChain(VoicePool& pool, float mixedOscs, uint32_t voiceIndex) {
  auto& svf = pool.svf;
  auto& ladder = pool.ladder;

  auto& destValues = pool.modMatrix.destValues;
  auto& lfoContribs = pool.lfoModState.contribs;

  uint32_t v = voiceIndex;
  float signal = mixedOscs;

  for (uint8_t i = 0; i < pool.signalChain.length; i++) {
    switch (pool.signalChain.slots[i]) {

    case SignalProcessor::SVF: {
      float cutoff = dsp::filters::modulateCutoff(svf.cutoff,
                                                  destValues[ModDest::SVFCutoff][v] +
                                                      lfoContribs[ModDest::SVFCutoff]);
      float resonance =
          svf.resonance + destValues[ModDest::SVFResonance][v] + lfoContribs[ModDest::SVFResonance];
      signal = filters::processSVFilter(svf, signal, v, cutoff, resonance, pool.invSampleRate);
      break;
    }

    case SignalProcessor::Ladder: {
      float cutoff = dsp::filters::modulateCutoff(ladder.cutoff,
                                                  destValues[ModDest::LadderCutoff][v] +
                                                      lfoContribs[ModDest::LadderCutoff]);
      float resonance = ladder.resonance + destValues[ModDest::LadderResonance][v] +
                        lfoContribs[ModDest::LadderResonance];
      signal =
          filters::processLadderFilter(ladder, signal, v, cutoff, resonance, pool.invSampleRate);
      break;
    }

    case SignalProcessor::Saturator:
      signal = synth::saturator::processSaturator(pool.saturator, signal);
      break;

    case SignalProcessor::None:
      break;
    }
  }

  return signal;
}

/* ==== Post-block: Update prevDestValues with current value ====
 * Will be referenced at the Pre-pass of the next block
 * Active voices only
 * ============================================================== */
void postProcessBlock(VoicePool& pool) {
  auto& prevDestValues = pool.modMatrix.prevDestValues;
  auto& destValues = pool.modMatrix.destValues;

  for (uint32_t i = 0; i < pool.activeCount; i++) {
    uint32_t v = pool.activeIndices[i]; // voiceIndex

    prevDestValues[ModDest::Osc1Pitch][v] = destValues[ModDest::Osc1Pitch][v];
    prevDestValues[ModDest::Osc2Pitch][v] = destValues[ModDest::Osc2Pitch][v];
    prevDestValues[ModDest::Osc3Pitch][v] = destValues[ModDest::Osc3Pitch][v];
    prevDestValues[ModDest::Osc4Pitch][v] = destValues[ModDest::Osc4Pitch][v];
  }
}

//==== </Processing Helpers> ====
} // namespace
//
void processVoices(VoicePool& pool, StereoBuffer output, size_t numSamples) {
  auto& lfoContribs = pool.lfoModState.contribs;

  // ==== Set and process Mod Matrix values (block-rate) ====
  preProcessBlock(pool, numSamples);

  // ==== Calculate each sample value (audio-rate) ====
  for (uint32_t sIndex = 0; sIndex < numSamples; sIndex++) {
    float sampleL = 0.0f;
    float sampleR = 0.0f;

    processLFOs(pool);

    // ---- Step 4: Compute audio-destination contributions ----
    // Global LFOs: all active voices read the same lfoContribs[] for this sample.
    // LFO rate/amplitude destinations are excluded — already handled in Step 1.
    lfo::clearContribs(pool.lfoModState);

    for (uint8_t r = 0; r < pool.modMatrix.count; r++) {
      const ModRoute& route = pool.modMatrix.routes[r];
      if (route.src == ModSrc::NoSrc || route.dest == ModDest::NoDest)
        continue;

      // Skip LFO parameter destinations — handled via lfoOutputs in Step 1.
      if (route.dest == ModDest::LFO1Rate || route.dest == ModDest::LFO1Amplitude ||
          route.dest == ModDest::LFO2Rate || route.dest == ModDest::LFO2Amplitude ||
          route.dest == ModDest::LFO3Rate || route.dest == ModDest::LFO3Amplitude)
        continue;

      if (route.src == ModSrc::LFO1)
        lfoContribs[route.dest] += pool.lfoModState.lfo1 * route.amount;

      else if (route.src == ModSrc::LFO2)
        lfoContribs[route.dest] += pool.lfoModState.lfo2 * route.amount;

      else if (route.src == ModSrc::LFO3)
        lfoContribs[route.dest] += pool.lfoModState.lfo3 * route.amount;
    }

    // Iterating backwards to more easily deal with swapping voices
    // that have/will become Idle/inactive after processing
    for (uint32_t i = pool.activeCount; i > 0; i--) {
      uint32_t vIndex = pool.activeIndices[i - 1];

      if (pool.porta.coeff > 0.0f && pool.porta.offsets[vIndex] != 0.0f) {
        pool.porta.offsets[vIndex] *= pool.porta.coeff;
        if (fabsf(pool.porta.offsets[vIndex]) < 0.01f)
          pool.porta.offsets[vIndex] = 0.0f;
      }

      float mixedOscs = processAndMixOscillators(pool, vIndex, sIndex);

      float signal = processSignalChain(pool, mixedOscs, vIndex);

      float ampEnv = envelope::processEnvelope(pool.ampEnv, vIndex);

      // Check if amplitude envelope completed - remove immediately
      if (pool.ampEnv.states[vIndex] == envelope::EnvelopeStatus::Idle) {
        removeInactiveIndex(pool, vIndex);
        // No index adjustment needed - iterating backwards
      }

      float voiceSample = signal * ampEnv * pool.velocities[vIndex] * VOICE_GAIN;
      sampleL += voiceSample * pool.panL[vIndex];
      sampleR += voiceSample * pool.panR[vIndex];
    }

    // TODO(nico): Basic soft clip for now.
    // Mainly for protection and not as an effect
    output.left[sIndex] = dsp::math::fastTanh(sampleL * pool.masterGain);
    output.right[sIndex] = dsp::math::fastTanh(sampleR * pool.masterGain);
  }

  // Advance pitch interpolation state for next block
  postProcessBlock(pool);
}

} // namespace synth::voices
