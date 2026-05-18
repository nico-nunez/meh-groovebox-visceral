#pragma once

#include "app/doc/DocAuthoredModel.h"
#include "app/doc/DocDiagnostics.h"
#include "app/doc/DocSequencerModel.h"

#include <cstdint>

namespace app::doc {

struct PlannedSequencerTrackOp {
  uint8_t trackIndex = 0;
  sequencer::PatternBank bank{};
};

struct SequencerTargetResult {
  bool ok = false;
  DocDiagnostics diagnostics{};
  sequencer::PatternSnapshot snapshot{};
};

SequencerTargetResult buildSequencerTargetSnapshot(const AuthoredSeqDocModel* model,
                                                   DocID documentID,
                                                   DocRevision revision,
                                                   sequencer::PatternSnapshot* out);

void buildAdmittedSeqTargetModel(const AuthoredDocModel* nextModel,
                                 AuthoredDocModel* admitted,
                                 PatternArena* admittedArena);
} // namespace app::doc
