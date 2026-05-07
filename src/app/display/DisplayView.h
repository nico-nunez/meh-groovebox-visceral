#pragma once

#include "app/display/DisplayState.h"

namespace app {
struct AppContext;
}

namespace app::display {

void drawDisplayDashboard(AppContext& app, const DisplayDashboardSnapshot& snapshot);

void drawTransportSection(AppContext& app, const TransportSnapshot& snapshot);
void drawTrackSection(AppContext& app, const TrackSnapshot& snapshot);
void drawMixerSection(const MixerSnapshot& snapshot);
void drawSequencerPatternSection(const SequencerPatternSnapshot& snapshot);
void drawDocumentStatusSection(const DocumentStatusSnapshot& snapshot);
void drawMIDIRoutingSection(const MIDIRoutingSnapshot& snapshot);
void drawKeyboardMIDIHelpSection();

void drawSynthView(AppContext& app, const DisplayDashboardSnapshot& snapshot);
void drawMixerView(AppContext& app, const DisplayDashboardSnapshot& snapshot);
void drawSequencerView(AppContext& app, const DisplayDashboardSnapshot& snapshot);
void drawTransportView(AppContext& app, const DisplayDashboardSnapshot& snapshot);
void drawRoutingView(AppContext& app, const DisplayDashboardSnapshot& snapshot);

} // namespace app::display
