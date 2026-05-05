#include "TestRunner.h"

#include <cstdio>

int g_passed = 0;
int g_failed = 0;

void runDocSequencerParserTests();
void runDocSequencerPlannerTests();
void runDocAuthoringServiceSeqApplyTests();
void runDocAuthoringServiceLifecycleTests();
void runDocApplyFileBindingTests();

int main() {
  runDocSequencerParserTests();
  runDocSequencerPlannerTests();
  runDocAuthoringServiceSeqApplyTests();
  runDocAuthoringServiceLifecycleTests();
  runDocApplyFileBindingTests();

  const char* summaryColor = g_failed > 0 ? ANSI_RED : ANSI_GREEN;
  printf("\n%s%d passed, %d failed%s\n", summaryColor, g_passed, g_failed, ANSI_RESET);
  return g_failed > 0 ? 1 : 0;
}
