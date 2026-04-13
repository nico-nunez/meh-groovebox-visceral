#pragma once

#include <cstddef>
#include <cstdio>
#include <string_view>

namespace meh::utils {

inline bool strscpy(char* dst, const char* src, size_t dstSize) {
  if (!dst || dstSize == 0)
    return false;

  if (!src) {
    dst[0] = '\0';
    return false;
  }

  size_t i = 0;
  for (; i + 1 < dstSize && src[i] != '\0'; i++)
    dst[i] = src[i];

  dst[i] = '\0';
  return src[i] == '\0';
}

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

} // namespace meh::utils
