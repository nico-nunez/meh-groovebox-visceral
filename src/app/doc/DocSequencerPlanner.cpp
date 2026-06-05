#include "app/doc/DocSequencerPlanner.h"

#include "app/Sequencer.h"
#include "app/doc/DocSequencerModel.h"
#include "app/doc/metadata/DocMetadata.h"

#include <cstdint>

namespace app::doc {

namespace {

void copyStepNotePatch(const AuthoredStepNotePatch& src, app::StepNotePatch* dst) {
  dst->op = src.op;
  dst->hasNoteOn = src.hasNoteOn;
  dst->noteOn = src.noteOn;
  dst->hasTie = src.hasTie;
  dst->tie = src.tie;
  dst->hasGate = src.hasGate;
  dst->gate = src.gate;
  dst->hasNote = src.hasNote;
  dst->note = src.note;
  dst->hasVelocity = src.hasVelocity;
  dst->velocity = src.velocity;
}

void copyStepPatch(const AuthoredStepPatch& src, app::StepEventPatch* dst) {
  dst->op = src.op;
  dst->hasActive = src.hasActive;
  dst->active = src.active;
  dst->hasNoteCount = src.hasNoteCount;
  dst->noteCount = src.noteCount;
  for (uint8_t i = 0; i < sequencer::MAX_NOTES_PER_STEP; ++i) {
    if (!src.hasNotePatch[i])
      continue;
    dst->hasNotePatch[i] = true;
    copyStepNotePatch(src.notes[i], &dst->notes[i]);
  }

  dst->locks.op = src.locks.op;
  dst->locks.lockCount = src.locks.numLocks;
  for (uint8_t i = 0; i < src.locks.numLocks; ++i)
    dst->locks.locks[i] = src.locks.locks[i];
}

void copyPatternPatch(const AuthoredPatternPatch& src, app::PatternPatch* dst) {
  dst->op = src.op;
  dst->hasNumSteps = src.hasNumSteps;
  dst->numSteps = src.numSteps;
  dst->hasStepsPerBeat = src.hasStepsPerBeat;
  dst->stepsPerBeat = src.stepsPerBeat;

  for (uint8_t step = 0; step < sequencer::MAX_PATTERN_STEPS; ++step) {
    if (!src.hasStep[step] || !hasAuthoredStepPatchEdits(src.steps[step]))
      continue;
    dst->hasStep[step] = true;
    copyStepPatch(src.steps[step], &dst->steps[step]);
  }
}

DocDiagnostic makeSequencerTargetDiagnostic(DocID documentID,
                                            DocRevision revision,
                                            const AuthoredTrackSeqModel* track,
                                            const char* message) {
  DocDiagnostic d{};
  d.severity = DiagnosticSeverity::Error;
  d.source = DiagnosticSource::Planner;
  d.documentID = documentID;
  d.revision = revision;
  d.code = docdiag::SequencerPlanningFailed;
  d.message = message ? message : "sequencer target build failed";
  if (track) {
    d.span = track->patternsSpan;
    d.relatedTarget =
        "track:" + std::to_string(static_cast<int>(track->trackIndex) + 1) + ".patterns";
  }
  return d;
}

} // namespace

SequencerTargetResult buildSequencerPatch(const AuthoredSeqDocModel* model,
                                          DocID documentID,
                                          DocRevision revision,
                                          app::SequencerPatch* out) {
  SequencerTargetResult result{};
  if (!model || !out) {
    result.diagnostics.push_back(makeSequencerTargetDiagnostic(documentID,
                                                               revision,
                                                               nullptr,
                                                               "null sequencer patch output"));
    return result;
  }

  *out = app::SequencerPatch{};

  for (uint8_t trackIndex = 0; trackIndex < app::MAX_TRACKS; ++trackIndex) {
    if (!model->hasTrackState[trackIndex])
      continue;

    const AuthoredTrackSeqModel& src = model->tracks[trackIndex];
    app::TrackSequencerPatch& dst = out->tracks[trackIndex];
    out->hasTrack[trackIndex] = true;

    dst.bankOp = src.patternBankOp;
    dst.hasActiveSlot = src.hasActiveSlot;
    dst.activeSlot = src.activeSlot;

    for (uint8_t slot = 0; slot < sequencer::PATTERNS_PER_LANE; ++slot) {
      if (!src.hasPatternSlot[slot])
        continue;

      dst.hasSlot[slot] = true;
      dst.slots[slot].op = src.patternSlots[slot].op;
      copyPatternPatch(src.patternSlots[slot].pattern, &dst.slots[slot].pattern);
    }
  }

  result.ok = true;
  return result;
}

void buildAdmittedSeqTargetModel(const AuthoredDocModel* nextModel, AuthoredDocModel* admitted) {
  for (uint8_t track = 0; track < app::MAX_TRACKS; ++track) {
    if (!nextModel->sequencer.hasTrackState[track])
      continue;

    admitted->sequencer.hasTrackState[track] = true;
    admitted->sequencer.tracks[track] = nextModel->sequencer.tracks[track];
  }
}

} // namespace app::doc
