#include "KeyProcessor.h"

#include "app/AppContext.h"

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

static GLFWwindow* g_window = nullptr;

void requestQuit() {
  if (g_window) {
    glfwSetWindowShouldClose(g_window, GLFW_TRUE);
  }
}

static void keyCallback(GLFWwindow* window, int key, int /*scancode*/, int action, int /*mods*/) {
  if (action == GLFW_REPEAT) {
    return;
  }

  auto* ctx = static_cast<AppContext*>(glfwGetWindowUserPointer(window));

  if (action == GLFW_PRESS) {
    if (key == GLFW_KEY_ESCAPE) {
      glfwSetWindowShouldClose(window, GLFW_TRUE);
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

int startGLFWLoop(AppContext* ctx, hMidiSession midiSessionPtr) {
  if (!glfwInit()) {
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

  g_window = glfwCreateWindow(800, 500, "Meh Synth", nullptr, nullptr);
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
  ImGui_ImplGlfw_InitForOpenGL(g_window, true);
  ImGui_ImplOpenGL3_Init("#version 150");

  while (!glfwWindowShouldClose(g_window)) {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin("Meh Synth", nullptr, flags);
    ImGui::TextUnformatted("Super Synth");
    ImGui::TextUnformatted("");
    ImGui::TextUnformatted("Press 'z' to go down an octive and 'x' to go up an octive");
    ImGui::TextUnformatted("");
    ImGui::TextUnformatted("================= Keyboard Layout ================");
    ImGui::TextUnformatted("|    |   |   |   |   |   |   |   |   |   |   |   |");
    ImGui::TextUnformatted("|    |   |   |   |   |   |   |   |   |   |   |   |");
    ImGui::TextUnformatted("|    | w |   | E |   |   | T |   | Y |   | U |   |");
    ImGui::TextUnformatted("|    |___|   |___|   |   |___|   |___|   |___|   |");
    ImGui::TextUnformatted("|      |       |     |     |       |       |     |");
    ImGui::TextUnformatted("|      |       |     |     |       |       |     |");
    ImGui::TextUnformatted("|  A   |   S   |  D  |  F  |   G   |   H   |  J  |");
    ImGui::TextUnformatted("|______|_______|_____|_____|_______|_______|_____|");
    ImGui::TextUnformatted("");
    ImGui::TextUnformatted("Press keys... (ESC to quit)");
    ImGui::End();
    ImGui::PopStyleVar(3);

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
