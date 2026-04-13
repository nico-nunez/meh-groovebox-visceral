#include "MIDISession.h"

#include "app/AppContext.h"
#include "meh_utils/MehUtils.h"
#include "synth/events/Events.h"

#include <iostream>

namespace app::midi {

namespace {
using d_MIDIEvent = device_io::MidiEvent;
using s_MIDIEvent = synth::events::MIDIEvent;

// Handle MIDI device events
static void midiCallback(d_MIDIEvent deviceEvent, void* userContext) {
  auto* ctx = static_cast<AppContext*>(userContext);

  // Convert raw device MIDI data to user friendly event
  s_MIDIEvent event{};
  event.type = static_cast<s_MIDIEvent::Type>(deviceEvent.type);
  event.channel = deviceEvent.channel;
  event.timestamp = deviceEvent.timestamp;

  switch (event.type) {
  case s_MIDIEvent::Type::NoteOn:
    event.data.noteOn = {deviceEvent.data1, deviceEvent.data2};
    break;
  case s_MIDIEvent::Type::NoteOff:
    event.data.noteOff = {deviceEvent.data1, deviceEvent.data2};
    break;
  case s_MIDIEvent::Type::ControlChange:
    event.data.cc = {deviceEvent.data1, deviceEvent.data2};
    break;
  case s_MIDIEvent::Type::PitchBend:
    event.data.pitchBend = {deviceEvent.pitchBendValue};
    break;
  case s_MIDIEvent::Type::ProgramChange:
    event.data.programChange = {deviceEvent.data1};
    break;
  case s_MIDIEvent::Type::Aftertouch:
    event.data.aftertouch = {deviceEvent.data1, deviceEvent.data2};
    break;
  case s_MIDIEvent::Type::ChannelPressure:
    event.data.channelPressure = {deviceEvent.data1};
    break;

  default:
    break;
  }

  pushMIDIEvent(ctx, event);
}

} // namespace

hMidiSession initSession(AppContext* ctx) {
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
    meh::utils::LogF("Enter midi device number: ");
    std::cin >> srcIndex;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    // Adjust for options starting at 1 (not 0)
    --srcIndex;

    if (srcIndex < 0 || srcIndex >= numMidiDevices) {
      printf("Invalid option selected: %d", srcIndex);
      return midiSession;
    }

    midiSession = device_io::setupMidiSession({}, midiCallback, ctx);

    device_io::connectMidiSource(midiSession, midiSourceBuffer[srcIndex].uniqueID);

    device_io::startMidiSession(midiSession);

  } else {
    meh::utils::LogF("No MIDI devices found\n");
  }

  return midiSession;
}

} // namespace app::midi
