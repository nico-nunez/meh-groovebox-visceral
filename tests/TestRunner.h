#pragma once

#include <cstdio>

#define ANSI_GREEN  "\033[32m"
#define ANSI_RED    "\033[31m"
#define ANSI_CYAN   "\033[36m"
#define ANSI_YELLOW "\033[1;33m"
#define ANSI_BOLD   "\033[1m"
#define ANSI_RESET  "\033[0m"

extern int g_passed;
extern int g_failed;
extern bool g_verbose;
extern bool g_suites;
extern int g_suite_passed;
extern int g_suite_failed;
extern const char* g_suite_name;

void printSuiteResult();

#define SUITE(name)                                                                                \
  do {                                                                                             \
    printSuiteResult();                                                                            \
    g_suite_name = name;                                                                           \
    g_suite_passed = 0;                                                                            \
    g_suite_failed = 0;                                                                            \
    if (g_verbose)                                                                                 \
      printf("\n" ANSI_CYAN "▸ " ANSI_BOLD "%s" ANSI_RESET "\n", name);                            \
    else if (g_suites)                                                                             \
      printf("\n" ANSI_CYAN "▸ " ANSI_BOLD "%s" ANSI_RESET "\n", name);                            \
  } while (0)

#define TEST(name)                                                                                 \
  do {                                                                                             \
    if (g_verbose)                                                                                 \
      printf("\n" ANSI_YELLOW "%s" ANSI_RESET "\n", name);                                         \
  } while (0)

#define CHECK(name, cond)                                                                          \
  do {                                                                                             \
    if (cond) {                                                                                    \
      ++g_passed;                                                                                  \
      ++g_suite_passed;                                                                            \
      if (g_verbose)                                                                               \
        printf("  " ANSI_GREEN "PASS" ANSI_RESET ": %s\n", name);                                  \
    } else {                                                                                       \
      ++g_failed;                                                                                  \
      ++g_suite_failed;                                                                            \
      printf("  " ANSI_RED "FAIL" ANSI_RESET ": %s\n", name);                                      \
    }                                                                                              \
  } while (0)
