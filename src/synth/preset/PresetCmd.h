#pragma once

#include <sstream>

namespace synth {
struct Engine;
}

namespace synth::preset {
// ============================================================
// Process Preset Input Command (terminal) Helper
// ============================================================
void processPresetCmd(std::istringstream& iss, Engine& engine);

} // namespace synth::preset
