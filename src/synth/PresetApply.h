#pragma once

#include "synth/Preset.h"

#include <string>
#include <vector>

namespace synth {
struct Engine;
}

namespace synth::preset {

// ============================================================
// Apply: Preset → Engine (VoicePool + ParamRouter)
// ============================================================

struct ApplyResult {
  std::vector<std::string> warnings;
};

ApplyResult applyPreset(const Preset& preset, Engine& engine);

// ============================================================
// Capture: Engine → Preset
// ============================================================

Preset capturePreset(const Engine& engine);

} // namespace synth::preset
