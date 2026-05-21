#include "app/editor/LuaLSDiagnostics.h"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

namespace app::editor {
namespace {

constexpr const char* kFixtureFileName = "buffer.lua";

std::string shellQuote(const std::string& value) {
  std::string out = "'";
  for (const char ch : value) {
    if (ch == '\'')
      out += "'\\''";
    else
      out += ch;
  }
  out += "'";
  return out;
}

std::string readCommandOutput(const std::string& command, int& exitCode) {
  std::array<char, 512> buffer{};
  std::string output{};

  FILE* pipe = popen(command.c_str(), "r");
  if (!pipe) {
    exitCode = -1;
    return output;
  }

  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe))
    output += buffer.data();

  exitCode = pclose(pipe);
  return output;
}

bool writeTextFile(const std::filesystem::path& path, const std::string& text) {
  std::ofstream out(path, std::ios::binary);
  if (!out)
    return false;
  out << text;
  return static_cast<bool>(out);
}

std::filesystem::path makeWorkspacePath() {
  const char* tmp = std::getenv("TMPDIR");
  std::filesystem::path base = tmp && tmp[0] != '\0' ? tmp : "/tmp";
  return base / "meh-groovebox-luals-editor";
}

// bool startsWith(const std::string& value, const std::string& prefix) {
//   return value.rfind(prefix, 0) == 0;
// }

std::size_t findFixturePathEnd(const std::string& line) {
  const std::string fixtureSuffix = std::string(kFixtureFileName) + ":";
  const std::size_t fixture = line.find(fixtureSuffix);
  if (fixture == std::string::npos)
    return std::string::npos;

  return fixture + std::strlen(kFixtureFileName);
}

bool parseLuaLSLocationLine(const std::string& line, LuaLSDiagnostic& out) {
  const std::size_t fixturePathEnd = findFixturePathEnd(line);
  if (fixturePathEnd == std::string::npos)
    return false;

  const std::size_t first = line.find(':');
  const std::size_t second = line.find(':', first + 1);
  const std::size_t bracket = line.find('[', second + 1);
  const std::size_t bracketEnd = line.find(']', bracket + 1);
  const std::size_t codeStart = line.rfind('(');
  const std::size_t codeEnd = line.rfind(')');

  if (first == std::string::npos || second == std::string::npos || bracket == std::string::npos ||
      bracketEnd == std::string::npos)
    return false;

  out.line = static_cast<uint32_t>(std::stoul(line.substr(first + 1, second - first - 1)));
  out.column = static_cast<uint32_t>(std::stoul(line.substr(second + 1, bracket - second - 1)));
  out.severity = line.substr(bracket + 1, bracketEnd - bracket - 1);

  const std::size_t messageStart = bracketEnd + 2;
  if (messageStart < line.size())
    out.message = line.substr(messageStart);

  if (codeStart != std::string::npos && codeEnd != std::string::npos && codeEnd > codeStart) {
    out.code = line.substr(codeStart + 1, codeEnd - codeStart - 1);

    if (messageStart < codeStart) {
      out.message = line.substr(messageStart, codeStart - messageStart);
      while (!out.message.empty() && (out.message.back() == ' ' || out.message.back() == '\t')) {
        out.message.pop_back();
      }
    }
  }

  return true;
}

struct WorkerState {
  std::mutex mutex{};
  bool running = false;
  bool hasResult = false;
  uint64_t serial = 0;
  LuaLSRunResult result{};
};

WorkerState gWorker{};

} // namespace

const char* authoredLuaLSLibraryRoot() {
  return "generated/luals/authored_document";
}

std::string findLuaLanguageServerBinary() {
  const char* env = std::getenv("LUALS_BIN");
  if (env && env[0] != '\0' && std::filesystem::exists(env))
    return env;

  const char* candidates[] = {
      "/Users/nico/.local/share/nvim/mason/bin/lua-language-server",
      "/opt/homebrew/bin/lua-language-server",
      "/usr/local/bin/lua-language-server",
  };

  for (const char* candidate : candidates) {
    if (std::filesystem::exists(candidate))
      return candidate;
  }

  return "";
}

std::string stripAnsiEscapes(const std::string& value) {
  std::string out{};
  for (std::size_t i = 0; i < value.size(); ++i) {
    if (value[i] == '\033' && i + 1 < value.size() && value[i + 1] == '[') {
      i += 2;
      while (i < value.size() &&
             !((value[i] >= 'A' && value[i] <= 'Z') || (value[i] >= 'a' && value[i] <= 'z'))) {
        ++i;
      }
      continue;
    }

    out.push_back(value[i]);
  }
  return out;
}

std::vector<LuaLSDiagnostic> parseLuaLSPrettyOutput(const std::string& output) {
  std::vector<LuaLSDiagnostic> diagnostics{};
  std::istringstream lines(output);
  std::string line{};

  while (std::getline(lines, line)) {
    const std::string cleanLine = stripAnsiEscapes(line);

    LuaLSDiagnostic diagnostic{};
    if (parseLuaLSLocationLine(cleanLine, diagnostic))
      diagnostics.push_back(std::move(diagnostic));
  }

  return diagnostics;
}

LuaLSRunResult runLuaLSDiagnosticsForText(const std::string& text) {
  LuaLSRunResult result{};
  const std::string luals = findLuaLanguageServerBinary();
  if (luals.empty()) {
    result.message = "LuaLS not found";
    return result;
  }

  result.lualsFound = true;

  const std::filesystem::path workspace = makeWorkspacePath();
  std::filesystem::remove_all(workspace);
  std::filesystem::create_directories(workspace);

  const std::filesystem::path bufferPath = workspace / kFixtureFileName;
  const std::filesystem::path configPath = workspace / ".luarc.json";
  const std::filesystem::path libraryRoot = std::filesystem::absolute(authoredLuaLSLibraryRoot());

  if (!writeTextFile(bufferPath, text)) {
    result.message = "LuaLS workspace write failed";
    return result;
  }

  std::ostringstream config;
  config << "{\n"
         << "  \"workspace.library\": [\"" << libraryRoot.string() << "\"],\n"
         << "  \"runtime.version\": \"Lua 5.4\",\n"
         << "  \"diagnostics.globals\": [],\n"
         << "  \"diagnostics.disable\": []\n"
         << "}\n";
  if (!writeTextFile(configPath, config.str())) {
    result.message = "LuaLS config write failed";
    return result;
  }

  int exitCode = 0;
  const std::string command = shellQuote(luals) + " --check=" + shellQuote(workspace.string()) +
                              " --checklevel=Warning --check_format=pretty 2>&1";
  const std::string output = readCommandOutput(command, exitCode);

  result.commandSucceeded = exitCode == 0;
  result.diagnostics = parseLuaLSPrettyOutput(output);
  result.message =
      result.diagnostics.empty() ? "LuaLS diagnostics clean" : "LuaLS diagnostics found";
  return result;
}

void maybeStartLuaLSDiagnostics(AuthoredDocEditorState& editor) {
  std::lock_guard<std::mutex> workerLock(gWorker.mutex);
  if (gWorker.running)
    return;

  if (editor.luals.completedSerial == editor.editSerial)
    return;

  const uint64_t serial = editor.editSerial;
  const std::string text = editor.buffer.text;
  editor.luals.status = LanguageServiceStatus::Running;
  editor.luals.requestedSerial = serial;
  editor.luals.running = true;

  gWorker.running = true;
  gWorker.hasResult = false;
  gWorker.serial = serial;

  std::thread([serial, text]() {
    LuaLSRunResult result = runLuaLSDiagnosticsForText(text);
    std::lock_guard<std::mutex> lock(gWorker.mutex);
    gWorker.result = std::move(result);
    gWorker.serial = serial;
    gWorker.hasResult = true;
    gWorker.running = false;
  }).detach();
}

void collectFinishedLuaLSDiagnostics(AuthoredDocEditorState& editor) {
  std::lock_guard<std::mutex> lock(gWorker.mutex);
  if (!gWorker.hasResult)
    return;

  editor.luals.completedSerial = gWorker.serial;
  editor.luals.running = false;
  editor.luals.diagnostics = gWorker.result.diagnostics;
  editor.luals.message = gWorker.result.message;

  if (!gWorker.result.lualsFound)
    editor.luals.status = LanguageServiceStatus::Unavailable;
  else if (!gWorker.result.diagnostics.empty())
    editor.luals.status = LanguageServiceStatus::Failed;
  else
    editor.luals.status = LanguageServiceStatus::Succeeded;

  gWorker.hasResult = false;
}

} // namespace app::editor
