#pragma once

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

PlannedSequencerApply planSequencerApply(const AuthoredSeqDocModel& nextModel,
                                         const AuthoredSeqDocModel* previousAdmittedModel);

} // namespace app::doc
