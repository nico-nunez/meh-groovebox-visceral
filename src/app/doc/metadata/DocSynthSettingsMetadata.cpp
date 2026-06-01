#include "DocSynthSettingsMetadata.h"
#include "app/doc/metadata/DocMetadata.h"
#include "synth/params/ParamDefs.h"

#include <cstdio>
#include <cstring>

namespace app::doc {
namespace {

namespace sp = synth::param;

template <typename T, std::size_t N> constexpr DocMetadataSpan<T> spanOf(const T (&items)[N]) {
  return {items, N};
}

bool equals(const char* a, const char* b) {
  return a && b && std::strcmp(a, b) == 0;
}

constexpr DocLuaValueKind paramTypeToLuaKind(sp::ParamType type) {
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

constexpr AuthoredSynthParamField kAuthoredSynthParamFields[] = {
#define X(id, root, field, type, min, max, def, group)                                             \
  {PUBLIC_PARAM_STR(root, field),                                                                  \
   PUBLIC_PARAM_STR(root, field),                                                                  \
   synth::param::id,                                                                               \
   paramTypeToLuaKind(sp::ParamType::type)},
    PARAM_LIST
#undef X
};

} // namespace

DocMetadataSpan<AuthoredSynthParamField> authoredSynthParamFields() {
  return spanOf(kAuthoredSynthParamFields);
}

const AuthoredSynthParamField* findAuthoredSynthParamField(const char* authoredPath) {
  for (const auto& field : authoredSynthParamFields()) {
    if (equals(field.authoredPath, authoredPath))
      return &field;
  }
  return nullptr;
}

const AuthoredSynthParamField*
findAuthoredSynthParamFieldByCanonicalParam(const char* canonicalParam) {
  for (const auto& field : authoredSynthParamFields()) {
    if (equals(field.canonicalParam, canonicalParam))
      return &field;
  }
  return nullptr;
}

DocLuaValueKind authoredSynthValueKindForParamType(synth::param::ParamType type) {
  using Type = synth::param::ParamType;
  switch (type) {
  case Type::Bool:
    return DocLuaValueKind::Boolean;
  case Type::Int8:
    return DocLuaValueKind::Integer;
  case Type::OscBankID:
  case Type::NoiseType:
  case Type::FilterMode:
  case Type::DistortionType:
  case Type::PhaseMode:
  case Type::Subdivision:
    return DocLuaValueKind::String;
  case Type::Float:
    return DocLuaValueKind::Number;
  }
  return DocLuaValueKind::Any;
}

} // namespace app::doc
