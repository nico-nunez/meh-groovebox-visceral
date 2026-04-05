#include "PresetIO.h"
#include "PresetSerializer.h"

#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

namespace synth::preset {

// ============================================================
// Path helpers
// ============================================================

namespace {

std::string getHomeDir() {
  const char* home = std::getenv("HOME");
  return home ? std::string(home) : std::string();
}

// Create directory and parents. Returns true on success or if exists.
bool mkdirp(const std::string& path) {
  struct stat st{};
  if (stat(path.c_str(), &st) == 0)
    return S_ISDIR(st.st_mode);

  // Find parent
  size_t slash = path.rfind('/');
  if (slash != std::string::npos && slash > 0) {
    if (!mkdirp(path.substr(0, slash)))
      return false;
  }

  return mkdir(path.c_str(), 0755) == 0;
}

bool fileExists(const std::string& path) {
  struct stat st{};
  return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

std::string readFile(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open())
    return {};
  std::ostringstream ss;
  ss << file.rdbuf();
  return ss.str();
}

bool writeFile(const std::string& path, const std::string& content) {
  std::ofstream file(path);
  if (!file.is_open())
    return false;
  file << content;
  return file.good();
}

// List .json files in a directory. Returns filenames without extension.
std::vector<std::pair<std::string, std::string>> listJsonFiles(const std::string& dir) {
  std::vector<std::pair<std::string, std::string>> entries;

  DIR* d = opendir(dir.c_str());
  if (!d)
    return entries;

  struct dirent* ent;
  while ((ent = readdir(d)) != nullptr) {
    std::string name = ent->d_name;
    if (name.size() > 5 && name.substr(name.size() - 5) == ".json") {
      std::string stem = name.substr(0, name.size() - 5);
      entries.push_back({stem, dir + "/" + name});
    }
  }

  closedir(d);
  return entries;
}

} // anonymous namespace

// ============================================================
// Directory management
// ============================================================

std::string getPresetsDir() {
  return getHomeDir() + "/.meh-synth/presets";
}

std::string getUserPresetsDir() {
  return getPresetsDir() + "/user";
}

std::string getFactoryPresetsDir() {
  return getPresetsDir() + "/factory";
}

bool ensurePresetDirs() {
  return mkdirp(getUserPresetsDir()) && mkdirp(getFactoryPresetsDir());
}

// ============================================================
// Save
// ============================================================

std::string savePreset(const Preset& preset, const std::string& path) {
  // Ensure parent directory exists
  size_t slash = path.rfind('/');
  if (slash != std::string::npos) {
    std::string dir = path.substr(0, slash);
    if (!mkdirp(dir))
      return "Failed to create directory: " + dir;
  }

  std::string json = serializePreset(preset);

  if (!writeFile(path, json))
    return "Failed to write file: " + path;

  return {}; // success
}

// ============================================================
// Load (exact path)
// ============================================================

LoadResult loadPreset(const std::string& path) {
  LoadResult result;
  result.filePath = path;

  std::string content = readFile(path);
  if (content.empty()) {
    result.error = "File not found or empty: " + path;
    return result;
  }

  auto desResult = deserializePreset(content);
  result.preset = std::move(desResult.preset);
  result.warnings = std::move(desResult.warnings);

  if (!desResult.ok()) {
    result.error = desResult.error;
  }

  return result;
}

// ============================================================
// Load by name (search user/ then factory/)
// ============================================================

LoadResult loadPresetByName(const std::string& name) {
  // If name looks like a path, treat as exact
  if (name.find('/') != std::string::npos ||
      (name.size() > 5 && name.substr(name.size() - 5) == ".json")) {
    return loadPreset(name);
  }

  std::string jsonName = name + ".json";

  // Search order:
  // 1. ~/.meh-synth/presets/user/<name>.json
  std::string userPath = getUserPresetsDir() + "/" + jsonName;
  if (fileExists(userPath))
    return loadPreset(userPath);

  // 2. ~/.meh-synth/presets/factory/<name>.json
  std::string factoryPath = getFactoryPresetsDir() + "/" + jsonName;
  if (fileExists(factoryPath))
    return loadPreset(factoryPath);

  // 3. presets/factory/<name>.json (dev/repo fallback)
  std::string devPath = "presets/factory/" + jsonName;
  if (fileExists(devPath))
    return loadPreset(devPath);

  LoadResult result;
  result.error = "Preset not found: " + name;
  return result;
}

// ============================================================
// List
// ============================================================

std::vector<PresetEntry> listPresets() {
  std::vector<PresetEntry> entries;

  // Factory presets (from ~/.meh-synth/presets/factory/)
  for (const auto& [name, path] : listJsonFiles(getFactoryPresetsDir())) {
    entries.push_back({name, path, true});
  }

  // Dev fallback factory presets (from presets/factory/ in repo)
  // Only add if the above directory was empty or didn't exist
  if (entries.empty()) {
    for (const auto& [name, path] : listJsonFiles("presets/factory")) {
      entries.push_back({name, path, true});
    }
  }

  // User presets
  for (const auto& [name, path] : listJsonFiles(getUserPresetsDir())) {
    entries.push_back({name, path, false});
  }

  return entries;
}

} // namespace synth::preset
