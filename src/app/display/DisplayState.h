#pragma once

#include "app/Constants.h"
#include "app/Sequencer.h"
#include "app/Transport.h"
#include "app/display/SynthDisplayState.h"
#include "app/editor/AuthoredDocEditor.h"

#include <atomic>
#include <cstdint>

namespace app {
struct AppContext;
}

namespace synth::events {
struct MIDIEvent;
}

namespace app::display {
using app::transport::TransportMode;
struct TransportTelemetry {
  TransportMode mode = TransportMode::Stopped;
  float bpm = app::transport::DEFAULT_BPM;
  double beatPosition = 0.0;
  uint64_t samplePosition = 0;
};

struct MIDIRuntimeTelemetry {
  uint32_t eventCounter = 0;
  uint8_t lastNote = 0;
  uint8_t lastVelocity = 0;
  uint8_t lastChannel = MIDI_CHANNEL_UNASSIGNED;
  uint8_t lastType = 0;
  bool hasLastEvent = false;
};

struct MixerRuntimeTelemetry {
  bool enabled = true;
  float gain = 1.0f;
  float pan = 0.0f;
};

struct TrackRuntimeTelemetry {
  SynthRuntimeTelemetry synth{};
  MIDIRuntimeTelemetry midi{};
  MixerRuntimeTelemetry mixer{};
};

struct DisplayRuntimeTelemetry {
  uint64_t sequence = 0;
  TransportTelemetry transport{};
  TrackRuntimeTelemetry tracks[MAX_TRACKS]{};
};

struct DisplayPublication {
  DisplayRuntimeTelemetry buffers[2]{};
  std::atomic<uint32_t> readIndex{0};
};

struct TransportSnapshot {
  app::transport::TransportMode mode = app::transport::TransportMode::Stopped;
  float bpm = app::transport::DEFAULT_BPM;
  double beatPosition = 0.0;
  uint64_t samplePosition = 0;
  app::transport::MusicalPosition musical{};
};

struct TrackSnapshot {
  uint8_t selectedTrack = 0;
  uint8_t midiStickyTrack = MIDI_CHANNEL_UNASSIGNED;
  bool midiFollowsCurrentTrack = true;
};

struct MixerSnapshot {
  bool enabled = true;
  float gain = 1.0f;
  float pan = 0.0f;
};

struct SequencerPatternSnapshot {
  uint8_t activeSlot = app::sequencer::INVALID_PATTERN_SLOT;
  bool occupiedSlots[app::sequencer::PATTERNS_PER_LANE]{};
  bool hasActivePattern = false;
  uint8_t numSteps = 0;
  uint8_t stepsPerBeat = 0;
  uint8_t activeStepCount = 0;
  uint8_t noteOnStepCount = 0;
  uint16_t paramLockCount = 0;
};

struct DocumentStatusSnapshot {
  bool dirty = false;
  app::doc::DocRevision applyRevision = 0;
  app::doc::DocRevision lastAppliedRevision = 0;
  uint32_t backendDiagnosticCount = 0;
  uint32_t lualsDiagnosticCount = 0;
  app::editor::LanguageServiceStatus lualsStatus = app::editor::LanguageServiceStatus::Idle;
};

struct MIDIRoutingSnapshot {
  uint8_t stickyTrack = MIDI_CHANNEL_UNASSIGNED;
  uint8_t channelMap[MAX_MIDI_CHANNELS]{};
  bool followsCurrentTrack = true;
  MIDIRuntimeTelemetry selectedTrackActivity{};
};

struct DisplayControlSnapshot {
  TrackSnapshot track{};
  SequencerPatternSnapshot sequencer{};
  DocumentStatusSnapshot document{};
  MIDIRoutingSnapshot midi{};
};

struct DisplayDashboardSnapshot {
  TransportSnapshot transport{};
  TrackSnapshot track{};
  MixerSnapshot mixer{};
  SynthSummarySnapshot synth{};
  SequencerPatternSnapshot sequencer{};
  DocumentStatusSnapshot document{};
  MIDIRoutingSnapshot midi{};
};

void publishDisplayRuntimeTelemetry(DisplayPublication& publication,
                                    const DisplayRuntimeTelemetry& telemetry);
DisplayRuntimeTelemetry readDisplayRuntimeTelemetry(const DisplayPublication& publication);

DisplayRuntimeTelemetry makeDisplayRuntimeTelemetry(const AppContext& app);
void recordDisplayMIDIEvent(DisplayPublication& publication,
                            uint8_t track,
                            const synth::events::MIDIEvent& evt);

DisplayControlSnapshot makeDisplayControlSnapshot(const AppContext& app);
DisplayDashboardSnapshot mergeDisplaySnapshot(const DisplayControlSnapshot& control,
                                              const DisplayRuntimeTelemetry& runtime);
DisplayDashboardSnapshot makeDisplayDashboardSnapshot(const AppContext& app);

SequencerPatternSnapshot makeSequencerPatternSnapshot(const app::sequencer::SequencerState& seq,
                                                      uint8_t lane);

} // namespace app::display
