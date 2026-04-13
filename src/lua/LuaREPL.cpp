#include "LuaREPL.h"

#include "LuaBindings.h"

#include "linenoise.h"
#include "utils/KeyProcessor.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

namespace lua::repl {
static lua_State* gL = nullptr;
static char gHintBuf[512];

namespace {
void finalizeCandidates(std::vector<std::string>& out) {
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
}

void collectTopLevelCandidates(const char* buf, std::vector<std::string>& out) {
  size_t len = strlen(buf);
  for (const auto& key : bindings::getVisibleGlobals()) {
    if (strncmp(key.c_str(), buf, len) == 0)
      out.emplace_back(key);
  }
}

bool pushTableByRawPath(const char* path) {
  lua_pushglobaltable(gL); // stack: _G

  const char* seg = path;
  while (*seg) {
    const char* nextDot = strchr(seg, '.');
    size_t segLen = nextDot ? (size_t)(nextDot - seg) : strlen(seg);

    if (segLen == 0) {
      lua_pop(gL, 1);
      return false;
    }

    lua_pushlstring(gL, seg, segLen);
    lua_rawget(gL, -2); // raw lookup only, no __index
    lua_remove(gL, -2); // discard previous table, keep fetched value

    if (lua_isnil(gL, -1)) {
      lua_pop(gL, 1);
      return false;
    }

    seg = nextDot ? nextDot + 1 : seg + segLen;
  }

  if (!lua_istable(gL, -1)) {
    lua_pop(gL, 1);
    return false;
  }

  return true; // resolved table remains on stack
}

void collectRealTableFields(lua_State* L, const char* fieldPrefix, std::vector<std::string>& out) {
  size_t prefixLen = strlen(fieldPrefix);

  lua_pushnil(L);
  while (lua_next(L, -2)) {
    if (lua_type(L, -2) == LUA_TSTRING) {
      const char* key = lua_tostring(L, -2);
      if (strncmp(key, fieldPrefix, prefixLen) == 0)
        out.emplace_back(key);
    }
    lua_pop(L, 1); // pop value, keep key
  }
}

void collectIndexedProxyFields(const char* path,
                               const char* fieldPrefix,
                               std::vector<std::string>& out) {
  size_t prefixLen = strlen(fieldPrefix);

  if (const auto* fields = bindings::getParamFields(path)) {
    for (const auto& field : *fields) {
      if (strncmp(field.c_str(), fieldPrefix, prefixLen) == 0)
        out.push_back(field);
    }
  }
}

std::vector<std::string> collectCompletionCandidates(const char* buf) {
  std::vector<std::string> out;

  if (!strlen(buf))
    return out;

  const char* lastDot = strrchr(buf, '.');
  if (!lastDot) {
    collectTopLevelCandidates(buf, out);
    finalizeCandidates(out);
    return out;
  }

  size_t pathLen = static_cast<size_t>(lastDot - buf);
  if (pathLen == 0)
    return out;

  std::string path(buf, pathLen);
  const char* fieldPrefix = lastDot + 1;
  std::vector<std::string> fields;

  if (!pushTableByRawPath(path.c_str()))
    return out;

  collectRealTableFields(gL, fieldPrefix, fields);
  lua_pop(gL, 1); // pop resolved table

  collectIndexedProxyFields(path.c_str(), fieldPrefix, fields);

  finalizeCandidates(fields);

  for (const auto& field : fields)
    out.emplace_back(path + "." + field);

  return out;
}

void completionCallback(const char* buf, linenoiseCompletions* lc) {
  std::vector<std::string> candidates = collectCompletionCandidates(buf);
  for (const auto& candidate : candidates)
    linenoiseAddCompletion(lc, candidate.c_str());
}

char* hintsCallback(const char* buf, int* color, int* bold) {
  std::vector<std::string> candidates = collectCompletionCandidates(buf);

  if (candidates.empty())
    return nullptr;

  if (candidates.size() == 1) {
    const std::string& full = candidates[0];
    size_t bufLen = strlen(buf);
    if (full.size() > bufLen) {
      snprintf(gHintBuf, sizeof(gHintBuf), "%s", full.c_str() + bufLen);
      *color = 90;
      *bold = 0;
      return gHintBuf;
    }
    return nullptr;
  }

  int written = snprintf(gHintBuf, sizeof(gHintBuf), "  [");
  auto writtenS = static_cast<size_t>(written);
  for (size_t i = 0; i < candidates.size() && written < (int)sizeof(gHintBuf) - 4; i++) {
    if (i > 0)
      written += snprintf(gHintBuf + written, sizeof(gHintBuf) - writtenS, ", ");

    const char* full = candidates[i].c_str();
    const char* lastDot = strrchr(full, '.');
    const char* display = lastDot ? lastDot + 1 : full;

    written += snprintf(gHintBuf + written, sizeof(gHintBuf) - writtenS, "%s", display);
  }

  snprintf(gHintBuf + written, sizeof(gHintBuf) - writtenS, "]");
  *color = 90;
  *bold = 0;
  return gHintBuf;
}

// gHintBuf is static; nothing to free
void freeHintsCallback(void* /*hint*/) {}

} // namespace

void runLuaREPL(app::AppContext* appCtx) {
  lua_State* L = luaL_newstate();
  luaL_openlibs(L);
  bindings::registerSynthBindings(L, *appCtx);

  gL = L;
  linenoiseSetCompletionCallback(completionCallback);
  linenoiseSetHintsCallback(hintsCallback);
  linenoiseSetFreeHintsCallback(freeHintsCallback);

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
  app::utils::requestQuit();
  lua_close(L);
}

} // namespace lua::repl
