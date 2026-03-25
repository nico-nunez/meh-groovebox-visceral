#pragma once

#include "LuaState.h"

namespace lua::bindings {

void registerSynthBindings(lua_State* L, Engine& engine, hSynthSession session);

} // namespace lua::bindings
