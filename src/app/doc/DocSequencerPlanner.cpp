#include "app/doc/DocSequencerPlanner.h"

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

bool lanePatternsEqual(const seq::LanePattern& a, const seq::LanePattern& b) {
  if (a.numSteps != b.numSteps || a.stepsPerBeat != b.stepsPerBeat)
    return false;

  for (uint8_t step = 0; step < a.numSteps; ++step) {
    if (!stepEventsEqual(a.steps[step], b.steps[step]))
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
      bank.slots[slot].pattern = track.patterns[slot].pattern;
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

PlannedSequencerApply planSequencerApply(const AuthoredSeqDocModel& nextModel,
                                         const AuthoredSeqDocModel* previousAdmittedModel) {
  PlannedSequencerApply result{};
  result.ok = true;

  for (uint8_t trackIndex = 0; trackIndex < MAX_TRACKS; ++trackIndex) {
    const bool nextPresent = nextModel.hasTrackState[trackIndex];
    const bool previousPresent =
        previousAdmittedModel && previousAdmittedModel->hasTrackState[trackIndex];

    if (!nextPresent)
      continue;

    if (previousPresent && authoredTrackBanksEqual(nextModel.tracks[trackIndex],
                                                   previousAdmittedModel->tracks[trackIndex]))
      continue;

    PlannedSequencerTrackOp op{};
    op.trackIndex = trackIndex;
    op.bank = buildPatternBank(nextModel.tracks[trackIndex]);
    result.trackOps.push_back(op);
  }

  return result;
}

} // namespace app::doc
