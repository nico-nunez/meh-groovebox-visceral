#include "VoicePool.h"

#include "Envelope.h"
#include "LFO.h"
#include "MonoMode.h"
#include "Noise.h"
#include "ParamRanges.h"
#include "Unison.h"
#include "WavetableBanks.h"
#include "WavetableOsc.h"

#include "dsp/Buffers.h"
#include "dsp/Filters.h"
#include "dsp/Math.h"
#include "dsp/Waveshaper.h"
#include "dsp/Wavetable.h"
#include "synth/Types.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace synth::voices {
using mod_matrix::ModDest;
using mod_matrix::ModDest2D;
using mod_matrix::ModRoute;
using mod_matrix::ModSrc;

namespace osc = wavetable::osc;
namespace env = envelope;

using dsp::buffers::StereoBuffer;

void initVoicePool(VoicePool& pool) {
  wavetable::banks::initFactoryBanks();

  // Initialization Stereo (balanced)
  for (uint8_t i = 0; i < MAX_VOICES; i++) {
    pool.panL[i] = 1.0f;
    pool.panR[i] = 1.0f;
  }

  // Initialize sustaion
  for (uint8_t i = 0; i < MAX_VOICES; i++)
    pool.sustain.notes[i] = false;

  // Initialize curve tables (empty to start)
  env::updateCurveTables(pool.ampEnv);
  env::updateCurveTables(pool.filterEnv);
  env::updateCurveTables(pool.modEnv);
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
  pool.deClickGain[voiceIndex] = 0.0f;

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

  const osc::PhaseMode phaseModes[NUM_OSCS] = {pool.osc1.phaseMode,
                                               pool.osc2.phaseMode,
                                               pool.osc3.phaseMode,
                                               pool.osc4.phaseMode};
  const float randomRanges[NUM_OSCS] = {
      pool.osc1.randomRange,
      pool.osc2.randomRange,
      pool.osc3.randomRange,
      pool.osc4.randomRange,
  };
  const float resetPhases[NUM_OSCS] = {
      pool.osc1.resetPhase,
      pool.osc2.resetPhase,
      pool.osc3.resetPhase,
      pool.osc4.resetPhase,
  };

  unison::initUnisonSubPhases(pool.unison, phaseModes, randomRanges, resetPhases, voiceIndex);

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

void releaseVoice(VoicePool& pool, uint8_t midiNote, float sampleRate) {
  uint32_t voiceIndex = pool.mono.enabled && pool.mono.voiceIndex < MAX_VOICES
                            ? pool.mono.voiceIndex
                            : findVoiceRelease(pool, midiNote);

  if (!isValidActiveIndex(voiceIndex))
    return;

  // Mono: revert to previous held note instead of releasing
  if (pool.mono.enabled && pool.mono.stackDepth > 0) {
    uint8_t prevNote = pool.mono.noteStack[pool.mono.stackDepth - 1];

    if (pool.porta.enabled) {
      float from = static_cast<float>(pool.midiNotes[voiceIndex]);
      float to = static_cast<float>(prevNote);
      pool.porta.offsets[voiceIndex] = (from - to) + pool.porta.offsets[voiceIndex];
      pool.porta.lastNote = prevNote;
    }

    redirectVoicePitch(pool, voiceIndex, prevNote, sampleRate);
    return; // don't release envs below
  }

  envelope::triggerRelease(pool.ampEnv, voiceIndex);
  envelope::triggerRelease(pool.filterEnv, voiceIndex);
  envelope::triggerRelease(pool.modEnv, voiceIndex);
}

void panicVoicePool(VoicePool& pool) {
  pool.activeCount = 0;
  for (uint32_t v = 0; v < MAX_VOICES; v++) {
    pool.activeIndices[v] = 0;
    pool.isActive[v] = 0;
    pool.sustain.notes[v] = false;
    pool.porta.offsets[v] = 0.0f;
  }
  pool.porta.lastNote = 0;

  for (uint8_t i = 0; i < mono::MAX_HELD_NOTES; i++) {
    pool.mono.heldNotes[i] = false;
  }
  pool.mono.stackDepth = 0;
  for (uint8_t i = 0; i < mono::MAX_NOTE_STACK; i++) {
    pool.mono.noteStack[i] = 0;
  }
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
    if (pool.lfo1.retrigger) {
      pool.lfo1.phase = 0.0f;
      pool.lfo1.delayTimer = pool.lfo1.delayCount;
      pool.lfo1.attackTimer = pool.lfo1.attackCount;
    }
    if (pool.lfo2.retrigger) {
      pool.lfo2.phase = 0.0f;
      pool.lfo2.delayTimer = pool.lfo2.delayCount;
      pool.lfo2.attackTimer = pool.lfo2.attackCount;
    }
    if (pool.lfo3.retrigger) {
      pool.lfo3.phase = 0.0f;
      pool.lfo3.delayTimer = pool.lfo3.delayCount;
      pool.lfo3.attackTimer = pool.lfo3.attackCount;
    }
  }

  addActiveIndex(pool, voiceIndex);
}

void handleNoteOff(VoicePool& pool, uint8_t midiNote, float sampleRate) {
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

  releaseVoice(pool, midiNote, sampleRate);
}

// ===========================
// Voice Processing
// ===========================

//=========================== <Anonymous Helpers> ===========================
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
    modSrcs[ModSrc::KeyTrack] = dsp::math::midiKeyTrackAmount(pool.midiNotes[v]);

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
void processLFOs(VoicePool& pool, float invSampleRate) {
  float effectiveRate1 = pool.lfo1.effectiveRate + pool.modMatrix.destValues[ModDest::LFO1Rate][0];
  float effectiveRate2 = pool.lfo2.effectiveRate + pool.modMatrix.destValues[ModDest::LFO2Rate][0];
  float effectiveRate3 = pool.lfo3.effectiveRate + pool.modMatrix.destValues[ModDest::LFO3Rate][0];

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

  pool.lfoModState.lfo1 = advanceLFO(pool.lfo1, invSampleRate, effectiveRate1, effectiveAmp1);
  pool.lfoModState.lfo2 = advanceLFO(pool.lfo2, invSampleRate, effectiveRate2, effectiveAmp2);
  pool.lfoModState.lfo3 = advanceLFO(pool.lfo3, invSampleRate, effectiveRate3, effectiveAmp3);
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
                   (matrix.destStepValues[dest][v] * static_cast<float>(s)) + lfoContribs[dest];

  if (osc.fixed) {
    // Fixed base: decoupled from keyboard pitch, portamento, and pitch bend.
    return osc.fixedPhaseInc * dsp::math::semitonesToFreqRatio(pitchMod);
  }

  pitchMod += semitoneBend + portaOffsets[v];

  return osc.phaseIncrements[v] * dsp::math::semitonesToFreqRatio(pitchMod) * osc.ratio;
}

float computeOscFMMod(osc::WavetableOsc& osc,
                      float globalScale,
                      uint32_t voiceIndex,
                      osc::FMSource src,
                      osc::WavetableOscModState& modState) {
  float fm = 0.0f;

  for (uint8_t r = 0; r < osc.fmRouteCount; r++) {
    float depth = osc::fmDepthCurve(osc.fmRoutes[r].depth) * globalScale;
    fm += osc::getFmInputValue(modState, voiceIndex, osc.fmRoutes[r].source, src) * depth;
  }
  return fm;
}

// Shared oscillator parameters — computed once, used by both mono and unison paths
struct OscParams {
  float pitchInc[4];
  float scan[4];
  float fm[4];
  float mix[4];
};

OscParams computeOscParams(VoicePool& pool, uint32_t v, uint32_t s) {
  using param::ranges::osc::clampScanPos;
  using std::max;
  using wavetable::osc::FMSource;

  auto& dest = pool.modMatrix.destValues;
  auto& lfo = pool.lfoModState.contribs;

  float semitoneBend = pool.pitchBend.value * pool.pitchBend.range;

  OscParams p;

  // Pitch
  p.pitchInc[0] = interpolatePitchInc(pool.osc1,
                                      pool.modMatrix,
                                      lfo,
                                      pool.porta.offsets,
                                      semitoneBend,
                                      ModDest::Osc1Pitch,
                                      v,
                                      s);
  p.pitchInc[1] = interpolatePitchInc(pool.osc2,
                                      pool.modMatrix,
                                      lfo,
                                      pool.porta.offsets,
                                      semitoneBend,
                                      ModDest::Osc2Pitch,
                                      v,
                                      s);
  p.pitchInc[2] = interpolatePitchInc(pool.osc3,
                                      pool.modMatrix,
                                      lfo,
                                      pool.porta.offsets,
                                      semitoneBend,
                                      ModDest::Osc3Pitch,
                                      v,
                                      s);
  p.pitchInc[3] = interpolatePitchInc(pool.osc4,
                                      pool.modMatrix,
                                      lfo,
                                      pool.porta.offsets,
                                      semitoneBend,
                                      ModDest::Osc4Pitch,
                                      v,
                                      s);

  // Scan
  p.scan[0] =
      clampScanPos(pool.osc1.scanPos + dest[ModDest::Osc1ScanPos][v] + lfo[ModDest::Osc1ScanPos]);
  p.scan[1] =
      clampScanPos(pool.osc2.scanPos + dest[ModDest::Osc2ScanPos][v] + lfo[ModDest::Osc2ScanPos]);
  p.scan[2] =
      clampScanPos(pool.osc3.scanPos + dest[ModDest::Osc3ScanPos][v] + lfo[ModDest::Osc3ScanPos]);
  p.scan[3] =
      clampScanPos(pool.osc4.scanPos + dest[ModDest::Osc4ScanPos][v] + lfo[ModDest::Osc4ScanPos]);

  // FM
  auto& oscModState = pool.oscModState;

  float fm1 = pool.osc1.fmDepthMod + dest[ModDest::Osc1FMDepth][v] + lfo[ModDest::Osc1FMDepth];
  float fm2 = pool.osc2.fmDepthMod + dest[ModDest::Osc2FMDepth][v] + lfo[ModDest::Osc2FMDepth];
  float fm3 = pool.osc3.fmDepthMod + dest[ModDest::Osc3FMDepth][v] + lfo[ModDest::Osc3FMDepth];
  float fm4 = pool.osc4.fmDepthMod + dest[ModDest::Osc4FMDepth][v] + lfo[ModDest::Osc4FMDepth];

  p.fm[0] = computeOscFMMod(pool.osc1, std::max(0.0f, fm1), v, FMSource::Osc1, oscModState);
  p.fm[1] = computeOscFMMod(pool.osc2, std::max(0.0f, fm2), v, FMSource::Osc2, oscModState);
  p.fm[2] = computeOscFMMod(pool.osc3, std::max(0.0f, fm3), v, FMSource::Osc3, oscModState);
  p.fm[3] = computeOscFMMod(pool.osc4, std::max(0.0f, fm4), v, FMSource::Osc4, oscModState);

  // Mix
  p.mix[0] = max(0.0f, pool.osc1.mixLevel + dest[ModDest::Osc1Mix][v] + lfo[ModDest::Osc1Mix]);
  p.mix[1] = max(0.0f, pool.osc2.mixLevel + dest[ModDest::Osc2Mix][v] + lfo[ModDest::Osc2Mix]);
  p.mix[2] = max(0.0f, pool.osc3.mixLevel + dest[ModDest::Osc3Mix][v] + lfo[ModDest::Osc3Mix]);
  p.mix[3] = max(0.0f, pool.osc4.mixLevel + dest[ModDest::Osc4Mix][v] + lfo[ModDest::Osc4Mix]);

  return p;
}

// ==== Non-unison (normal path) ====
void processOscs(VoicePool& pool, const OscParams& p, uint32_t v, float& outL, float& outR) {
  using dsp::wavetable::selectMipLevel;

  float mip0 = selectMipLevel(p.pitchInc[0] * dsp_wt::TABLE_SIZE_F);
  float mip1 = selectMipLevel(p.pitchInc[1] * dsp_wt::TABLE_SIZE_F);
  float mip2 = selectMipLevel(p.pitchInc[2] * dsp_wt::TABLE_SIZE_F);
  float mip3 = selectMipLevel(p.pitchInc[3] * dsp_wt::TABLE_SIZE_F);

  float o0 = osc::processOsc(pool.osc1, v, mip0, p.scan[0], p.fm[0], p.pitchInc[0]);
  float o1 = osc::processOsc(pool.osc2, v, mip1, p.scan[1], p.fm[1], p.pitchInc[1]);
  float o2 = osc::processOsc(pool.osc3, v, mip2, p.scan[2], p.fm[2], p.pitchInc[2]);
  float o3 = osc::processOsc(pool.osc4, v, mip3, p.scan[3], p.fm[3], p.pitchInc[3]);

  pool.oscModState.osc1[v] = o0;
  pool.oscModState.osc2[v] = o1;
  pool.oscModState.osc3[v] = o2;
  pool.oscModState.osc4[v] = o3;

  float noiseOut = noise::processNoise(pool.noise);
  float mono =
      (o0 * p.mix[0] + o1 * p.mix[1] + o2 * p.mix[2] + o3 * p.mix[3] + noiseOut) * pool.oscMixGain;
  outL = mono;
  outR = mono;
}

// ==== Unison ====
void processOscsUnison(VoicePool& pool,
                       const OscParams& p,
                       uint32_t v,
                       float& outL,
                       float& outR,
                       const bool active[4]) {
  using dsp::wavetable::selectMipLevel;

  auto& uni = pool.unison;
  const int8_t count = pool.unison.voices;

  // Osc refs for indexed access in the loop
  WavetableOsc* oscs[4] = {&pool.osc1, &pool.osc2, &pool.osc3, &pool.osc4};

  float sumL = 0.0f, sumR = 0.0f;
  float fmFb[4] = {};

  for (uint8_t u = 0; u < count; u++) {
    float detuneRatio = uni.detuneRatios[u];
    float outs[4];

    for (uint8_t o = 0; o < 4; o++) {
      float pitchInc = p.pitchInc[o] * detuneRatio;
      float mip = selectMipLevel(pitchInc * dsp_wt::TABLE_SIZE_F);
      float& phase = uni.subPhases[o][u][v];

      // DSP layer called directly — needs sub-voice phase refs.
      // processWavetableOsc advances phase internally.
      outs[o] =
          active[o]
              ? dsp_wt::processWavetableOsc(oscs[o]->bank, phase, mip, p.scan[o], p.fm[o], pitchInc)
              : 0.0f;
      fmFb[o] += outs[o];
    }

    float mixed = outs[0] * p.mix[0] + outs[1] * p.mix[1] + outs[2] * p.mix[2] + outs[3] * p.mix[3];
    sumL += mixed * uni.panGainsL[u];
    sumR += mixed * uni.panGainsR[u];
  }

  // FM feedback: averaged across sub-voices
  float invCount = 1.0f / static_cast<float>(count);
  pool.oscModState.osc1[v] = fmFb[0] * invCount;
  pool.oscModState.osc2[v] = fmFb[1] * invCount;
  pool.oscModState.osc3[v] = fmFb[2] * invCount;
  pool.oscModState.osc4[v] = fmFb[3] * invCount;

  float noiseOut = noise::processNoise(pool.noise);
  outL = (sumL + noiseOut * 0.5f) * pool.oscMixGain * uni.gainComp;
  outR = (sumR + noiseOut * 0.5f) * pool.oscMixGain * uni.gainComp;
}

// Process Oscillators with interpolation and mix (sum) values
void processAndMixOscs(VoicePool& pool,
                       float& outL,
                       float& outR,
                       const bool active[4],
                       uint32_t voiceIndex,
                       uint32_t sampleIndex) {
  OscParams p = computeOscParams(pool, voiceIndex, sampleIndex);

  if (pool.unison.enabled && pool.unison.voices > 1) {
    processOscsUnison(pool, p, voiceIndex, outL, outR, active);
  } else {
    processOscs(pool, p, voiceIndex, outL, outR);
  }
}

// Signal Chain processing
void processSignalChain(VoicePool& pool,
                        float& signalL,
                        float& signalR,
                        uint32_t voiceIndex,
                        float invSampleRate) {
  using dsp::filters::modulateCutoff;
  using param::ranges::mod::clampCutoffMod;

  auto& svf = pool.svf;
  auto& ladder = pool.ladder;

  auto& destValues = pool.modMatrix.destValues;
  auto& lfoContribs = pool.lfoModState.contribs;

  uint32_t v = voiceIndex;

  for (uint8_t i = 0; i < pool.signalChain.length; i++) {
    switch (pool.signalChain.slots[i]) {

    case SignalProcessor::SVF: {
      float cutoff = modulateCutoff(svf.cutoff,
                                    clampCutoffMod(destValues[ModDest::SVFCutoff][v] +
                                                   lfoContribs[ModDest::SVFCutoff]));
      float resonance =
          svf.resonance + destValues[ModDest::SVFResonance][v] + lfoContribs[ModDest::SVFResonance];
      filters::processSVFilter(svf, signalL, signalR, v, cutoff, resonance, invSampleRate);
      break;
    }

    case SignalProcessor::Ladder: {
      float cutoff = modulateCutoff(ladder.cutoff,
                                    clampCutoffMod(destValues[ModDest::LadderCutoff][v] +
                                                   lfoContribs[ModDest::LadderCutoff]));
      float resonance = ladder.resonance + destValues[ModDest::LadderResonance][v] +
                        lfoContribs[ModDest::LadderResonance];
      filters::processLadderFilter(ladder, signalL, signalR, v, cutoff, resonance, invSampleRate);
      break;
    }

    case SignalProcessor::Saturator:
      signalL = synth::saturator::processSaturator(pool.saturator, signalL);
      signalR = synth::saturator::processSaturator(pool.saturator, signalR);
      break;

    case SignalProcessor::None:
      break;
    }
  }
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

} // namespace
//=========================== </Anonymous Helpers> ===========================

void processVoices(VoicePool& pool, StereoBuffer output, size_t numSamples, float invSampleRate) {
  auto& lfoContribs = pool.lfoModState.contribs;

  const bool activeOscs[4] = {
      pool.osc1.enabled && pool.osc1.bank != nullptr,
      pool.osc2.enabled && pool.osc2.bank != nullptr,
      pool.osc3.enabled && pool.osc3.bank != nullptr,
      pool.osc4.enabled && pool.osc4.bank != nullptr,
  };

  // ==== Set and process Mod Matrix values (block-rate) ====
  preProcessBlock(pool, numSamples);

  // ==== Calculate each sample value (audio-rate) ====
  for (uint32_t sIndex = 0; sIndex < numSamples; sIndex++) {
    float sampleL = 0.0f;
    float sampleR = 0.0f;

    processLFOs(pool, invSampleRate);

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

      float oscL = 0.0f;
      float oscR = 0.0f;
      processAndMixOscs(pool, oscL, oscR, activeOscs, vIndex, sIndex);
      processSignalChain(pool, oscL, oscR, vIndex, invSampleRate);

      float ampEnv = envelope::processEnvelope(pool.ampEnv, vIndex);

      // Check if amplitude envelope completed - remove immediately
      if (pool.ampEnv.states[vIndex] == envelope::EnvelopeStatus::Idle) {
        removeInactiveIndex(pool, vIndex);
        // No index adjustment needed - iterating backwards
      }

      pool.deClickGain[vIndex] = std::min(1.0f, pool.deClickGain[vIndex] + DE_CLICK_INCREMENT);
      float gain = ampEnv * pool.velocities[vIndex] * VOICE_GAIN * pool.deClickGain[vIndex];

      sampleL += oscL * gain * pool.panL[vIndex];
      sampleR += oscR * gain * pool.panR[vIndex];
    }

    // TODO(nico): Basic soft clip for now.
    // Mainly for protection and not as an effect
    output.left[sIndex] = dsp::waveshaper::softLimit(sampleL * pool.masterGain);
    output.right[sIndex] = dsp::waveshaper::softLimit(sampleR * pool.masterGain);
  }

  // Advance pitch interpolation state for next block
  postProcessBlock(pool);
}

} // namespace synth::voices
