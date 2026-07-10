#include "app/ExternalEditorWorkspace.h"

#include "app/doc/DocLuaLSStub.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace app {
namespace {

bool writeAuthoredStub(const GrooveboxPaths& paths, std::string* message) {
  std::error_code ec{};
  std::filesystem::create_directories(paths.authoredStubDir, ec);
  if (ec) {
    if (message)
      *message = "failed to create generated dir: " + ec.message();
    return false;
  }

  const std::filesystem::path stubPath = paths.authoredStubDir / "meh_groovebox_authored.lua";
  std::ofstream out(stubPath, std::ios::binary);
  if (!out) {
    if (message)
      *message = "failed to open authored LuaLS stub for writing: " + stubPath.string();
    return false;
  }

  out << doc::renderAuthoredDocumentLuaLSStub();
  if (!out) {
    if (message)
      *message = "failed while writing authored LuaLS stub";
    return false;
  }

  return true;
}

bool writeLuaLSConfigFile(const GrooveboxPaths& paths, std::string* message) {
  std::error_code ec{};
  std::filesystem::create_directories(paths.configDir, ec);
  if (ec) {
    if (message)
      *message = "failed to create config dir: " + ec.message();
    return false;
  }

  std::ofstream out(paths.lualsConfigFile, std::ios::binary);
  if (!out) {
    if (message)
      *message = "failed to open .luarc.json for writing";
    return false;
  }

  const std::string libraryPath =
      escapeJsonStringForExternalEditorWorkspace(paths.authoredStubDir.string());

  out << "{\n";
  out << "  \"$schema\": "
         "\"https://raw.githubusercontent.com/LuaLS/vscode-lua/master/setting/schema.json\",\n";
  out << "  \"runtime.version\": \"Lua 5.4\",\n";
  out << "  \"workspace.library\": [\n";
  out << "    \"" << libraryPath << "\"\n";
  out << "  ]\n";
  out << "}\n";

  if (!out) {
    if (message)
      *message = "failed while writing .luarc.json";
    return false;
  }

  return true;
}

} // namespace

std::string escapeJsonStringForExternalEditorWorkspace(const std::string& value) {
  std::string out{};
  out.reserve(value.size());

  for (const char ch : value) {
    switch (ch) {
    case '\\':
      out += "\\\\";
      break;
    case '"':
      out += "\\\"";
      break;
    case '\b':
      out += "\\b";
      break;
    case '\f':
      out += "\\f";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\r':
      out += "\\r";
      break;
    case '\t':
      out += "\\t";
      break;
    default:
      out.push_back(ch);
      break;
    }
  }

  return out;
}

ExternalEditorWorkspaceSetupResult ensureExternalEditorWorkspace(const GrooveboxPaths& paths) {
  ExternalEditorWorkspaceSetupResult result{};

  result.authoredStubsWritten = writeAuthoredStub(paths, &result.message);
  if (!result.authoredStubsWritten) {
    std::printf("[editor] %s\n", result.message.c_str());
    return result;
  }

  result.lualsConfigWritten = writeLuaLSConfigFile(paths, &result.message);
  if (!result.lualsConfigWritten)
    std::printf("[editor] %s\n", result.message.c_str());

  return result;
}

} // namespace app
