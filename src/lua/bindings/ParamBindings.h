#pragma once

#include "LuaBindings.h"

namespace lua::bindings {

const std::vector<std::string>* getParamFields(const char* group);

int paramGroupIndex(lua_State* L);
int paramGroupNewIndex(lua_State* L);
void registerParamBindings(lua_State*);

} // namespace lua::bindings
