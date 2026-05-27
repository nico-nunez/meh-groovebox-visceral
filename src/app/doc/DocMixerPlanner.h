#pragma once

#include "app/GrooveboxPatch.h"
#include "app/doc/DocAuthoredModel.h"
#include "app/doc/DocDiagnostics.h"

#include <cstdint>

namespace app::doc {

struct PlannedMixerParamOp {
  uint8_t trackIndex = 0;
  app::params::AppParamID paramID = app::params::AppParamID::Count;
  float value = 0.0f;
  const AuthoredMixerParamField* field = nullptr;
  SourceSpan span{};
};

struct MixerTargetResult {
  bool ok = false;
  DocDiagnostics diagnostics{};
};

MixerTargetResult buildMixerPatch(const AuthoredDocModel* model,
                                  DocID documentID,
                                  DocRevision revision,
                                  app::MixerPatch* out);

void buildAdmittedMixerTargetModel(const AuthoredDocModel* nextModel, AuthoredDocModel* admitted);

} // namespace app::doc
