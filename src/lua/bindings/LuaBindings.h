#pragma once

#include "lua/LuaState.h"

#include <string>
#include <vector>

#define CMD_CHECK(expr)                                                                            \
  do {                                                                                             \
    if (!(expr).ok) {                                                                              \
      luaL_error(L, "%s", (expr).err);                                                             \
      return CMD_FAILURE;                                                                          \
    }                                                                                              \
    printf("OK\n");                                                                                \
    return CMD_SUCCESS;                                                                            \
  } while (0)

#define CMD_BAD_INPUT 10
#define CMD_FAILURE   1
#define CMD_SUCCESS   0

namespace lua::bindings {

const std::vector<std::string>& getVisibleGlobals();
void addVisibleGlobal(const char* name);

inline void registerFunction(lua_State* L, lua_CFunction l_func, const char* name) {
  lua_pushcfunction(L, l_func);
  lua_setfield(L, -2, name);
}

void registerSynthBindings(lua_State* L, AppContext& appCtx);

} // namespace lua::bindings
