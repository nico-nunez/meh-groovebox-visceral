#pragma once

#include "Preset.h"

#include <string>
#include <vector>

namespace synth::preset {

// ============================================================
// Serialize: Preset → JSON string
// ============================================================

// Returns pretty-printed JSON string ready to write to file.
std::string serializePreset(const Preset& preset);

// ============================================================
// Deserialize: JSON string → Preset (with validation)
// ============================================================

struct DeserializeResult {
  Preset preset;
  std::vector<std::string> warnings; // Non-fatal issues (clamped values, unknown enums)
  std::string error;                 // Fatal error (unparseable JSON)

  bool ok() const { return error.empty(); }
};

// Parse JSON string into Preset. On parse failure, result.ok() is false
// and result.error has the message. On success, result.preset is populated
// and result.warnings contains any non-fatal fixups.
DeserializeResult deserializePreset(const std::string& jsonStr);

} // namespace synth::preset
