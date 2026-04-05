#pragma once

#include "Preset.h"

#include <string>
#include <vector>

namespace synth::preset {

// ============================================================
// Load result — wraps DeserializeResult + file-level info
// ============================================================

struct LoadResult {
  Preset preset;
  std::vector<std::string> warnings;
  std::string error;    // Fatal: file not found, read error, or JSON parse error
  std::string filePath; // Resolved path that was loaded

  bool ok() const { return error.empty(); }
};

// ============================================================
// File I/O
// ============================================================

// Save preset to file. Creates parent directories if needed.
// Returns empty string on success, or error message on failure.
std::string savePreset(const Preset& preset, const std::string& path);

// Load preset from an exact file path.
LoadResult loadPreset(const std::string& path);

// Load preset by name — searches user/ then factory/ directories.
// If name contains '/' or ends with ".json", treated as exact path.
// Otherwise searches: ~/.meh-synth/presets/user/<name>.json
//                     ~/.meh-synth/presets/factory/<name>.json
//                     presets/factory/<name>.json (dev fallback)
LoadResult loadPresetByName(const std::string& name);

// ============================================================
// Directory management
// ============================================================

// Returns ~/.meh-synth/presets/
std::string getPresetsDir();

// Returns ~/.meh-synth/presets/user/
std::string getUserPresetsDir();

// Returns ~/.meh-synth/presets/factory/
std::string getFactoryPresetsDir();

// Ensure preset directories exist. Call once at startup.
// Returns false if directory creation fails.
bool ensurePresetDirs();

// ============================================================
// Listing
// ============================================================

struct PresetEntry {
  std::string name; // Filename without .json extension
  std::string path; // Full path
  bool isFactory;   // true = factory/, false = user/
};

// List all available presets (factory + user).
std::vector<PresetEntry> listPresets();

} // namespace synth::preset
