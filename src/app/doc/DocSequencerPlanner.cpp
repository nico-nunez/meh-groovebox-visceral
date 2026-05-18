#include "app/doc/DocSequencerPlanner.h"

#include "app/Sequencer.h"
#include "app/doc/DocMetadata.h"
#include "app/doc/DocSequencerModel.h"

#include <cstdint>

namespace app::doc {

namespace {
namespace seq = app::sequencer;

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

SequencerTargetResult buildSequencerTargetSnapshot(const AuthoredSeqDocModel* model,
                                                   DocID documentID,
                                                   DocRevision revision,
                                                   sequencer::PatternSnapshot* out) {
  SequencerTargetResult result{};
  if (!model || !out) {
    const char* errMsg = !model ? "null sequencer model" : "null sequencer target output";
    result.diagnostics.push_back(
        makeSequencerTargetDiagnostic(documentID, revision, nullptr, errMsg));
    return result;
  }

  *out = sequencer::PatternSnapshot{};

  for (uint8_t trackIndex = 0; trackIndex < app::MAX_TRACKS; ++trackIndex) {
    if (!model->hasTrackState[trackIndex])
      continue;

    const AuthoredTrackSeqModel& track = model->tracks[trackIndex];
    out->lanes[trackIndex] = buildPatternBank(track);
  }

  result.ok = true;
  return result;
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
