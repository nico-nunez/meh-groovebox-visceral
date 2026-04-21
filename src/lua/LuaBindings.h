// #pragma once
//
// #include "LuaState.h"
//
// #include <string>
// #include <vector>
//
// #define CMD_BAD_INPUT 10
// #define CMD_FAILURE   1
// #define CMD_SUCCESS   0
//
// namespace lua::bindings {
//
// void registerSynthBindings(lua_State* L, AppContext& appCtx);
//
// const std::vector<std::string>& getVisibleGlobals();
// const std::vector<std::string>* getParamFields(const char* group);
// void addVisibleGlobal(const char* name);
//
// } // namespace lua::bindings
