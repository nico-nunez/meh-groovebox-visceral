#include "TestHelpers.h"
#include "TestRunner.h"

#include "app/AppContext.h"
#include "app/doc/DocAuthoringService.h"
#include "lua/bindings/LuaBindings.h"

extern "C" {
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include <cstdio>
#include <string>

namespace {

static constexpr const char* kOneTrackDocument =
    "track(1, TrackSettings { patterns = { [1] = { numSteps = 1, stepsPerBeat = 4, "
    "steps = { { active = true, notes = { { note = 60, velocity = 100 } } } } } }, activeSlot = 1 })";

static constexpr const char* kTempFilePath = "/tmp/doc_binding_test.lua";

using test::hasDiagnostic;

struct LuaFixture {
  app::AppContext* app = nullptr;
  lua_State* L = nullptr;

  LuaFixture() {
    app = test::makeAppContext();
    L = luaL_newstate();
    luaL_openlibs(L);
    lua::bindings::registerSynthBindings(L, *app);
  }

  ~LuaFixture() {
    if (L)
      lua_close(L);
    test::destroyAppContext(app);
  }

  // Returns true if the Lua string executed without error.
  bool exec(const char* code) { return luaL_dostring(L, code) == LUA_OK; }

  // Returns the top-of-stack string (error message after a failed exec).
  std::string topString() {
    const char* s = lua_tostring(L, -1);
    std::string result = s ? s : "";
    lua_pop(L, 1);
    return result;
  }
};

} // namespace

static void test_apply_file_success_calls_document_service() {
  TEST("apply_file_success_calls_document_service");
  LuaFixture fixture{};

  FILE* f = fopen(kTempFilePath, "w");
  fputs(kOneTrackDocument, f);
  fclose(f);

  std::string luaCall = std::string("applyFile('") + kTempFilePath + "')";
  CHECK("Lua call ok", fixture.exec(luaCall.c_str()));
  CHECK("status Completed",
        fixture.app->documents.authoring.apply.status == app::doc::ApplyStatus::Completed);
  CHECK("track 0 admitted",
        fixture.app->documents.authoring.apply.lastAdmittedDocModel.sequencer.hasTrackState[0]);
}

static void test_apply_file_failure_surfaces_first_diagnostic() {
  TEST("apply_file_failure_surfaces_first_diagnostic");
  LuaFixture fixture{};

  bool ok = fixture.exec("applyFile('/path/that/does/not/exist.lua')");
  CHECK("Lua call failed", !ok);

  std::string errMsg = fixture.topString();
  CHECK("error contains file read message",
        errMsg.find("failed to read document file") != std::string::npos);
  CHECK("diagnostic document.file.read_failed",
        hasDiagnostic(fixture.app->documents.authoring.apply.diagnostics,
                      "document.file.read_failed"));
}

static void test_lua_binding_does_not_register_document_constructors() {
  TEST("lua_binding_does_not_register_document_constructors");
  LuaFixture fixture{};

  lua_getglobal(fixture.L, "applyFile");
  CHECK("applyFile is registered", lua_isfunction(fixture.L, -1));
  lua_pop(fixture.L, 1);

  lua_getglobal(fixture.L, "TrackSettings");
  CHECK("TrackSettings not registered", lua_isnil(fixture.L, -1));
  lua_pop(fixture.L, 1);

  lua_getglobal(fixture.L, "SynthSettings");
  CHECK("SynthSettings not registered", lua_isnil(fixture.L, -1));
  lua_pop(fixture.L, 1);

  lua_getglobal(fixture.L, "MixerSettings");
  CHECK("MixerSettings not registered", lua_isnil(fixture.L, -1));
  lua_pop(fixture.L, 1);

  lua_getglobal(fixture.L, "track");
  CHECK("document track() not registered", lua_isnil(fixture.L, -1));
  lua_pop(fixture.L, 1);
}

void runDocApplyFileBindingTests() {
  SUITE("Lua / DocApplyFileBinding");
  test_apply_file_success_calls_document_service();
  test_apply_file_failure_surfaces_first_diagnostic();
  test_lua_binding_does_not_register_document_constructors();
}
