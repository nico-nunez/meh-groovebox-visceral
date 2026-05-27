#pragma once

#include "app/GrooveboxPatch.h"
#include "app/Sequencer.h"
#include "app/doc/DocTypes.h"

namespace app::doc {

struct PatternArena {
  sequencer::LanePattern pool[app::MAX_TRACKS][sequencer::PATTERNS_PER_LANE]{};

  sequencer::LanePattern* get(uint8_t track, uint8_t slot) { return &pool[track][slot]; }
  const sequencer::LanePattern* get(uint8_t track, uint8_t slot) const {
    return &pool[track][slot];
  }
};

struct AuthoredStepLocksPatch {
  app::PatchObjectOp op = app::PatchObjectOp::None;
  sequencer::ParamLock locks[sequencer::MAX_LOCKS_PER_STEP]{};
  uint8_t numLocks = 0;
  SourceSpan span{};
};

struct AuthoredStepPatch {
  app::PatchObjectOp op = app::PatchObjectOp::None;
  SourceSpan span{};

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

  AuthoredStepLocksPatch locks{};
};

struct AuthoredPatternPatch {
  app::PatchObjectOp op = app::PatchObjectOp::None;
  SourceSpan span{};

  bool hasNumSteps = false;
  uint8_t numSteps = 0;

  bool hasStepsPerBeat = false;
  uint8_t stepsPerBeat = 0;

  bool hasStep[sequencer::MAX_PATTERN_STEPS]{};
  AuthoredStepPatch steps[sequencer::MAX_PATTERN_STEPS]{};
};

struct AuthoredPatternSlotPatch {
  app::PatchObjectOp op = app::PatchObjectOp::None;
  SourceSpan span{};
  AuthoredPatternPatch pattern{};
};

struct AuthoredTrackSeqModel {
  bool hasSequencerPatch = false;

  app::PatchObjectOp patternBankOp = app::PatchObjectOp::None;
  SourceSpan patternsSpan{};

  bool hasPatternSlot[sequencer::PATTERNS_PER_LANE]{};
  AuthoredPatternSlotPatch patternSlots[sequencer::PATTERNS_PER_LANE]{};

  bool hasActiveSlot = false;
  uint8_t activeSlot = sequencer::INVALID_PATTERN_SLOT;
  SourceSpan activeSlotSpan{};

  SourceSpan trackSpan{};
  uint8_t trackIndex = 0;
  ActivePatternSlotSource activeSlotSource = ActivePatternSlotSource::Unset;
};

struct AuthoredSeqDocModel {
  DocID documentID = 0;
  DocRevision revision = 0;

  // True only when the authored document contains sequencer edits for the track.
  bool hasTrackState[app::MAX_TRACKS]{};
  AuthoredTrackSeqModel tracks[app::MAX_TRACKS]{};
};

inline bool hasAuthoredStepPatchEdits(const AuthoredStepPatch& patch) {
  return patch.op == app::PatchObjectOp::Clear || patch.op == app::PatchObjectOp::Replace ||
         patch.hasActive || patch.hasNoteOn || patch.hasLegato || patch.hasGate || patch.hasNote ||
         patch.hasVelocity || patch.locks.op != app::PatchObjectOp::None;
}

inline bool hasAuthoredPatternPatchEdits(const AuthoredPatternPatch& patch) {
  if (patch.op == app::PatchObjectOp::Clear || patch.op == app::PatchObjectOp::Replace ||
      patch.hasNumSteps || patch.hasStepsPerBeat) {
    return true;
  }

  for (uint8_t step = 0; step < sequencer::MAX_PATTERN_STEPS; ++step) {
    if (patch.hasStep[step] && hasAuthoredStepPatchEdits(patch.steps[step]))
      return true;
  }

  return false;
}

inline bool hasAuthoredTrackSeqPatchEdits(const AuthoredTrackSeqModel& track) {
  if (track.patternBankOp == app::PatchObjectOp::Clear ||
      track.patternBankOp == app::PatchObjectOp::Replace || track.hasActiveSlot) {
    return true;
  }

  for (uint8_t slot = 0; slot < sequencer::PATTERNS_PER_LANE; ++slot) {
    if (track.hasPatternSlot[slot])
      return true;
  }

  return false;
}

} // namespace app::doc
