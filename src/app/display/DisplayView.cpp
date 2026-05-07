#include "app/display/DisplayView.h"

#include "app/AppContext.h"
#include "app/ControlEvents.h"
#include "app/display/SynthDisplayView.h"

#include "imgui.h"

namespace app::display {
namespace {

const char* transportModeLabel(app::transport::TransportMode mode) {
  switch (mode) {
  case app::transport::TransportMode::Stopped:
    return "stopped";
  case app::transport::TransportMode::Playing:
    return "playing";
  case app::transport::TransportMode::Paused:
    return "paused";
  }
  return "unknown";
}

const char* lualsStatusLabel(app::editor::LanguageServiceStatus status) {
  switch (status) {
  case app::editor::LanguageServiceStatus::Unavailable:
    return "unavailable";
  case app::editor::LanguageServiceStatus::Idle:
    return "idle";
  case app::editor::LanguageServiceStatus::Pending:
    return "pending";
  case app::editor::LanguageServiceStatus::Running:
    return "running";
  case app::editor::LanguageServiceStatus::Succeeded:
    return "succeeded";
  case app::editor::LanguageServiceStatus::Failed:
    return "failed";
  }
  return "unknown";
}

const char* midiTypeLabel(uint8_t type) {
  const auto typed = static_cast<synth::MIDIEvent::Type>(type);
  switch (typed) {
  case synth::MIDIEvent::Type::NoteOn:
    return "note on";
  case synth::MIDIEvent::Type::NoteOff:
    return "note off";
  default:
    return "event";
  }
}

void drawQueueResult(const app::VoidResult& result) {
  if (!result.ok && result.err)
    ImGui::Text("control error: %s", result.err);
}

} // namespace

void drawTransportSection(AppContext& app, const TransportSnapshot& snapshot) {
  ImGui::SeparatorText("Transport");
  ImGui::Text("mode: %s", transportModeLabel(snapshot.mode));
  ImGui::Text("bpm: %.2f", static_cast<double>(snapshot.bpm));
  ImGui::Text("beat: %.3f", snapshot.beatPosition);
  ImGui::Text("position: bar %llu beat %u",
              static_cast<unsigned long long>(snapshot.musical.bar),
              snapshot.musical.beat);
  ImGui::Text("sample: %llu", static_cast<unsigned long long>(snapshot.samplePosition));

  if (ImGui::Button("Play"))
    drawQueueResult(app::pushControlEvent(&app, app::events::createPlayEvent()));
  ImGui::SameLine();
  if (ImGui::Button("Pause"))
    drawQueueResult(app::pushControlEvent(&app, app::events::createPauseEvent()));
  ImGui::SameLine();
  if (ImGui::Button("Stop"))
    drawQueueResult(app::pushControlEvent(&app, app::events::createStopEvent()));

  static float bpmEdit = app::transport::DEFAULT_BPM;
  static float lastSnapshotBpm = app::transport::DEFAULT_BPM;
  static bool bpmEditing = false;
  if (!bpmEditing && snapshot.bpm != lastSnapshotBpm) {
    bpmEdit = snapshot.bpm;
    lastSnapshotBpm = snapshot.bpm;
  }
  ImGui::SetNextItemWidth(120.0f);
  ImGui::InputFloat("BPM", &bpmEdit, 1.0f, 10.0f, "%.2f");
  bpmEditing = ImGui::IsItemActive();
  if (ImGui::IsItemDeactivatedAfterEdit()) {
    drawQueueResult(app::pushControlEvent(&app, app::events::createBPMEvent(bpmEdit)));
    lastSnapshotBpm = bpmEdit;
    bpmEditing = false;
  }

  // static float bpmEdit = app::transport::DEFAULT_BPM;
  // bpmEdit = snapshot.bpm;
  // ImGui::SetNextItemWidth(120.0f);
  // if (ImGui::InputFloat("BPM",
  //                       &bpmEdit,
  //                       1.0f,
  //                       10.0f,
  //                       "%.2f",
  //                       ImGuiInputTextFlags_EnterReturnsTrue)) {
  //   drawQueueResult(app::pushControlEvent(&app, app::events::createBPMEvent(bpmEdit)));
  // }
}

void drawTrackSection(AppContext& app, const TrackSnapshot& snapshot) {
  ImGui::SeparatorText("Selected Track");
  ImGui::Text("current: %u", snapshot.selectedTrack);
  ImGui::Text("midi sticky: %s",
              snapshot.midiStickyTrack == MIDI_CHANNEL_UNASSIGNED ? "unassigned" : "assigned");
  ImGui::Text("midi follows current track: %s", snapshot.midiFollowsCurrentTrack ? "yes" : "no");

  int selected = static_cast<int>(snapshot.selectedTrack);
  if (ImGui::SliderInt("Current Track", &selected, 0, MAX_TRACKS - 1)) {
    drawQueueResult(app::pushControlEvent(&app,
                                          app::events::createCurrentTrackEvent(
                                              static_cast<uint8_t>(selected))));
  }
}

void drawMixerSection(const MixerSnapshot& snapshot) {
  ImGui::SeparatorText("Selected Track Mixer");
  ImGui::Text("enabled: %s", snapshot.enabled ? "yes" : "no");
  ImGui::Text("gain: %.3f", static_cast<double>(snapshot.gain));
  ImGui::Text("pan: %.3f", static_cast<double>(snapshot.pan));
}

void drawSequencerPatternSection(const SequencerPatternSnapshot& snapshot) {
  ImGui::SeparatorText("Selected Track Sequencer Pattern");
  ImGui::Text("active slot: %s",
              snapshot.activeSlot == app::sequencer::INVALID_PATTERN_SLOT ? "none" : "assigned");
  ImGui::Text("has active pattern: %s", snapshot.hasActivePattern ? "yes" : "no");
  ImGui::Text("length: %u steps", snapshot.numSteps);
  ImGui::Text("steps per beat: %u", snapshot.stepsPerBeat);
  ImGui::Text("active steps: %u", snapshot.activeStepCount);
  ImGui::Text("note-on steps: %u", snapshot.noteOnStepCount);
  ImGui::Text("p-locks: %u", snapshot.paramLockCount);

  ImGui::TextUnformatted("occupied slots:");
  for (uint8_t i = 0; i < app::sequencer::PATTERNS_PER_LANE; ++i) {
    ImGui::SameLine();
    ImGui::Text("%u:%s", i + 1, snapshot.occupiedSlots[i] ? "x" : "-");
  }
}

void drawDocumentStatusSection(const DocumentStatusSnapshot& snapshot) {
  ImGui::SeparatorText("Document / Apply Status");
  ImGui::Text("dirty: %s", snapshot.dirty ? "yes" : "no");
  ImGui::Text("apply revision: %llu", static_cast<unsigned long long>(snapshot.applyRevision));
  ImGui::Text("last applied: %llu", static_cast<unsigned long long>(snapshot.lastAppliedRevision));
  ImGui::Text("backend diagnostics: %u", snapshot.backendDiagnosticCount);
  ImGui::Text("LuaLS diagnostics: %u", snapshot.lualsDiagnosticCount);
  ImGui::Text("LuaLS: %s", lualsStatusLabel(snapshot.lualsStatus));
}

void drawMIDIRoutingSection(const MIDIRoutingSnapshot& snapshot) {
  ImGui::SeparatorText("MIDI Routing");
  ImGui::Text("sticky track: %s",
              snapshot.stickyTrack == MIDI_CHANNEL_UNASSIGNED ? "unassigned" : "assigned");
  ImGui::Text("follows current track: %s", snapshot.followsCurrentTrack ? "yes" : "no");

  ImGui::TextUnformatted("channel map:");
  for (uint8_t ch = 0; ch < MAX_MIDI_CHANNELS; ++ch) {
    const uint8_t mapped = snapshot.channelMap[ch];
    ImGui::Text("ch %u -> %s", ch + 1, mapped == MIDI_CHANNEL_UNASSIGNED ? "follow" : "track");
    if (mapped != MIDI_CHANNEL_UNASSIGNED) {
      ImGui::SameLine();
      ImGui::Text("%u", mapped);
    }
  }

  const MIDIRuntimeTelemetry& midi = snapshot.selectedTrackActivity;
  if (midi.hasLastEvent) {
    ImGui::Text("last: %s note %u vel %u ch %u count %u",
                midiTypeLabel(midi.lastType),
                midi.lastNote,
                midi.lastVelocity,
                midi.lastChannel == MIDI_CHANNEL_UNASSIGNED ? 0 : midi.lastChannel + 1,
                midi.eventCounter);
  } else {
    ImGui::TextUnformatted("last: none");
  }
}

void drawKeyboardMIDIHelpSection() {
  ImGui::SeparatorText("Keyboard MIDI Help");
  ImGui::TextUnformatted(
      "Display view maps keyboard keys to MIDI when ImGui is not capturing input.");
  ImGui::TextUnformatted("z/x change octave; a w s e d f t g y h u j k o l p play notes.");
  ImGui::TextUnformatted("Cmd+1/Ctrl+1 Display, Cmd+2/Ctrl+2 Editor, Esc quit.");
}

void drawDisplayDashboard(AppContext& app, const DisplayDashboardSnapshot& snapshot) {
  ImGui::SeparatorText("Display");
  ImGui::TextUnformatted("Meh Groovebox");
  drawTransportSection(app, snapshot.transport);
  drawTrackSection(app, snapshot.track);
  drawMixerSection(snapshot.mixer);
  drawSynthSummarySection(snapshot.synth);
  drawSynthDisplay(snapshot.synth);
  drawSequencerPatternSection(snapshot.sequencer);
  drawDocumentStatusSection(snapshot.document);
  drawMIDIRoutingSection(snapshot.midi);
  drawKeyboardMIDIHelpSection();
}

void drawSynthView(AppContext&, const DisplayDashboardSnapshot& snapshot) {
  ImGui::SeparatorText("Synth");
  drawSynthSummarySection(snapshot.synth);
  drawSynthDisplay(snapshot.synth);
}

void drawMixerView(AppContext& app, const DisplayDashboardSnapshot& snapshot) {
  ImGui::SeparatorText("Mixer");
  drawTrackSection(app, snapshot.track);
  drawMixerSection(snapshot.mixer);
}

void drawSequencerView(AppContext& app, const DisplayDashboardSnapshot& snapshot) {
  ImGui::SeparatorText("Sequencer");
  drawTrackSection(app, snapshot.track);
  drawSequencerPatternSection(snapshot.sequencer);
}

void drawTransportView(AppContext& app, const DisplayDashboardSnapshot& snapshot) {
  ImGui::SeparatorText("Transport");
  drawTransportSection(app, snapshot.transport);
}

void drawRoutingView(AppContext&, const DisplayDashboardSnapshot& snapshot) {
  ImGui::SeparatorText("Routing");
  drawMIDIRoutingSection(snapshot.midi);
  drawKeyboardMIDIHelpSection();
}
} // namespace app::display
