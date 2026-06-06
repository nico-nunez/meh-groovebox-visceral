#pragma once

#include "app/doc/DocMetadata.h"
#include "synth/params/ParamDefs.h"

namespace app::doc {
namespace sp = synth::param;

struct AuthoredSynthParamField {
  const char* authoredPath = "";
  const char* canonicalParam = "";
  sp::ParamID paramID = sp::PARAM_UNKNOWN;
  DocLuaValueKind valueKind = DocLuaValueKind::Any;
  const char* doc = "";
};

DocMetadataSpan<AuthoredSynthParamField> authoredSynthParamFields();

const AuthoredSynthParamField* findAuthoredSynthParamField(const char* authoredPath);

constexpr DocLuaValueKind synthParamTypeToLuaKind(sp::ParamType type) {
  switch (type) {
  case sp::ParamType::Float:
    return DocLuaValueKind::Number;

  case sp::ParamType::Int8:
    return DocLuaValueKind::Integer;

  case sp::ParamType::Bool:
    return DocLuaValueKind::Boolean;

  case sp::ParamType::OscBankID:
  case sp::ParamType::NoiseType:
  case sp::ParamType::FilterMode:
  case sp::ParamType::DistortionType:
  case sp::ParamType::PhaseMode:
  case sp::ParamType::Subdivision:
    return DocLuaValueKind::String;

  default:
    return DocLuaValueKind::Any;
  }
}

} // namespace app::doc
