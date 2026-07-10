#include "TestRunner.h"

#include "app/ExternalEditorWorkspace.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace {

std::filesystem::path testRoot(const char* name) {
  return std::filesystem::temp_directory_path() / name;
}

bool writeFile(const std::filesystem::path& path, const char* text) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream out(path, std::ios::binary);
  if (!out)
    return false;
  out << text;
  return static_cast<bool>(out);
}

bool fileExists(const std::filesystem::path& path) {
  return std::filesystem::exists(path) && std::filesystem::is_regular_file(path);
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

app::GrooveboxPaths makeTestPaths(const std::filesystem::path& root) {
  app::GrooveboxPaths paths{};
  paths.configDir = root / "config";
  paths.lualsConfigFile = paths.configDir / ".luarc.json";
  paths.generatedDir = paths.configDir / "generated";
  paths.authoredStubDir = paths.generatedDir / "authored_document";
  return paths;
}

} // namespace

static void test_external_editor_workspace_json_escape() {
  TEST("external_editor_workspace_json_escape");

  const std::string escaped = app::escapeJsonStringForExternalEditorWorkspace("a\\b\"c\n");

  CHECK("slashes quotes newline escaped", escaped == "a\\\\b\\\"c\\n");
}

static void test_external_editor_workspace_writes_authored_stub_from_embedded_metadata() {
  TEST("external_editor_workspace_writes_authored_stub_from_embedded_metadata");

  const std::filesystem::path root = testRoot("groovebox-write-stub-test");
  std::filesystem::remove_all(root);

  const app::GrooveboxPaths paths = makeTestPaths(root);

  const app::ExternalEditorWorkspaceSetupResult result = app::ensureExternalEditorWorkspace(paths);

  CHECK("stub written", result.authoredStubsWritten);
  CHECK("config written", result.lualsConfigWritten);
  CHECK("message empty on success", result.message.empty());

  const std::string stub = readFile(paths.authoredStubDir / "meh_groovebox_authored.lua");
  CHECK("stub has authored function", stub.find("function track") != std::string::npos);
  CHECK("stub excludes applyFile", stub.find("applyFile") == std::string::npos);

  const std::string luarc = readFile(paths.lualsConfigFile);
  CHECK("luarc references authored", luarc.find("authored_document") != std::string::npos);

  std::filesystem::remove_all(root);
}

static void test_external_editor_workspace_overwrites_stale_stub() {
  TEST("external_editor_workspace_overwrites_stale_stub");

  const std::filesystem::path root = testRoot("groovebox-overwrite-stub-test");
  std::filesystem::remove_all(root);

  const app::GrooveboxPaths paths = makeTestPaths(root);
  CHECK("write stale file",
        writeFile(paths.authoredStubDir / "meh_groovebox_authored.lua", "-- stale\n"));

  const app::ExternalEditorWorkspaceSetupResult result = app::ensureExternalEditorWorkspace(paths);

  CHECK("stub written", result.authoredStubsWritten);
  CHECK("stale content replaced",
        fileExists(paths.authoredStubDir / "meh_groovebox_authored.lua") &&
            readFile(paths.authoredStubDir / "meh_groovebox_authored.lua").find("-- stale") ==
                std::string::npos);

  std::filesystem::remove_all(root);
}

void runExternalEditorWorkspaceTests() {
  SUITE("ExternalEditorWorkspace");
  test_external_editor_workspace_json_escape();
  test_external_editor_workspace_writes_authored_stub_from_embedded_metadata();
  test_external_editor_workspace_overwrites_stale_stub();
}
