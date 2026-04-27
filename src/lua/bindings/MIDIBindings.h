#pragma once

struct lua_State;

namespace lua::bindings {

void registerMIDICommands(lua_State*);

} // namespace lua::bindings
