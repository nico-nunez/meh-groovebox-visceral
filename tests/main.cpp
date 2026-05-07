#include "TestRunner.h"

#include <chrono>
#include <cstdio>
#include <cstring>

int g_passed = 0;
int g_failed = 0;
bool g_verbose = false;
bool g_suites = false;
int g_suite_passed = 0;
int g_suite_failed = 0;
const char* g_suite_name = nullptr;

void printSuiteResult() {
  if (!g_suites || !g_suite_name)
    return;
  const bool ok = g_suite_failed == 0;
  if (ok)
    printf("  " ANSI_GREEN "✓ %d passed" ANSI_RESET "\n", g_suite_passed);
  else
    printf("  " ANSI_RED "✗ %d passed, %d failed" ANSI_RESET "\n", g_suite_passed, g_suite_failed);
}

void runDocApplyFileBindingTests();
void runDocAuthoringServiceSeqApplyTests();
void runDocAuthoringServiceLifecycleTests();
void runDocAuthoredSynthEndToEndTests();
void runAuthoredDocEditorTests();

void runDisplayPublicationTests();
void runSynthDisplayStateTests();
void runDisplayDashboardSnapshotTests();

void runDocSequencerParserTests();
void runDocSynthSettingsParserTests();

void runDocSynthPlannerTests();
void runDocSequencerPlannerTests();

void runSequencerPlaybackTests();

void runDocMetadataTests();
void runDocSynthSettingsMetadataTests();
void runMetadataVerificationTests();
void runLuaLSStubGenerationTests();
void runLuaRuntimeMetadataTests();
void runLuaLSDiagnosticsTests();

int main(int argc, char* argv[]) {
  int buildSecs = -1;

  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "-v") == 0)
      g_verbose = true;
    else if (std::strcmp(argv[i], "-s") == 0)
      g_suites = true;
    else if (std::strncmp(argv[i], "--build-secs=", 13) == 0)
      buildSecs = std::atoi(argv[i] + 13);
  }

  auto t0 = std::chrono::steady_clock::now();

  runDocApplyFileBindingTests();
  runDocAuthoringServiceSeqApplyTests();
  runDocAuthoringServiceLifecycleTests();
  runDocAuthoredSynthEndToEndTests();
  runAuthoredDocEditorTests();

  runDisplayPublicationTests();
  runSynthDisplayStateTests();
  runDisplayDashboardSnapshotTests();

  runDocSequencerParserTests();
  runDocSynthSettingsParserTests();

  runDocSynthPlannerTests();
  runDocSequencerPlannerTests();

  runSequencerPlaybackTests();

  runDocMetadataTests();
  runDocSynthSettingsMetadataTests();
  runMetadataVerificationTests();
  runLuaRuntimeMetadataTests();
  runLuaLSStubGenerationTests();
  runLuaLSDiagnosticsTests();

  printSuiteResult();

  auto testMs =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - t0)
          .count();

  const bool ok = g_failed == 0;
  printf("\n%s%s %d passed, %d failed%s\n",
         ok ? ANSI_GREEN : ANSI_RED,
         ok ? "✓" : "✗",
         g_passed,
         g_failed,
         ANSI_RESET);

  if (buildSecs >= 0)
    printf("\033[2m  build %ds · tests %lldms\033[0m\n", buildSecs, (long long)testMs);
  else
    printf("\033[2m  tests %lldms\033[0m\n", (long long)testMs);

  return g_failed > 0 ? 1 : 0;
}
