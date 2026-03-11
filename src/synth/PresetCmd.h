#pragma once

#include "synth/Engine.h"

#include <sstream>

namespace synth::preset {
// ============================================================
// Process Preset Input Command (terminal) Helper
// ============================================================
void processPresetCmd(std::istringstream& iss, Engine& engine);

} // namespace synth::preset
