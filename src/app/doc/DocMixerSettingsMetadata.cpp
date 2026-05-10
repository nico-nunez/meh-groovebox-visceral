#include "app/doc/DocMixerSettingsMetadata.h"

#include <cstring>

namespace app::doc {
namespace {

namespace ap = app::params;

template <typename T, std::size_t N> constexpr DocMetadataSpan<T> spanOf(const T (&items)[N]) {
  return {items, N};
}

bool equals(const char* a, const char* b) {
  return a && b && std::strcmp(a, b) == 0;
}

constexpr AuthoredMixerParamField field(const char* authoredField,
                                        ap::AppParamID paramID,
                                        DocLuaValueKind valueKind,
                                        const char* doc = "") {
  return {authoredField, paramID, valueKind, doc};
}

constexpr AuthoredMixerParamField kAuthoredTrackMixerParamFields[] = {
    field("gain",
          ap::AppParamID::TrackGain,
          DocLuaValueKind::Number,
          "Track output gain. Range 0.0..1.0."),
    field("pan",
          ap::AppParamID::TrackPan,
          DocLuaValueKind::Number,
          "Track pan position. Range -1.0..1.0."),
    field("mute", ap::AppParamID::TrackMute, DocLuaValueKind::Boolean, "Track mute state."),
};

} // namespace

DocMetadataSpan<AuthoredMixerParamField> authoredTrackMixerParamFields() {
  return spanOf(kAuthoredTrackMixerParamFields);
}

const AuthoredMixerParamField* findAuthoredTrackMixerParamField(const char* authoredField) {
  for (const auto& f : authoredTrackMixerParamFields()) {
    if (equals(f.authoredField, authoredField))
      return &f;
  }
  return nullptr;
}

DocLuaValueKind authoredMixerValueKindForParamType(app::params::AppParamType type) {
  using Type = app::params::AppParamType;
  switch (type) {
  case Type::Bool:
    return DocLuaValueKind::Boolean;
  case Type::Float:
    return DocLuaValueKind::Number;
  }
  return DocLuaValueKind::Any;
}

} // namespace app::doc
