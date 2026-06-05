#include "KeyProcessor.h"

#include "app/AppContext.h"
#include "app/FileWatchApply.h"
#include "app/display/DisplayState.h"
#include "app/display/DisplayView.h"

#include "device_io/MidiCapture.h"
#include "synth/events/Events.h"

#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_NONE
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>

#include <cctype>
#include <cstddef>
#include <cstdint>

namespace app::utils {
namespace evt = synth::events;

enum class MainView {
  Synth,
  Mixer,
  Sequencer,
  Transport,
  Routing,
  Editor,
};

static GLFWwindow* g_window = nullptr;
static MainView g_activeView = MainView::Synth;

void requestQuit() {
  if (g_window) {
    glfwSetWindowShouldClose(g_window, GLFW_TRUE);
  }
}

static bool currentViewAcceptsKeyboardMIDI() {
  return g_activeView != MainView::Editor;
}

static void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int mods) {
  if (action == GLFW_REPEAT) {
    return;
  }

  auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));

  const bool commandModifier = (mods & GLFW_MOD_SUPER) != 0 || (mods & GLFW_MOD_CONTROL) != 0;
  if (action == GLFW_PRESS && commandModifier) {
    if (key == GLFW_KEY_1) {
      g_activeView = MainView::Synth;
      return;
    }
    if (key == GLFW_KEY_2) {
      g_activeView = MainView::Mixer;
      return;
    }
    if (key == GLFW_KEY_3) {
      g_activeView = MainView::Sequencer;
      return;
    }
    if (key == GLFW_KEY_4) {
      g_activeView = MainView::Transport;
      return;
    }
    if (key == GLFW_KEY_5) {
      g_activeView = MainView::Routing;
      return;
    }
    if (key == GLFW_KEY_6) {
      g_activeView = MainView::Editor;
      return;
    }
  }

  ImGuiIO& io = ImGui::GetIO();
  if (io.WantCaptureKeyboard) {
    return;
  }

  if (!currentViewAcceptsKeyboardMIDI())
    return;

  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_ESCAPE) {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
      return;
    }

    if (g_activeView == MainView::Editor) {
      return;
    }

    uint8_t note = asciiToMidi(static_cast<char>(tolower(key)));
    if (note == 0) {
      return;
    }

    evt::MIDIEvent event{};
    event.type = evt::MIDIEvent::Type::NoteOn;
    event.data.noteOn = {note, 127};
    event.channel = MIDI_CHANNEL_UNASSIGNED;
    pushMIDIEvent(ctx, event);
    return;
  }

  if (action == GLFW_RELEASE) {
    if (key == GLFW_KEY_Z || key == GLFW_KEY_X) {
      return;
    }

    if (g_activeView == MainView::Editor) {
      return;
    }

    uint8_t note = asciiToMidi(static_cast<char>(tolower(key)));
    if (note == 0) {
      return;
    }

    evt::MIDIEvent event{};
    event.type = evt::MIDIEvent::Type::NoteOff;
    event.data.noteOff = {note, 0};
    event.channel = MIDI_CHANNEL_UNASSIGNED;
    pushMIDIEvent(ctx, event);
  }
}

static void drawFeatureView(AppContext& ctx) {
  const auto snapshot = app::display::makeDisplayDashboardSnapshot(ctx);

  switch (g_activeView) {
  case MainView::Synth:
    app::display::drawSynthView(ctx, snapshot);
    break;
  case MainView::Mixer:
    app::display::drawMixerView(ctx, snapshot);
    break;
  case MainView::Sequencer:
    app::display::drawSequencerView(ctx, snapshot);
    break;
  case MainView::Transport:
    app::display::drawTransportView(ctx, snapshot);
    break;
  case MainView::Routing:
    app::display::drawRoutingView(ctx, snapshot);
    break;
  case MainView::Editor:
    app::display::drawEditorView(ctx);
    break;
  }
}

static void drawViewButton(const char* label, MainView view) {
  const bool active = g_activeView == view;
  ImGui::BeginDisabled(active);
  if (ImGui::Button(label))
    g_activeView = view;
  ImGui::EndDisabled();
}

static void drawViewSwitcher() {
  drawViewButton("Synth", MainView::Synth);
  ImGui::SameLine();
  drawViewButton("Mixer", MainView::Mixer);
  ImGui::SameLine();
  drawViewButton("Sequencer", MainView::Sequencer);
  ImGui::SameLine();
  drawViewButton("Transport", MainView::Transport);
  ImGui::SameLine();
  drawViewButton("Routing", MainView::Routing);
  ImGui::SameLine();
  drawViewButton("Editor", MainView::Editor);
}

int startGLFWLoop(AppContext* ctx, hMidiSession midiSessionPtr) {
  if (!glfwInit()) {
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  g_window = glfwCreateWindow(1280, 800, "Meh Synth", nullptr, nullptr);
  if (!g_window) {
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(g_window);
  glfwSwapInterval(1);

  glfwSetWindowUserPointer(g_window, ctx);
  glfwSetKeyCallback(g_window, keyCallback);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  ImGuiStyle& style = ImGui::GetStyle();
  ImVec4* colors = style.Colors;
  // colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.0f); // #1a1a1a
  // colors[ImGuiCol_ChildBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
  // colors[ImGuiCol_FrameBg] = ImVec4(0.08f, 0.08f, 0.08f, 1.0f); // editor text area
  colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.0f);
  colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.0f);
  colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f); // editor text area
  colors[ImGuiCol_FrameBgHovered] = ImVec4(0.14f, 0.14f, 0.14f, 1.0f);
  colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
  colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
  colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
  colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.30f, 0.30f, 1.0f);
  colors[ImGuiCol_ButtonActive] = ImVec4(0.87f, 0.45f, 0.22f, 1.0f); // orange accent
  colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
  colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
  colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.0f);
  colors[ImGuiCol_TextDisabled] = ImVec4(0.40f, 0.40f, 0.40f, 1.0f);

  ImGuiIO& io = ImGui::GetIO();
  ImFont* font = io.Fonts->AddFontFromFileTTF("/System/Library/Fonts/Menlo.ttc", 14.0f);
  if (font) {
    io.FontDefault = font;
  }

  ImGui_ImplGlfw_InitForOpenGL(g_window, true);
  ImGui_ImplOpenGL3_Init("#version 150");

  while (!glfwWindowShouldClose(g_window)) {
    glfwPollEvents();

    app::pollExternalSessionFileApply(ctx);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoSavedSettings;
    ImGui::Begin("Meh Groovebox", nullptr, flags);
    drawViewSwitcher();
    ImGui::Separator();

    ImGui::BeginChild("MainView", ImVec2(0.0f, 0.0f), false);
    drawFeatureView(*ctx);
    ImGui::EndChild();
    ImGui::End();

    // ImGui::ShowDemoWindow();

    // ImGui::PopStyleVar(3);
    ImGui::Render();

    int w = 0;
    int h = 0;
    glfwGetFramebufferSize(g_window, &w, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(g_window);
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwDestroyWindow(g_window);
  g_window = nullptr;
  glfwTerminate();

  if (midiSessionPtr) {
    device_io::stopMidiSession(midiSessionPtr);
    device_io::cleanupMidiSession(midiSessionPtr);
  }

  return 0;
}

uint8_t asciiToMidi(char key) {
  static constexpr uint8_t SEMITONES = 12;
  static uint8_t octiveOffset = 0;

  uint8_t midiKey = 0;

  // Change Octive
  if (key == 122) { // ('z')
    --octiveOffset;
  }

  if (key == 120) { // ('x')
    ++octiveOffset;
  }

  // Change Velocity
  // 99  // ('c')
  // 118 // ('v')

  switch (key) {
  case 97: //  ('a') "C"
    midiKey = 60;
    break;
  case 119: // ('w') "C#"
    midiKey = 61;
    break;
  case 115: // ('s') "D"
    midiKey = 62;
    break;
  case 101: // ('e') "D#"
    midiKey = 63;
    break;
  case 100: // ('d') "E"
    midiKey = 64;
    break;
  case 102: // ('f') "F"
    midiKey = 65;
    break;
  case 116: // ('t') "F#"
    midiKey = 66;
    break;
  case 103: // ('g') "G"
    midiKey = 67;
    break;
  case 121: // ('y') "G#"
    midiKey = 68;
    break;
  case 104: // ('h') "A"
    midiKey = 69;
    break;
  case 117: // ('u') "A#"
    midiKey = 70;
    break;
  case 106: // ('j') "B"
    midiKey = 71;
    break;
  case 107: // ('k') "C"
    midiKey = 72;
    break;
  case 111: // ('o') "C#"
    midiKey = 73;
    break;
  case 108: // ('l') "D"
    midiKey = 74;
    break;
  case 112: // ('p') "D#"
    midiKey = 75;
    break;

  default:
    return 0; // unmapped key
  }

  return midiKey + (octiveOffset * SEMITONES);
}

} // namespace app::utils
