#pragma once

#include <string>
#include <vector>

struct lua_State;

namespace lua::bindings {

const std::vector<std::string>* getAppParamFields(const char* table);

void registerMixerBindings(lua_State* L);

} // namespace lua::bindings
