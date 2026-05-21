#pragma once

#include "app/GrooveboxPaths.h"

#include <filesystem>
#include <string>

namespace app {

struct ExternalEditorWorkspaceSetupResult {
  bool authoredStubSourceFound = false;
  bool authoredStubsCopied = false;
  bool lualsConfigWritten = false;
  std::string message{};
};

std::string escapeJsonStringForExternalEditorWorkspace(const std::string& value);

ExternalEditorWorkspaceSetupResult
ensureExternalEditorWorkspace(const GrooveboxPaths& paths,
                              const std::filesystem::path& projectRoot);

} // namespace app
