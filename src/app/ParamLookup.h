#pragma once

#include "synth/ParamDefs.h"

namespace app::param_lookup {

synth::param::ParamID getParamIDByName(const char* name);
const char* getParamName(synth::param::ParamID id);
void printParamList(const char* optionalFilter = nullptr);

} // namespace app::param_lookup
