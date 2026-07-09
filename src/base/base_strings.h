#pragma once

#include "base_core.h"

typedef struct String8 String8;
struct String8 {
  U8* str;
  U64 size;
};

typedef struct String16 String16;
struct String16 {
  U16* str;
  U64 size;
};

typedef struct String32 String32;
struct String32 {
  U32* str;
  U64 size;
};
