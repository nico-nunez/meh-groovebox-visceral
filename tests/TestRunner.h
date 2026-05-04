#pragma once

#include <cstdio>

#define ANSI_GREEN "\033[32m"
#define ANSI_RED   "\033[31m"
#define ANSI_RESET "\033[0m"

extern int g_passed;
extern int g_failed;

#define CHECK(name, cond)                                                                          \
  do {                                                                                             \
    if (cond) {                                                                                    \
      ++g_passed;                                                                                  \
      printf("  " ANSI_GREEN "PASS" ANSI_RESET ": %s\n", name);                                    \
    } else {                                                                                       \
      ++g_failed;                                                                                  \
      printf("  " ANSI_RED "FAIL" ANSI_RESET ": %s\n", name);                                      \
    }                                                                                              \
  } while (0)
