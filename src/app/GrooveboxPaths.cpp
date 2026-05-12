#include "app/GrooveboxPaths.h"

#include "app/editor/AuthoredDocEditor.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace app {
namespace {

std::filesystem::path grooveboxConfigDir() {
  const char* home = std::getenv("HOME");
  if (!home || home[0] == '\0')
    return std::filesystem::temp_directory_path() / "groovebox";
  return std::filesystem::path(home) / ".config" / "groovebox";
}

std::filesystem::path lastSessionFilePath(const std::filesystem::path& configDir) {
  return configDir / "last_session";
}

std::string readLastSession(const std::filesystem::path& configDir) {
  std::ifstream f(lastSessionFilePath(configDir));
  if (!f)
    return {};
  std::string path;
  std::getline(f, path);
  return path;
}

void writeLastSession(const std::filesystem::path& configDir,
                      const std::filesystem::path& sessionFile) {
  std::ofstream f(lastSessionFilePath(configDir));
  if (f)
    f << sessionFile.string() << '\n';
}

void ensureDefaultSession(const std::filesystem::path& path) {
  if (std::filesystem::exists(path))
    return;
  std::ofstream f(path);
  if (f)
    f << editor::authoredDocumentTemplate();
}

} // namespace

GrooveboxPaths resolveGrooveboxPaths(int argc, char* argv[]) {
  GrooveboxPaths paths;
  paths.configDir = grooveboxConfigDir();

  std::filesystem::create_directories(paths.configDir);

  if (argc > 1 && argv[1][0] != '\0') {
    paths.sessionFile = std::filesystem::absolute(std::filesystem::path(argv[1]));
  } else {
    const std::string last = readLastSession(paths.configDir);
    if (!last.empty() && std::filesystem::exists(last))
      paths.sessionFile = last;
    else
      paths.sessionFile = paths.configDir / "session.lua";
  }

  ensureDefaultSession(paths.sessionFile);
  writeLastSession(paths.configDir, paths.sessionFile);

  return paths;
}

} // namespace app
