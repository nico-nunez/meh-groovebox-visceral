#pragma once

#include "app/doc/DocAuthoredModel.h"
#include "app/doc/DocDiagnostics.h"
#include "app/doc/DocSequencerModel.h"

#include <cstdint>
#include <vector>

namespace app::doc {

struct PlannedSequencerTrackOp {
  uint8_t trackIndex = 0;
  sequencer::PatternBank bank{};
};

struct PlannedSequencerApply {
  bool ok = false;
  DocDiagnostics diagnostics{};
  std::vector<PlannedSequencerTrackOp> trackOps{};
};

void planSequencerApply(const AuthoredSeqDocModel* nextModel,
                        const AuthoredSeqDocModel* previousAdmittedModel,
                        PlannedSequencerApply* seqPlan);

void buildAdmittedSeqTargetModel(const AuthoredDocModel* nextModel,
                                 AuthoredDocModel* admitted,
                                 PatternArena* admittedArena);
} // namespace app::doc
