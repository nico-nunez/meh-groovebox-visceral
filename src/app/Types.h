#pragma once

#include <cstddef>
#include <cstdint>

#define RESULT_FIELDS                                                                              \
  bool ok = true;                                                                                  \
  const char* err = nullptr;

#define MAKE_UINT8_ARR(name, size)                                                                 \
  uint8_t name##_data[size] = {};                                                                  \
  Uint8Arr name = {name##_data, size}

#define MAKE_UINT32_ARR(name, size)                                                                \
  uint8_t name##_data[size] = {};                                                                  \
  Uint32Arr name = {name##_data, size}

#define CHECK_RESULT(exp)                                                                          \
  {                                                                                                \
    auto res = (exp);                                                                              \
    if (!res.ok)                                                                                   \
      return (exp);                                                                                \
  }

namespace app {

// =================
// Function return
// =================

struct VoidResult {
  RESULT_FIELDS
};

struct FloatResult {
  float value{};
  RESULT_FIELDS
};

struct DoubleResult {
  double value{};
  RESULT_FIELDS
};

struct IntResult {
  int value{};
  RESULT_FIELDS
};

struct Uint8Result {
  uint8_t value{};
  RESULT_FIELDS
};

struct Uint16Result {
  uint16_t value{};
  RESULT_FIELDS
};

struct Uint32Result {
  uint32_t value{};
  RESULT_FIELDS
};

struct Uint64Result {
  uint32_t value{};
  RESULT_FIELDS
};

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
