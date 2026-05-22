#include "app/GrooveboxPaths.h"

#include "app/editor/AuthoredDocEditor.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace app {
namespace {

std::filesystem::path defaultConfigDir() {
  const char* home = std::getenv("HOME");
  if (!home || home[0] == '\0')
    return std::filesystem::temp_directory_path() / "groovebox";

  // return std::filesystem::path(home) / ".config" / "groovebox";
  return std::filesystem::path(home) / "groovebox-demo";
}

std::string readFirstLine(const std::filesystem::path& path) {
  std::ifstream in(path);
  if (!in)
    return {};

  std::string line{};
  std::getline(in, line);
  return line;
}

bool writeTextFile(const std::filesystem::path& path, const std::string& text) {
  std::error_code ec{};
  std::filesystem::create_directories(path.parent_path(), ec);
  if (ec)
    return false;

  std::ofstream out(path, std::ios::binary);
  if (!out)
    return false;

  out << text;
  return static_cast<bool>(out);
}

std::filesystem::path resolveSessionFile(const GrooveboxPaths& paths, int argc, char* argv[]) {
  if (argc > 1 && argv[1] && argv[1][0] != '\0')
    return std::filesystem::absolute(std::filesystem::path(argv[1]));

  const std::string lastSession = readFirstLine(paths.lastSessionFile);
  if (!lastSession.empty())
    return std::filesystem::absolute(std::filesystem::path(lastSession));

  return paths.configDir / "session.lua";
}

void ensureSessionFileExists(const std::filesystem::path& sessionFile) {
  if (std::filesystem::exists(sessionFile))
    return;

  writeTextFile(sessionFile, app::editor::authoredDocumentTemplate());
}

} // namespace

GrooveboxPaths resolveGrooveboxPaths(int argc, char* argv[]) {
  GrooveboxPaths paths{};
  paths.configDir = defaultConfigDir();
  paths.lastSessionFile = paths.configDir / "last_session";
  paths.lualsConfigFile = paths.configDir / ".luarc.json";
  paths.generatedDir = paths.configDir / "generated";
  paths.authoredStubDir = paths.generatedDir / "authored_document";

  std::error_code ec{};
  std::filesystem::create_directories(paths.configDir, ec);

  paths.sessionFile = resolveSessionFile(paths, argc, argv);
  ensureSessionFileExists(paths.sessionFile);
  writeTextFile(paths.lastSessionFile, paths.sessionFile.string() + "\n");

  return paths;
}

} // namespace app
