#pragma once

#include "DocMetadata.h"
#include "synth/params/ParamDefs.h"

namespace app::doc {

struct AuthoredSynthParamField {
  const char* authoredPath = "";
  const char* canonicalParam = "";
  synth::param::ParamID paramID = synth::param::PARAM_UNKNOWN;
  DocLuaValueKind valueKind = DocLuaValueKind::Any;
  const char* doc = "";
};

DocMetadataSpan<AuthoredSynthParamField> authoredSynthParamFields();

const AuthoredSynthParamField* findAuthoredSynthParamField(const char* authoredPath);
const AuthoredSynthParamField*
findAuthoredSynthParamFieldByCanonicalParam(const char* canonicalParam);

DocLuaValueKind authoredSynthValueKindForParamType(synth::param::ParamType type);

} // namespace app::doc
