#pragma once

#include <filesystem>

namespace app {

struct GrooveboxPaths {
  std::filesystem::path configDir;   // ~/.config/groovebox
  std::filesystem::path sessionFile; // resolved, absolute session file path
};

GrooveboxPaths resolveGrooveboxPaths(int argc, char* argv[]);

} // namespace app
