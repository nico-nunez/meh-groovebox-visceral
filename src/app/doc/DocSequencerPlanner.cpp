#include "app/doc/DocSequencerPlanner.h"
#include "app/doc/DocSequencerModel.h"

#include <cstdint>

namespace app::doc {

namespace {
namespace seq = app::sequencer;

bool stepEventsEqual(const seq::StepEvent& a, const seq::StepEvent& b) {
  if (a.active != b.active || a.noteOn != b.noteOn || a.legato != b.legato || a.gate != b.gate ||
      a.note != b.note || a.velocity != b.velocity || a.numLocks != b.numLocks)
    return false;

  for (uint8_t i = 0; i < a.numLocks; ++i) {
    if (a.locks[i].paramID != b.locks[i].paramID || a.locks[i].value != b.locks[i].value)
      return false;
  }

  return true;
}

bool lanePatternsEqual(const seq::LanePattern* a, const seq::LanePattern* b) {
  if (a->numSteps != b->numSteps || a->stepsPerBeat != b->stepsPerBeat)
    return false;

  for (uint8_t step = 0; step < a->numSteps; ++step) {
    if (!stepEventsEqual(a->steps[step], b->steps[step]))
      return false;
  }

  return true;
}

seq::PatternBank buildPatternBank(const AuthoredTrackSeqModel& track) {
  seq::PatternBank bank{};
  bank.activeSlot = track.activeSlot;

  for (uint8_t slot = 0; slot < sequencer::PATTERNS_PER_LANE; ++slot) {
    bank.slots[slot].occupied = track.patterns[slot].occupied;
    if (track.patterns[slot].occupied)
      bank.slots[slot].pattern = *track.patterns[slot].pattern;
  }

  return bank;
}

bool authoredTrackBanksEqual(const AuthoredTrackSeqModel& a, const AuthoredTrackSeqModel& b) {
  if (a.activeSlot != b.activeSlot)
    return false;

  for (uint8_t slot = 0; slot < seq::PATTERNS_PER_LANE; ++slot) {
    if (a.patterns[slot].occupied != b.patterns[slot].occupied)
      return false;

    if (a.patterns[slot].occupied &&
        !lanePatternsEqual(a.patterns[slot].pattern, b.patterns[slot].pattern))
      return false;
  }
  return true;
}

} // namespace

void planSequencerApply(const AuthoredSeqDocModel* nextModel,
                        const AuthoredSeqDocModel* previousAdmittedModel,
                        PlannedSequencerApply* seqPlan) {
  for (uint8_t trackIndex = 0; trackIndex < MAX_TRACKS; ++trackIndex) {
    const bool nextPresent = nextModel->hasTrackState[trackIndex];
    const bool previousPresent =
        previousAdmittedModel && previousAdmittedModel->hasTrackState[trackIndex];

    if (!nextPresent)
      continue;

    if (previousPresent && authoredTrackBanksEqual(nextModel->tracks[trackIndex],
                                                   previousAdmittedModel->tracks[trackIndex]))
      continue;

    PlannedSequencerTrackOp op{};
    op.trackIndex = trackIndex;
    op.bank = buildPatternBank(nextModel->tracks[trackIndex]);
    seqPlan->trackOps.push_back(op);
  }
  seqPlan->ok = true;
}

void buildAdmittedSeqTargetModel(const AuthoredDocModel* nextModel,
                                 AuthoredDocModel* admitted,
                                 PatternArena* admittedArena) {
  // Sequencer: for each track in nextModel, copy metadata fields and pattern
  // data from scratchArena into admittedArena. Pattern pointers in dst are
  // written exactly once, directly to their admittedArena address — never
  // to scratchArena. Tracks absent from nextModel are not touched; their
  // previous data in lastAdmittedDocModel is retained as-is (carry-forward).
  for (uint8_t t = 0; t < app::MAX_TRACKS; ++t) {
    if (!nextModel->sequencer.hasTrackState[t])
      continue;
    admitted->sequencer.hasTrackState[t] = true;
    const AuthoredTrackSeqModel& src = nextModel->sequencer.tracks[t];
    AuthoredTrackSeqModel& dst = admitted->sequencer.tracks[t];
    dst.activeSlot = src.activeSlot;
    dst.patternsSpan = src.patternsSpan;
    dst.activeSlotSpan = src.activeSlotSpan;
    dst.trackSpan = src.trackSpan;
    dst.trackIndex = src.trackIndex;
    dst.activeSlotSource = src.activeSlotSource;
    dst.explicitlyAuthoredEmpty = src.explicitlyAuthoredEmpty;
    for (uint8_t s = 0; s < sequencer::PATTERNS_PER_LANE; ++s) {
      dst.patterns[s].occupied = src.patterns[s].occupied;
      dst.patterns[s].slotSpan = src.patterns[s].slotSpan;
      if (src.patterns[s].occupied && src.patterns[s].pattern) {
        *admittedArena->get(t, s) = *src.patterns[s].pattern;
        dst.patterns[s].pattern = admittedArena->get(t, s);
      } else {
        dst.patterns[s].pattern = nullptr;
      }
    }
  }
}

} // namespace app::doc
