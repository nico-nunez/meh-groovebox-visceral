#pragma once

#include "app/display/SynthDisplayState.h"

namespace app::display {

void drawSynthSummarySection(const SynthSummarySnapshot& snapshot);
void drawSynthDisplay(const SynthSummarySnapshot& snapshot);
void drawSynthOverviewTab(const SynthSummarySnapshot& snapshot);
void drawSynthOscTab(const SynthSummarySnapshot& snapshot);
void drawSynthFilterTab(const SynthSummarySnapshot& snapshot);
void drawSynthAmpEnvTab(const SynthSummarySnapshot& snapshot);
void drawSynthVoiceTab(const SynthSummarySnapshot& snapshot);
void drawSynthFXTab(const SynthSummarySnapshot& snapshot);

} // namespace app::display
