#ifndef BASE_CORE_H

#define BASE_CORE_H

#include <stdint.h>

#if defined(_WIN32)
#define OS_WINDOWS 1
#elif defined(__gnu_linux__) || defined(__linux__)
#define OS_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
#define OS_MAC 1
#else
#error This compiler/OS combo is not supported.
#endif

#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#define ARCH_X64 1
#elif defined(i386) || defined(__i386) || defined(__i386__)
#define ARCH_X86 1
#elif defined(__aarch64__)
#define ARCH_ARM64 1
#elif defined(__arm__)
#define ARCH_ARM32 1
#else
#error Architecture not supported.
#endif

#define internal      static
#define global        static
#define local_persist static

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;
typedef S8 B8;
typedef S16 B16;
typedef S32 B32;
typedef S64 B64;
typedef float F32;
typedef double F64;
typedef void VoidProc(void);

#endif // !BASE_CORE_H
