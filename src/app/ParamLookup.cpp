#include "ParamLookup.h"

#include <cstdio>
#include <cstring>

namespace app::param_lookup {

using synth::param::PARAM_COUNT;
using synth::param::PARAM_DEFS;
using synth::param::ParamID;

ParamID getParamIDByName(const char* name) {
  for (int i = 0; i < PARAM_COUNT; ++i) {
    if (strcmp(PARAM_DEFS[i].name, name) == 0)
      return static_cast<ParamID>(i);
  }
  return synth::param::ParamID::UNKNOWN;
}

const char* getParamName(ParamID id) {
  if (id >= 0 && id < PARAM_COUNT)
    return PARAM_DEFS[id].name;
  return nullptr;
}

void printParamList(const char* optionalFilter) {
  if (optionalFilter != nullptr) {
    for (const auto& def : PARAM_DEFS) {
      if (strstr(def.name, optionalFilter) != nullptr)
        printf("  %s\n", def.name);
    }
    return;
  }

  for (const auto& def : PARAM_DEFS)
    printf("  %s\n", def.name);
}

} // namespace app::param_lookup
