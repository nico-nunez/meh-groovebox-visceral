#pragma once

#include <cstddef>
#include <cstdint>

#define RESULT_FIELDS                                                                              \
  bool ok = true;                                                                                  \
  const char* err = nullptr;

#define DEFINE_VALUE_RESULT(T, V, Name)                                                            \
  struct Name##Result {                                                                            \
    T value = (V);                                                                                 \
    bool ok = true;                                                                                \
    const char* err = nullptr;                                                                     \
  }

#define MAKE_UINT8_ARR(name, size)                                                                 \
  uint8_t name##_data[size] = {};                                                                  \
  Uint8Arr name = {name##_data, size}

#define MAKE_UINT32_ARR(name, size)                                                                \
  uint8_t name##_data[size] = {};                                                                  \
  Uint32Arr name = {name##_data, size}

#define CHECK_RESULT(exp)                                                                          \
  do {                                                                                             \
    auto res = (exp);                                                                              \
    if (!res.ok)                                                                                   \
      return (exp);                                                                                \
  } while (0)

namespace app {

// =================
// Function return
// =================

struct VoidResult {
  RESULT_FIELDS
};

DEFINE_VALUE_RESULT(float, 0.0f, Float);
DEFINE_VALUE_RESULT(double, 0.0, Double);
DEFINE_VALUE_RESULT(int, 0, Int);
DEFINE_VALUE_RESULT(uint8_t, 0, UInt8);
DEFINE_VALUE_RESULT(uint16_t, 0, UInt16);
DEFINE_VALUE_RESULT(uint32_t, 0, Uint32);
DEFINE_VALUE_RESULT(uint64_t, 0, Uint64);

// ==============
// Arrays
// ==============

struct Uint8Arr {
  uint8_t* data;
  size_t size;
};

struct Uint32Arr {
  uint8_t* data;
  size_t size;
};

} // namespace app
