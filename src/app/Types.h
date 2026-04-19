#pragma once

#include <cstddef>
#include <cstdint>

#define RESULT_FIELDS                                                                              \
  bool ok = false;                                                                                 \
  const char* err = nullptr;

#define MAKE_UINT8_ARR(name, size)                                                                 \
  uint8_t name##_data[size] = {};                                                                  \
  Uint8Arr name = {name##_data, size}

#define MAKE_UINT32_ARR(name, size)                                                                \
  uint8_t name##_data[size] = {};                                                                  \
  Uint32Arr name = {name##_data, size}

namespace app {

struct Result {
  RESULT_FIELDS
};

struct Uint8Arr {
  uint8_t* data;
  size_t size;
};

struct Uint32Arr {
  uint8_t* data;
  size_t size;
};

} // namespace app
