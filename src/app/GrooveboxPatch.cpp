#include "app/GrooveboxPatch.h"

namespace app {

void resetGrooveboxPatch(GrooveboxPatch* patch) {
  *patch = GrooveboxPatch{};
}

bool hasStepNotePatchEdits(const StepNotePatch& patch) {
  return patch.op == PatchObjectOp::Clear || patch.op == PatchObjectOp::Replace ||
         patch.hasNoteOn || patch.hasTie || patch.hasGate || patch.hasNote || patch.hasVelocity;
}

bool hasStepPatchEdits(const StepEventPatch& patch) {
  if (patch.op == PatchObjectOp::Clear || patch.op == PatchObjectOp::Replace || patch.hasActive ||
      patch.hasNoteCount || patch.locks.op != PatchObjectOp::None) {
    return true;
  }

  for (uint8_t i = 0; i < sequencer::MAX_NOTES_PER_STEP; ++i) {
    if (patch.hasNotePatch[i] && hasStepNotePatchEdits(patch.notes[i]))
      return true;
  }
  return false;
}

bool hasPatternPatchEdits(const PatternPatch& patch) {
  if (patch.op == PatchObjectOp::Clear || patch.op == PatchObjectOp::Replace || patch.hasNumSteps ||
      patch.hasStepsPerBeat) {
    return true;
  }

  for (uint8_t step = 0; step < sequencer::MAX_PATTERN_STEPS; ++step) {
    if (patch.hasStep[step] && hasStepPatchEdits(patch.steps[step]))
      return true;
  }

  return false;
}

bool hasTrackSequencerPatchEdits(const TrackSequencerPatch& patch) {
  if (patch.bankOp == PatchObjectOp::Clear || patch.bankOp == PatchObjectOp::Replace ||
      patch.hasActiveSlot) {
    return true;
  }

  for (uint8_t slot = 0; slot < sequencer::PATTERNS_PER_LANE; ++slot) {
    if (patch.slots[slot].op == PatchObjectOp::Clear ||
        patch.slots[slot].op == PatchObjectOp::Replace ||
        hasPatternPatchEdits(patch.slots[slot].pattern)) {
      return true;
    }
  }

  return false;
}

bool hasGrooveboxPatchEdits(const GrooveboxPatch& patch) {
  if (patch.hasMixer && patch.mixer.writeCount > 0)
    return true;

  for (uint8_t track = 0; track < MAX_TRACKS; ++track) {
    if (patch.hasSynth[track] && patch.synth[track].writeCount > 0)
      return true;
  }

  if (!patch.hasSequencer)
    return false;

  for (uint8_t track = 0; track < MAX_TRACKS; ++track) {
    if (patch.sequencer.hasTrack[track] &&
        hasTrackSequencerPatchEdits(patch.sequencer.tracks[track]))
      return true;
  }

  return false;
}

} // namespace app
