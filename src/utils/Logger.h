#pragma once

#include <cstdio>
#include <string_view>

namespace app::utils {
void Log(std::string_view message);

// Printf-style logging with automatic flush for real-time output
// Overload for format string with arguments
template <typename... Args> void LogF(const char* format, Args... args) {
  printf(format, args...);
  fflush(stdout);
}

// Overload for format string without arguments (avoids security warning)
inline void LogF(const char* message) {
  printf("%s", message);
  fflush(stdout);
}

} // namespace app::utils
