#include "TestHelpers.h"
#include "TestRunner.h"

#include "app/AppContext.h"
#include "app/Constants.h"
#include "app/doc/DocMetadata.h"
#include "lua/bindings/LuaBindings.h"
#include "lua/metadata/LuaRuntimeMetadata.h"
#include "synth/params/ParamDefs.h"

#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace {

bool strEq(const char* a, const char* b) {
  return a && b && std::strcmp(a, b) == 0;
}

const lua::RuntimeLuaFunctionMetadata* findMethod(const lua::RuntimeLuaTableMetadata& table,
                                                  const char* name) {
  for (const auto& method : table.methods) {
    if (strEq(method.name, name))
      return &method;
  }
  return nullptr;
}

bool hasGlobal(const char* name) {
  return lua::findRuntimeLuaGlobal(name) != nullptr;
}

bool hasTable(const char* name) {
  return lua::findRuntimeLuaTable(name) != nullptr;
}

bool hasUserdataType(const char* name) {
  return lua::findRuntimeLuaUserdataType(name) != nullptr;
}

bool containsEngineProxyField(const std::vector<lua::RuntimeLuaProxyFieldMetadata>& fields,
                              const char* table,
                              const char* field) {
  for (const auto& item : fields) {
    if (item.table == table && item.field == field)
      return true;
  }
  return false;
}

bool containsAppProxyField(const std::vector<lua::RuntimeLuaProxyFieldMetadata>& fields,
                           const char* table,
                           const char* field) {
  for (const auto& item : fields) {
    if (item.table == table && item.field == field)
      return true;
  }
  return false;
}

bool luaGlobalIs(lua_State* L, const char* name, int luaType) {
  lua_getglobal(L, name);
  const bool ok = lua_type(L, -1) == luaType;
  lua_pop(L, 1);
  return ok;
}

bool luaTableHasFunction(lua_State* L, const char* tableName, const char* methodName) {
  lua_getglobal(L, tableName);
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return false;
  }

  lua_getfield(L, -1, methodName);
  const bool ok = lua_isfunction(L, -1);
  lua_pop(L, 2);
  return ok;
}

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
};

} // namespace

static void test_runtime_globals_include_canonical_apply_file() {
  TEST("runtime_globals_include_canonical_apply_file");
  const auto* applyFile = lua::findRuntimeLuaGlobal(lua::rtglobal::ApplyFile);
  CHECK("applyFile exists", applyFile != nullptr);
  CHECK("applyFile transitional",
        applyFile && applyFile->status == lua::RuntimeLuaStatus::Transitional);
  CHECK("apply_file absent", lua::findRuntimeLuaGlobal("apply_file") == nullptr);
}

static void test_runtime_globals_exclude_document_constructors() {
  TEST("runtime_globals_exclude_document_constructors");
  CHECK("no document track", !hasGlobal(app::doc::docglobal::Track));
  CHECK("no TrackSettings", !hasGlobal(app::doc::docctor::TrackSettings));
  CHECK("no SynthSettings", !hasGlobal(app::doc::docctor::SynthSettings));
  CHECK("no MixerSettings", !hasGlobal(app::doc::docctor::MixerSettings));
}

static void test_runtime_command_tables_are_present() {
  TEST("runtime_command_tables_are_present");
  CHECK("transport", hasTable(lua::rtglobal::Transport));
  CHECK("midi", hasTable(lua::rtglobal::Midi));
  CHECK("seq", hasTable(lua::rtglobal::Seq));
  CHECK("preset", hasTable(lua::rtglobal::Preset));
  CHECK("mod", hasTable(lua::rtglobal::Mod));
  CHECK("fm", hasTable(lua::rtglobal::Fm));
  CHECK("fx", hasTable(lua::rtglobal::Fx));
  CHECK("signal", hasTable(lua::rtglobal::Signal));
  CHECK("mixer", hasTable(lua::rtglobal::Mixer));
}

static void test_top_level_helpers_are_present() {
  TEST("top_level_helpers_are_present");
  CHECK("panic", hasGlobal(lua::rtglobal::Panic));
  CHECK("params", hasGlobal(lua::rtglobal::Params));
  CHECK("get", hasGlobal(lua::rtglobal::Get));
  CHECK("help", hasGlobal(lua::rtglobal::Help));
  CHECK("applyFile", hasGlobal(lua::rtglobal::ApplyFile));
  CHECK("clear", hasGlobal(lua::rtglobal::Clear));
  CHECK("quit", hasGlobal(lua::rtglobal::Quit));
}

static void test_seq_metadata_uses_static_bounds() {
  TEST("seq_metadata_uses_static_bounds");
  const auto* seq = lua::findRuntimeLuaTable(lua::rtglobal::Seq);
  CHECK("seq exists", seq != nullptr);

  const auto* selectTrack = seq ? findMethod(*seq, lua::rtmethod::SelectTrack) : nullptr;
  CHECK("seq.selectTrack exists", selectTrack != nullptr);
  CHECK("seq.selectTrack max track",
        selectTrack && selectTrack->args.data[0].integerBounds.max == app::MAX_TRACKS);
}

static void test_seq_editing_userdata_metadata_is_absent() {
  TEST("seq_editing_userdata_metadata_is_absent");
  CHECK("SeqTrack metadata absent", !hasUserdataType("SeqTrack"));
  CHECK("SeqStep metadata absent", !hasUserdataType("SeqStep"));
}

static void test_engine_param_proxy_fields_derive_from_param_defs() {
  TEST("engine_param_proxy_fields_derive_from_param_defs");
  std::vector<lua::RuntimeLuaProxyFieldMetadata> fields{};
  lua::collectRuntimeLuaEngineParamProxyFields(fields);

  CHECK("some fields", !fields.empty());
  CHECK("osc1 has a field",
        containsEngineProxyField(fields, "osc1", "bank") ||
            containsEngineProxyField(fields, "osc1", "freq"));
  CHECK("fx reverb has a field",
        containsEngineProxyField(fields, "fx.reverb", "decay") ||
            containsEngineProxyField(fields, "fx.reverb", "mix"));

  int countedDottedParams = 0;
  for (const auto& def : synth::param::PARAM_DEFS) {
    if (std::strchr(def.name, '.'))
      ++countedDottedParams;
  }
  CHECK("derived count <= dotted param count",
        static_cast<int>(fields.size()) <= countedDottedParams);
}

static void test_app_param_proxy_fields_derive_from_app_param_defs() {
  TEST("app_param_proxy_fields_derive_from_app_param_defs");
  std::vector<lua::RuntimeLuaProxyFieldMetadata> fields{};
  lua::collectRuntimeLuaAppParamProxyFields(fields);

  CHECK("some app fields", !fields.empty());
  CHECK("mixer has representative field",
        containsAppProxyField(fields, "mixer", "gain") ||
            containsAppProxyField(fields, "mixer", "masterGain"));
}

static void test_runtime_metadata_has_unique_global_symbols() {
  TEST("runtime_metadata_has_unique_global_symbols");
  std::set<std::string> seen{};
  bool unique = true;
  for (const auto& global : lua::runtimeLuaGlobals()) {
    if (!seen.insert(global.name).second)
      unique = false;
  }
  CHECK("unique globals", unique);
}

static void test_runtime_table_methods_are_unique() {
  TEST("runtime_table_methods_are_unique");
  bool unique = true;

  for (const auto& table : lua::runtimeLuaTables()) {
    std::set<std::string> seen{};
    for (const auto& method : table.methods) {
      if (!seen.insert(method.name).second)
        unique = false;
    }
  }

  for (const auto& type : lua::runtimeLuaUserdataTypes()) {
    std::set<std::string> seen{};
    for (const auto& method : type.methods) {
      if (!seen.insert(method.name).second)
        unique = false;
    }
  }

  CHECK("unique methods", unique);
}

static void test_runtime_binding_registers_metadata_globals() {
  TEST("runtime_binding_registers_metadata_globals");
  LuaFixture fixture{};

  CHECK("applyFile function", luaGlobalIs(fixture.L, lua::rtglobal::ApplyFile, LUA_TFUNCTION));
  CHECK("transport table", luaGlobalIs(fixture.L, lua::rtglobal::Transport, LUA_TTABLE));
  CHECK("midi table", luaGlobalIs(fixture.L, lua::rtglobal::Midi, LUA_TTABLE));
  CHECK("seq table", luaGlobalIs(fixture.L, lua::rtglobal::Seq, LUA_TTABLE));
  CHECK("preset table", luaGlobalIs(fixture.L, lua::rtglobal::Preset, LUA_TTABLE));
  CHECK("mod table", luaGlobalIs(fixture.L, lua::rtglobal::Mod, LUA_TTABLE));
  CHECK("fm table", luaGlobalIs(fixture.L, lua::rtglobal::Fm, LUA_TTABLE));
  CHECK("fx table", luaGlobalIs(fixture.L, lua::rtglobal::Fx, LUA_TTABLE));
  CHECK("signal table", luaGlobalIs(fixture.L, lua::rtglobal::Signal, LUA_TTABLE));
  CHECK("mixer table", luaGlobalIs(fixture.L, lua::rtglobal::Mixer, LUA_TTABLE));
}

static void test_runtime_binding_excludes_authored_document_symbols() {
  TEST("runtime_binding_excludes_authored_document_symbols");
  LuaFixture fixture{};

  CHECK("apply_file nil", luaGlobalIs(fixture.L, "apply_file", LUA_TNIL));
  CHECK("top-level track nil", luaGlobalIs(fixture.L, app::doc::docglobal::Track, LUA_TNIL));
  CHECK("TrackSettings nil", luaGlobalIs(fixture.L, app::doc::docctor::TrackSettings, LUA_TNIL));
  CHECK("SynthSettings nil", luaGlobalIs(fixture.L, app::doc::docctor::SynthSettings, LUA_TNIL));
  CHECK("MixerSettings nil", luaGlobalIs(fixture.L, app::doc::docctor::MixerSettings, LUA_TNIL));
}

static void test_runtime_binding_command_table_methods_match_metadata() {
  TEST("runtime_binding_command_table_methods_match_metadata");
  LuaFixture fixture{};

  for (const auto& table : lua::runtimeLuaTables()) {
    for (const auto& method : table.methods) {
      CHECK("method exists", luaTableHasFunction(fixture.L, table.name, method.name));
    }
  }
}

static void test_runtime_visible_globals_include_metadata_globals() {
  TEST("runtime_visible_globals_include_metadata_globals");
  LuaFixture fixture{};

  const auto& visible = lua::bindings::getVisibleGlobals();
  auto contains = [&](const char* name) {
    for (const auto& item : visible) {
      if (item == name)
        return true;
    }
    return false;
  };

  CHECK("applyFile visible", contains(lua::rtglobal::ApplyFile));
  CHECK("transport visible", contains(lua::rtglobal::Transport));
  CHECK("midi visible", contains(lua::rtglobal::Midi));
  CHECK("seq visible", contains(lua::rtglobal::Seq));
  CHECK("mixer visible", contains(lua::rtglobal::Mixer));
}

static void test_authored_and_runtime_metadata_surfaces_are_separate() {
  TEST("authored_and_runtime_metadata_surfaces_are_separate");
  CHECK("authored document track exists",
        app::doc::findAuthoredDocumentFunction(app::doc::docglobal::Track) != nullptr);
  CHECK("runtime top-level track absent", lua::findRuntimeLuaGlobal("track") == nullptr);

  CHECK("runtime applyFile exists", lua::findRuntimeLuaGlobal(lua::rtglobal::ApplyFile) != nullptr);
  CHECK("authored applyFile absent",
        app::doc::findAuthoredDocumentFunction(lua::rtglobal::ApplyFile) == nullptr);

  CHECK("runtime apply_file absent", lua::findRuntimeLuaGlobal("apply_file") == nullptr);
  CHECK("authored apply_file absent",
        app::doc::findAuthoredDocumentFunction("apply_file") == nullptr);

  CHECK("runtime TrackSettings absent",
        lua::findRuntimeLuaGlobal(app::doc::docctor::TrackSettings) == nullptr);
  CHECK("runtime transport exists", lua::findRuntimeLuaTable(lua::rtglobal::Transport) != nullptr);
  CHECK("authored transport absent",
        app::doc::findAuthoredDocumentFunction(lua::rtglobal::Transport) == nullptr);
}

void runLuaRuntimeMetadataTests() {
  SUITE("LuaRuntimeMetadata");
  test_runtime_globals_include_canonical_apply_file();
  test_runtime_globals_exclude_document_constructors();
  test_runtime_command_tables_are_present();
  test_top_level_helpers_are_present();
  test_seq_metadata_uses_static_bounds();
  test_seq_editing_userdata_metadata_is_absent();
  test_engine_param_proxy_fields_derive_from_param_defs();
  test_app_param_proxy_fields_derive_from_app_param_defs();
  test_runtime_metadata_has_unique_global_symbols();
  test_runtime_table_methods_are_unique();
  test_runtime_binding_registers_metadata_globals();
  test_runtime_binding_excludes_authored_document_symbols();
  test_runtime_binding_command_table_methods_match_metadata();
  test_runtime_visible_globals_include_metadata_globals();
  test_authored_and_runtime_metadata_surfaces_are_separate();
}
