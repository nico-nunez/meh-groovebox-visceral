#include "app/ExternalEditorWorkspace.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace app {
namespace {

std::filesystem::path authoredStubSourceDir(const std::filesystem::path& projectRoot) {
  return projectRoot / "generated" / "luals" / "authored_document";
}

bool refreshDirectoryCopy(const std::filesystem::path& src,
                          const std::filesystem::path& dst,
                          std::string* message) {
  if (!std::filesystem::exists(src) || !std::filesystem::is_directory(src)) {
    if (message)
      *message = "authored LuaLS stubs not found: " + src.string();
    return false;
  }

  std::error_code ec{};
  std::filesystem::remove_all(dst, ec);
  if (ec) {
    if (message)
      *message = "failed to remove stale authored stubs: " + ec.message();
    return false;
  }

  std::filesystem::create_directories(dst.parent_path(), ec);
  if (ec) {
    if (message)
      *message = "failed to create generated dir: " + ec.message();
    return false;
  }

  std::filesystem::copy(src, dst, std::filesystem::copy_options::recursive, ec);
  if (ec) {
    if (message)
      *message = "failed to copy authored LuaLS stubs: " + ec.message();
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

ExternalEditorWorkspaceSetupResult
ensureExternalEditorWorkspace(const GrooveboxPaths& paths,
                              const std::filesystem::path& projectRoot) {
  ExternalEditorWorkspaceSetupResult result{};
  const std::filesystem::path src = authoredStubSourceDir(projectRoot);

  result.authoredStubSourceFound =
      std::filesystem::exists(src) && std::filesystem::is_directory(src);
  result.authoredStubsCopied = refreshDirectoryCopy(src, paths.authoredStubDir, &result.message);

  if (!result.authoredStubsCopied) {
    std::printf("[editor] %s\n", result.message.c_str());
    std::printf("[editor] run `make luals-stubs` to generate authored stubs\n");
    return result;
  }

  result.lualsConfigWritten = writeLuaLSConfigFile(paths, &result.message);
  if (!result.lualsConfigWritten)
    std::printf("[editor] %s\n", result.message.c_str());

  return result;
}

} // namespace app
