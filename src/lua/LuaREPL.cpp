#include "LuaREPL.h"

#include "LuaBindings.h"

#include "linenoise.h"
#include "utils/KeyProcessor.h"

namespace lua::repl {
static lua_State* gL = nullptr; // for completion callback

static void completionCallback(const char* buf, linenoiseCompletions* lc) {
  size_t len = strlen(buf);

  lua_pushglobaltable(gL); // push _G
  lua_pushnil(gL);
  while (lua_next(gL, -2)) { // iterate _G keys
    if (lua_type(gL, -2) == LUA_TSTRING) {
      const char* key = lua_tostring(gL, -2);
      if (strncmp(key, buf, len) == 0)
        linenoiseAddCompletion(lc, key);
    }
    lua_pop(gL, 1); // pop value, keep key for next
  }
  lua_pop(gL, 1); // pop _G
}

void runLuaREPL(app::AppContext& appCtx) {
  lua_State* L = luaL_newstate();
  luaL_openlibs(L);
  bindings::registerSynthBindings(L, appCtx);

  gL = L;
  linenoiseSetCompletionCallback(completionCallback);
  linenoiseHistorySetMaxLen(500);

  char histPath[512];
  const char* home = getenv("HOME");
  snprintf(histPath, sizeof(histPath), "%s/.meh_synth_history", home ? home : ".");
  linenoiseHistoryLoad(histPath);

  std::string buffer;
  char* line;

  while ((line = linenoise(buffer.empty() ? "> " : ">> ")) != nullptr) {
    if (*line == '\0') {
      linenoiseFree(line);
      continue;
    }

    buffer += line;
    buffer += "\n";
    linenoiseFree(line);

    if (luaL_loadstring(L, buffer.c_str()) == LUA_OK) {
      buffer.pop_back(); // remove \n (newline)
      linenoiseHistoryAdd(buffer.c_str());
      linenoiseHistorySave(histPath);

      if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
      }
      buffer.clear();
    } else {
      const char* err = lua_tostring(L, -1);
      if (strstr(err, "<eof>")) {
        lua_pop(L, 1); // incomplete chunk — keep accumulating
      } else {
        fprintf(stderr, "%s\n", err);
        lua_pop(L, 1);
        buffer.clear();
      }
    }
  }

  gL = nullptr;
  synth::utils::requestQuit();
  lua_close(L);
}

} // namespace lua::repl
