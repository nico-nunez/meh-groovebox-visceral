#pragma once

#include "app/AppParams.h"
#include "app/Constants.h"
#include "app/Sequencer.h"

#include "synth/params/ParamDefs.h"

#include <cstdint>

namespace app {

enum class PatchObjectOp : uint8_t {
  None,
  Patch,
  Clear,
  Replace,
};

struct SynthParamPatchWrite {
  synth::param::ParamID paramID = synth::param::PARAM_UNKNOWN;
  float value = 0.0f;
};

inline constexpr uint16_t MAX_SYNTH_PARAM_PATCH_WRITES = synth::param::PARAM_COUNT;

struct TrackSynthPatch {
  SynthParamPatchWrite writes[MAX_SYNTH_PARAM_PATCH_WRITES]{};
  uint16_t writeCount = 0;
};

struct MixerParamPatchWrite {
  app::params::AppParamID paramID = app::params::AppParamID::Count;
  uint8_t trackIndex = 0;
  float value = 0.0f;
};

inline constexpr uint16_t MAX_MIXER_PARAM_PATCH_WRITES = 64;

struct MixerPatch {
  MixerParamPatchWrite writes[MAX_MIXER_PARAM_PATCH_WRITES]{};
  uint16_t writeCount = 0;
};

struct StepLockPatch {
  PatchObjectOp op = PatchObjectOp::None;
  sequencer::ParamLock locks[sequencer::MAX_LOCKS_PER_STEP]{};
  uint8_t lockCount = 0;
};

struct StepEventPatch {
  PatchObjectOp op = PatchObjectOp::None;

  bool hasActive = false;
  bool active = false;

  bool hasNoteOn = false;
  bool noteOn = false;

  bool hasLegato = false;
  bool legato = false;

  bool hasGate = false;
  float gate = 0.0f;

  bool hasNote = false;
  uint8_t note = 0;

  bool hasVelocity = false;
  uint8_t velocity = 0;

  StepLockPatch locks{};
};

struct PatternPatch {
  PatchObjectOp op = PatchObjectOp::None;

  bool hasNumSteps = false;
  uint8_t numSteps = 0;

  bool hasStepsPerBeat = false;
  uint8_t stepsPerBeat = 0;

  StepEventPatch steps[sequencer::MAX_PATTERN_STEPS]{};
  bool hasStep[sequencer::MAX_PATTERN_STEPS]{};
};

struct PatternSlotPatch {
  PatchObjectOp op = PatchObjectOp::None;
  PatternPatch pattern{};
};

struct TrackSequencerPatch {
  PatchObjectOp bankOp = PatchObjectOp::None;

  bool hasActiveSlot = false;
  uint8_t activeSlot = sequencer::INVALID_PATTERN_SLOT;

  PatternSlotPatch slots[sequencer::PATTERNS_PER_LANE]{};
  bool hasSlot[sequencer::PATTERNS_PER_LANE]{};
};

struct SequencerPatch {
  TrackSequencerPatch tracks[MAX_TRACKS]{};
  bool hasTrack[MAX_TRACKS]{};
};

struct GrooveboxPatch {
  TrackSynthPatch synth[MAX_TRACKS]{};
  bool hasSynth[MAX_TRACKS]{};

  MixerPatch mixer{};
  bool hasMixer = false;

  SequencerPatch sequencer{};
  bool hasSequencer = false;
};

void resetGrooveboxPatch(GrooveboxPatch* patch);

bool hasStepPatchEdits(const StepEventPatch& patch);
bool hasPatternPatchEdits(const PatternPatch& patch);
bool hasTrackSequencerPatchEdits(const TrackSequencerPatch& patch);
bool hasGrooveboxPatchEdits(const GrooveboxPatch& patch);

} // namespace app
