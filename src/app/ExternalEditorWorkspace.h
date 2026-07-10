#pragma once

#include "app/GrooveboxPaths.h"

#include <string>

namespace app {

struct ExternalEditorWorkspaceSetupResult {
  bool authoredStubsWritten = false;
  bool lualsConfigWritten = false;
  std::string message{};
};

std::string escapeJsonStringForExternalEditorWorkspace(const std::string& value);

ExternalEditorWorkspaceSetupResult ensureExternalEditorWorkspace(const GrooveboxPaths& paths);

} // namespace app
