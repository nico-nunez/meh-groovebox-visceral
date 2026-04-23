#pragma once

struct lua_State;

namespace lua::bindings {

void registerSeqCommands(lua_State* L);
}
