#pragma once

#include "LuaState.h"

namespace lua::bindings {

void registerSynthBindings(lua_State* L, AppContext& appCtx);

} // namespace lua::bindings
