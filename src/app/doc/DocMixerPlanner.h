#pragma once

#include "app/doc/DocAuthoredModel.h"
#include "app/doc/DocDiagnostics.h"

#include <cstdint>
#include <vector>

namespace app::doc {

struct PlannedMixerParamOp {
  uint8_t trackIndex = 0;
  app::params::AppParamID paramID = app::params::AppParamID::Count;
  float value = 0.0f;
  const AuthoredMixerParamField* field = nullptr;
  SourceSpan span{};
};

struct PlannedMixerApply {
  bool ok = false;
  DocDiagnostics diagnostics{};
  std::vector<PlannedMixerParamOp> paramOps{};
};

void planMixerApply(const AuthoredDocModel* nextModel,
                    const AuthoredDocModel* previousAdmittedModel,
                    PlannedMixerApply* mixerPlan);

void buildAdmittedMixerTargetModel(const AuthoredDocModel* nextModel, AuthoredDocModel* admitted);

} // namespace app::doc
