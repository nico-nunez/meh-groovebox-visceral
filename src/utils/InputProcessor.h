#pragma once

#include <string>

namespace app::session {
struct SynthSession;
using hSynthSession = SynthSession*;
} // namespace app::session

namespace synth {
struct Engine;

namespace utils {
void parseCommand(const std::string& line, Engine& engine, app::session::hSynthSession session);

} // namespace utils

} // namespace synth
