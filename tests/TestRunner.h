#pragma once

#include <cstdio>

#define ANSI_GREEN  "\033[32m"
#define ANSI_RED    "\033[31m"
#define ANSI_CYAN   "\033[36m"
#define ANSI_YELLOW "\033[1;33m"
#define ANSI_BOLD   "\033[1m"
#define ANSI_RESET  "\033[0m"

#define TEST(name)  printf("\n" ANSI_YELLOW "%s" ANSI_RESET "\n", name)

#define SUITE(name) printf("\n" ANSI_BOLD ANSI_CYAN "======== %s ========" ANSI_RESET, name)

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
