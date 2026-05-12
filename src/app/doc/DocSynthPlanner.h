#pragma once

#include "app/doc/DocAuthoredModel.h"
#include "app/doc/DocDiagnostics.h"

#include <cstdint>
#include <vector>

namespace app::doc {

struct PlannedSynthParamOp {
  uint8_t trackIndex = 0;
  synth::param::ParamID paramID = synth::param::PARAM_UNKNOWN;
  float value = 0.0f;
  const AuthoredSynthParamField* field = nullptr;
  SourceSpan span{};
};

struct PlannedSynthApply {
  bool ok = false;
  DocDiagnostics diagnostics{};
  std::vector<PlannedSynthParamOp> paramOps{};
};

void planSynthApply(const AuthoredDocModel* nextModel,
                    const AuthoredDocModel* previousAdmittedModel,
                    PlannedSynthApply* synthPlan);

void buildAdmittedSynthTargetModel(const AuthoredDocModel* nextModel, AuthoredDocModel* admitted);

} // namespace app::doc
