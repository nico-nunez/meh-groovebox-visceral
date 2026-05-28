#pragma once

#include "DocMetadata.h"
#include "app/AppParams.h"

namespace app::doc {

struct AuthoredMixerParamField {
  const char* authoredField = "";
  app::params::AppParamID paramID = app::params::AppParamID::Count;
  DocLuaValueKind valueKind = DocLuaValueKind::Any;
  const char* doc = "";
};

DocMetadataSpan<AuthoredMixerParamField> authoredTrackMixerParamFields();

const AuthoredMixerParamField* findAuthoredTrackMixerParamField(const char* field);

DocLuaValueKind authoredMixerValueKindForParamType(app::params::AppParamType type);

} // namespace app::doc
