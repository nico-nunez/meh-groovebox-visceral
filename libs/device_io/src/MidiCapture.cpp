#include "device_io/MidiCapture.h"

#include "meh_utils/MehUtils.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreMIDI/CoreMIDI.h>

#include <cstddef>
#include <cstdio>
#include <stdio.h>

namespace device_io {

struct MidiSession {
  // === User-provided ===
  MidiConfig config;
  MidiCallback userCallback = nullptr;
  void* userContext = nullptr;

  // === CoreMIDI handles ===
  MIDIClientRef client = 0;  // Connection to CoreMIDI system
  MIDIPortRef inputPort = 0; // Receives data from connected sources

  // === State ===
  bool running = false; // When false, callback ignores incoming events

  // === Connected sources ===
  // Track what's connected for cleanup and disconnect functionality
  static constexpr size_t MAX_CONNECTED_SOURCES = 16;
  struct ConnectedSource {
    MIDIEndpointRef endpoint = 0;
    MIDIUniqueID uniqueID = 0;
    bool active = false;
  };
  ConnectedSource connectedSources[MAX_CONNECTED_SOURCES] = {};
  size_t connectedSourceCount = 0;

  // === Future: Standalone mode (not needed for piggyback) ===
  // std::thread runLoopThread;
  // CFRunLoopRef runLoop = nullptr;
  // std::atomic<bool> threadRunning{false};
};

void parseAndDispatch(MidiSession* session, const MIDIPacket* packet) {
  const uint8_t* data = packet->data;
  size_t remaining = packet->length;

  while (remaining > 0) {
    uint8_t status = data[0];

    // Skip real-time messages (single byte, can appear anywhere)
    if (status >= 0xF8) {
      // 0xF8 = Clock, 0xFA = Start, 0xFB = Continue, 0xFC = Stop, 0xFE = Active
      // Sensing Handle or ignore based on config
      data++;
      remaining--;
      continue;
    }

    MidiEvent event = {};
    event.timestamp = packet->timeStamp;

    uint8_t messageType = status & 0xF0;
    event.channel = status & 0x0F;

    size_t messageLength = 0;

    switch (messageType) {
    case 0x80: // Note Off
      if (remaining < 3)
        return;
      event.type = MidiEvent::Type::NoteOff;
      event.data1 = data[1]; // note
      event.data2 = data[2]; // velocity
      messageLength = 3;
      break;

    case 0x90: // Note On
      if (remaining < 3)
        return;
      event.data1 = data[1]; // note
      event.data2 = data[2]; // velocity
      // Velocity 0 = Note Off (MIDI convention)
      event.type = (event.data2 > 0) ? MidiEvent::Type::NoteOn : MidiEvent::Type::NoteOff;
      messageLength = 3;
      break;

    case 0xA0: // Polyphonic Aftertouch
      if (remaining < 3)
        return;
      event.type = MidiEvent::Type::Aftertouch;
      event.data1 = data[1]; // note
      event.data2 = data[2]; // pressure
      messageLength = 3;
      break;

    case 0xB0: // Control Change
      if (remaining < 3)
        return;
      event.type = MidiEvent::Type::ControlChange;
      event.data1 = data[1]; // CC number
      event.data2 = data[2]; // value
      messageLength = 3;
      break;

    case 0xC0: // Program Change
      if (remaining < 2)
        return;
      event.type = MidiEvent::Type::ProgramChange;
      event.data1 = data[1]; // program number
      messageLength = 2;
      break;

    case 0xD0: // Channel Pressure
      if (remaining < 2)
        return;
      event.type = MidiEvent::Type::ChannelPressure;
      event.data1 = data[1]; // pressure
      messageLength = 2;
      break;

    case 0xE0: // Pitch Bend
      if (remaining < 3)
        return;
      event.type = MidiEvent::Type::PitchBend;
      event.data1 = data[1]; // LSB
      event.data2 = data[2]; // MSB
      // Convert to signed 14-bit: -8192 to +8191
      event.pitchBendValue = ((int16_t)(data[2] << 7 | data[1])) - 8192;
      messageLength = 3;
      break;

    default:
      // Unknown or system message, skip one byte
      data++;
      remaining--;
      continue;
    }

    // Dispatch to user callback
    if (session->userCallback) {
      session->userCallback(event, session->userContext);
    }

    data += messageLength;
    remaining -= messageLength;
  }
}

MIDIEndpointRef getSourceByUniqueID(MIDIUniqueID targetID) {
  MIDIObjectRef foundObject;
  MIDIObjectType foundType;

  OSStatus status = MIDIObjectFindByUniqueID(targetID, &foundObject, &foundType);
  if (status != noErr) {
    printf("Error finding MIDI object for uniqueID: %d\n", targetID);
    return 0;
  }

  if (status == noErr && foundType == kMIDIObjectType_Source) {
    return (MIDIEndpointRef)foundObject;
  }

  printf("Error, found type is not a source\n");
  return 0;
}

// ==== Navtive Callback ====
void midiInputCallback(const MIDIPacketList* packetList, void* refCon, void*) {
  auto* session = static_cast<MidiSession*>(refCon);

  // Early out if session is stopped
  if (!session || !session->running)
    return;

  const MIDIPacket* packet = &packetList->packet[0];

  for (UInt32 i = 0; i < packetList->numPackets; i++) {
    // NOTE(nico): A packet can contain multiple messages
    parseAndDispatch(session, packet);

    packet = MIDIPacketNext(packet);
  }
}

// ==== Helper Functions ====
int disconnectMidiSourceAtIndex(hMidiSession session, size_t srcIndex) {
  auto src = &session->connectedSources[srcIndex];

  if (!src->active) {
    printf("Error while attempting to disconnect inactive source.\n");
    return 1;
  }

  OSStatus status = MIDIPortDisconnectSource(session->inputPort, src->endpoint);
  if (status != noErr) {
    printf("Error disconnecting source from port: %d\n", status);
    return status;
  }

  size_t swapIndex = --session->connectedSourceCount;
  session->connectedSources[srcIndex] = session->connectedSources[swapIndex];

  session->connectedSources[swapIndex].endpoint = 0;
  session->connectedSources[swapIndex].uniqueID = 0;
  session->connectedSources[swapIndex].active = false;

  return noErr;
}

// ==== APIs ====

size_t getMidiSources(struct MidiSource* srcBuffer, size_t srcBufferCount) {
  size_t numSources = 0;

  ItemCount sourceCount = MIDIGetNumberOfSources();
  meh::utils::LogF("Found %ld MIDI sources\n", sourceCount);

  for (ItemCount i = 0; i < sourceCount && i < srcBufferCount; i++) {
    MIDIEndpointRef source = MIDIGetSource(i);

    CFStringRef name = NULL;
    MIDIObjectGetStringProperty(source, kMIDIPropertyDisplayName, &name);
    if (name) {
      CFStringGetCString(name,
                         srcBuffer[i].displayName,
                         sizeof(srcBuffer[i].displayName),
                         kCFStringEncodingUTF8);
      CFRelease(name);
    }

    MIDIObjectGetIntegerProperty(source, kMIDIPropertyUniqueID, &srcBuffer[i].uniqueID);
    numSources++;
  }

  return numSources;
};

MidiSession* setupMidiSession(const MidiConfig& config, MidiCallback callback, void* userContext) {
  auto session = new MidiSession();

  session->config = config;
  session->userCallback = callback;
  session->userContext = userContext;

  OSStatus status;

  // TODO(nico): add notify callback to allow for hot swappable devices
  status = MIDIClientCreate(CFSTR("Meh Device IO"), NULL, NULL, &session->client);
  if (status != noErr) {
    printf("Error creating MIDI client: %d\n", status);
    delete session;
    return nullptr;
  }

  status = MIDIInputPortCreate(session->client,
                               CFSTR("Meh Input"),
                               midiInputCallback,
                               session,
                               &session->inputPort);
  if (status != noErr) {
    printf("Error creating input port: %d\n", status);
    MIDIClientDispose(session->client);
    delete session;
    return nullptr;
  }

  return session;
}

int connectMidiSource(hMidiSession session, MIDIUniqueID uniqueID) {
  // TODO(nico): validate source

  auto connectedSource = &session->connectedSources[session->connectedSourceCount];

  MIDIEndpointRef source = getSourceByUniqueID(uniqueID);
  if (!source) {
    printf("Error retrieving source uniqueID: %d", uniqueID);
    return -1;
  }

  OSStatus status = MIDIPortConnectSource(session->inputPort, source, NULL);
  if (status != noErr) {
    printf("Error connecting MIDI source to port: %d\n", status);
    return 1;
  }

  connectedSource->active = true;
  connectedSource->endpoint = source;
  connectedSource->uniqueID = uniqueID;

  session->connectedSourceCount++;

  return noErr;
}

int disconnectMidiSource(hMidiSession session, MIDIUniqueID uniqueID) {
  for (size_t i = 0; i < session->connectedSourceCount; i++) {
    if (session->connectedSources[i].uniqueID == uniqueID)
      return disconnectMidiSourceAtIndex(session, i);
  }

  printf("No source found to disconnect from port\n");
  return 1;
}

int disconnectAllMidiSources(hMidiSession session) {
  OSStatus status;

  while (session->connectedSourceCount > 0) {
    status = disconnectMidiSourceAtIndex(session, 0);
    if (status != noErr)
      return status;
  }

  return noErr;
}

int startMidiSession(hMidiSession session) {
  session->running = true;
  return noErr;
}

int stopMidiSession(hMidiSession session) {
  session->running = false;
  return noErr;
}

int cleanupMidiSession(hMidiSession session) {
  OSStatus status;

  status = disconnectAllMidiSources(session);
  if (status != noErr) {
    printf("Unable to disconnect all MIDI sources.\n");
    return status;
  }

  status = MIDIPortDispose(session->inputPort);
  if (status != noErr) {
    printf("Error disposing of MIDI port: %d\n", status);
    return status;
  }

  status = MIDIClientDispose(session->client);
  if (status != noErr) {
    printf("Error disposing of MIDI client: %d\n", status);
    return status;
  }

  delete session;

  return noErr;
}
} // namespace device_io
