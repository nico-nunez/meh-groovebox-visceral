#pragma once

#include "app/doc/DocAuthoredModel.h"
#include "app/doc/DocDiagnostics.h"
#include "synth/params/ParamDefs.h"
#include "synth/program/SynthProgram.h"

#include <cstdint>

namespace app::doc {

namespace {
using synth::param::ParamID;
using synth::program::SynthProgram;
} // namespace

struct PlannedSynthParamOp {
  uint8_t trackIndex = 0;
  ParamID paramID = ParamID::PARAM_UNKNOWN;
  float value = 0.0f;
  const AuthoredSynthParamField* field = nullptr;
  SourceSpan span{};
};

struct SynthTargetProgramsResult {
  bool ok = false;
  DocDiagnostics diagnostics{};
};

SynthTargetProgramsResult buildSynthTargetPrograms(const AuthoredDocModel* model,
                                                   DocID documentID,
                                                   DocRevision revision,
                                                   SynthProgram* out);

void buildAdmittedSynthTargetModel(const AuthoredDocModel* nextModel, AuthoredDocModel* admitted);

} // namespace app::doc
