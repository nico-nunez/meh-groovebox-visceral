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

static void test_external_editor_workspace_missing_stubs_is_nonfatal() {
  TEST("external_editor_workspace_missing_stubs_is_nonfatal");

  const std::filesystem::path root = testRoot("groovebox-missing-stubs-test");
  std::filesystem::remove_all(root);

  app::GrooveboxPaths paths = makeTestPaths(root);
  const std::filesystem::path projectRoot = root / "missing-project";

  const app::ExternalEditorWorkspaceSetupResult result =
      app::ensureExternalEditorWorkspace(paths, projectRoot);

  CHECK("source missing", !result.authoredStubSourceFound);
  CHECK("copy skipped", !result.authoredStubsCopied);
  CHECK("config not written", !result.lualsConfigWritten);
  CHECK("message present", !result.message.empty());

  std::filesystem::remove_all(root);
}

static void test_external_editor_workspace_copies_authored_stubs_only() {
  TEST("external_editor_workspace_copies_authored_stubs_only");

  const std::filesystem::path root = testRoot("groovebox-copy-stubs-test");
  std::filesystem::remove_all(root);

  const std::filesystem::path projectRoot = root / "project";
  const std::filesystem::path authoredSrc =
      projectRoot / "generated" / "luals" / "authored_document";
  const std::filesystem::path runtimeSrc = projectRoot / "generated" / "luals" / "runtime_lua";

  CHECK("write authored stub",
        writeFile(authoredSrc / "meh_groovebox_authored.lua", "track = nil\n"));
  CHECK("write runtime stub",
        writeFile(runtimeSrc / "meh_groovebox_runtime.lua", "applyFile = nil\n"));

  app::GrooveboxPaths paths = makeTestPaths(root);
  CHECK("write stale file", writeFile(paths.authoredStubDir / "stale.lua", "stale\n"));

  const app::ExternalEditorWorkspaceSetupResult result =
      app::ensureExternalEditorWorkspace(paths, projectRoot);

  CHECK("source found", result.authoredStubSourceFound);
  CHECK("copy ok", result.authoredStubsCopied);
  CHECK("config written", result.lualsConfigWritten);
  CHECK("authored copied", fileExists(paths.authoredStubDir / "meh_groovebox_authored.lua"));
  CHECK("stale removed", !fileExists(paths.authoredStubDir / "stale.lua"));
  CHECK("runtime not copied",
        !fileExists(paths.generatedDir / "runtime_lua" / "meh_groovebox_runtime.lua"));

  const std::string luarc = readFile(paths.lualsConfigFile);
  CHECK("luarc references authored", luarc.find("authored_document") != std::string::npos);
  CHECK("luarc excludes runtime", luarc.find("runtime_lua") == std::string::npos);
  CHECK("luarc excludes applyFile", luarc.find("applyFile") == std::string::npos);

  std::filesystem::remove_all(root);
}

void runExternalEditorWorkspaceTests() {
  SUITE("ExternalEditorWorkspace");
  test_external_editor_workspace_json_escape();
  test_external_editor_workspace_missing_stubs_is_nonfatal();
  test_external_editor_workspace_copies_authored_stubs_only();
}
