#pragma once

#include "LuaState.h"

#include <string>
#include <vector>

namespace lua::bindings {

void registerSynthBindings(lua_State* L, AppContext& appCtx);

const std::vector<std::string>& getVisibleGlobals();
const std::vector<std::string>* getParamFields(const char* group);

} // namespace lua::bindings
