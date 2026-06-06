#include "SynthParamFields.h"

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

constexpr AuthoredSynthParamField kAuthoredSynthParamFields[] = {
#define X(id, root, field, type, min, max, def, group)                                             \
  {PUBLIC_PARAM_STR(root, field),                                                                  \
   PUBLIC_PARAM_STR(root, field),                                                                  \
   synth::param::id,                                                                               \
   synthParamTypeToLuaKind(sp::ParamType::type)},
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

} // namespace app::doc
