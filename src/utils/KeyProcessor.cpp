#include "KeyProcessor.h"
#include "Logger.h"

#include "synth_io/Events.h"
#include "synth_io/SynthIO.h"

#include "device_io/KeyCapture.h"
#include "device_io/MidiCapture.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>

namespace synth::utils {
namespace d_io = device_io;

namespace s_io = synth_io;

// Handle MIDI device events
static void midiCallback(d_io::MidiEvent devieEvent, void* context) {
  auto sessionPtr = static_cast<hSynthSession>(context);

  // Convert raw device MIDI data to user friendly event
  s_io::MIDIEvent event{};
  event.type = static_cast<s_io::MIDIEvent::Type>(devieEvent.type);
  event.channel = devieEvent.channel;
  event.timestamp = devieEvent.timestamp;

  switch (event.type) {
  case s_io::MIDIEvent::Type::NoteOn:
    event.data.noteOn = {devieEvent.data1, devieEvent.data2};
    break;
  case s_io::MIDIEvent::Type::NoteOff:
    event.data.noteOff = {devieEvent.data1, devieEvent.data2};
    break;
  case s_io::MIDIEvent::Type::ControlChange:
    event.data.cc = {devieEvent.data1, devieEvent.data2};
    break;
  case s_io::MIDIEvent::Type::PitchBend:
    event.data.pitchBend = {devieEvent.pitchBendValue};
    break;
  case s_io::MIDIEvent::Type::ProgramChange:
    event.data.programChange = {devieEvent.data1};
    break;
  case s_io::MIDIEvent::Type::Aftertouch:
    event.data.aftertouch = {devieEvent.data1, devieEvent.data2};
    break;
  case s_io::MIDIEvent::Type::ChannelPressure:
    event.data.channelPressure = {devieEvent.data1};
    break;

  default:
    break;
  }

  s_io::pushMIDIEvent(sessionPtr, event);
}

// Handle keyboard events
static void keyEventCallback(device_io::KeyEvent event, void* userContext) {
  auto sessionPtr = static_cast<hSynthSession>(userContext);

  if (event.type == device_io::KeyEventType::KeyDown) {
    // ESC (quit)
    if (event.keyCode == 53) {
      printf("ESC pressed, stopping...\n");
      device_io::terminateKeyCaptureLoop();
      return;
    }

    uint8_t note = asciiToMidi(event.character);
    if (note == 0)
      return;

    s_io::MIDIEvent midiEvent{};
    midiEvent.type = s_io::MIDIEvent::Type::NoteOn;
    midiEvent.data.noteOn = {note, 127};

    s_io::pushMIDIEvent(sessionPtr, midiEvent);

    // Note "OFF" event
  } else if (event.type == device_io::KeyEventType::KeyUp) {
    uint8_t note = asciiToMidi(event.character);

    // Currently 'z' & 'x' control octive up/down
    // Need to ignore keyup (note off) for now
    if (event.character == 120 || event.character == 122 || note == 0)
      return;

    s_io::MIDIEvent midiEvent{};
    midiEvent.type = s_io::MIDIEvent::Type::NoteOff;
    midiEvent.data.noteOff = {note, 0};

    s_io::pushMIDIEvent(sessionPtr, midiEvent);
  }
}

hMidiSession initMidiSession(hSynthSession sessionPtr) {
  // 1a. Setup MIDI on this thread's run loop for now
  constexpr size_t MAX_MIDI_DEVICES = 16;
  device_io::MidiSource midiSourceBuffer[MAX_MIDI_DEVICES];
  size_t numMidiDevices = device_io::getMidiSources(midiSourceBuffer, MAX_MIDI_DEVICES);

  hMidiSession midiSession = nullptr;

  if (numMidiDevices) {
    // Display MIDI source options
    for (size_t i = 0; i < numMidiDevices; i++) {
      // Display options as 1 based indexing
      printf("%ld. %s\n", i + 1, midiSourceBuffer[i].displayName);
    }

    uint32_t srcIndex;
    LogF("Enter midi device number: ");
    std::cin >> srcIndex;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // Adjust for options starting at 1 (not 0)
    --srcIndex;

    if (srcIndex < 0 || srcIndex >= numMidiDevices) {
      printf("Invalid option selected: %d", srcIndex);
      return midiSession;
    }

    midiSession = device_io::setupMidiSession({}, midiCallback, sessionPtr);

    device_io::connectMidiSource(midiSession, midiSourceBuffer[srcIndex].uniqueID);

    device_io::startMidiSession(midiSession);

  } else {
    synth::utils::LogF("No MIDI devices found\n");
  }

  return midiSession;
}

int startKeyInputCapture(hSynthSession sessionPtr, hMidiSession midiSessionPtr) {
  printf("KeyCapture Example\n");
  printf("------------------\n");
  printf("Press keys to see events. ESC to quit.\n\n");

  // 1. Initialize Cocoa app
  device_io::initKeyCaptureApp();

  // 2. Create a minimal window (required for local capture without permissions)
  device_io::WindowConfig config = device_io::defaultWindowConfig();
  config.title = "Meh Synth";
  config.width = 800;
  config.height = 500;

  if (!createCaptureWindow(config)) {
    printf("Failed to create window\n");
    return 1;
  }

  // 3. Start capturing with local mode (no permissions needed when window
  // focused)
  //    Change to CaptureMode::Global if you need capture when not focused
  //    Change to CaptureMode::Both if you want both behaviors
  if (!startKeyCapture(keyEventCallback, sessionPtr, device_io::CaptureMode::Local)) {
    printf("Failed to start key capture\n");
    return 1;
  }

  const char* windowText = "Super Synth\n\n"
                           "Press 'z' to go down an octive and 'c' to go up an octive\n\n"
                           "================= Keyboard Layout ================\n"
                           "|    |   |   |   |   |   |   |   |   |   |   |   |\n"
                           "|    |   |   |   |   |   |   |   |   |   |   |   |\n"
                           "|    | w |   | E |   |   | T |   | Y |   | U |   |\n"
                           "|    |___|   |___|   |   |___|   |___|   |___|   |\n"
                           "|      |       |     |     |       |       |     |\n"
                           "|      |       |     |     |       |       |     |\n"
                           "|  A   |   S   |  D  |  F  |   G   |   H   |  J  |\n"
                           "|______|_______|_____|_____|_______|_______|_____|\n\n"
                           "Press keys... (ESC to quit)\n";

  // Update window text
  device_io::setWindowText(windowText);

  // 4. Run the event loop (blocks until stopKeyCaptureLoop() called)
  device_io::runKeyCaptureLoop();

  // 5. Cleanup
  device_io::stopKeyCapture();

  if (midiSessionPtr) {
    device_io::stopMidiSession(midiSessionPtr);
    device_io::cleanupMidiSession(midiSessionPtr);
  }

  printf("Done.\n");
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

} // namespace synth::utils
