#pragma once

#include <filesystem>

namespace app {

struct GrooveboxPaths {
  std::filesystem::path configDir{};
  std::filesystem::path sessionFile{};
  std::filesystem::path lastSessionFile{};
  std::filesystem::path lualsConfigFile{};
  std::filesystem::path generatedDir{};
  std::filesystem::path authoredStubDir{};
};

GrooveboxPaths resolveGrooveboxPaths(int argc, char* argv[]);

} // namespace app
