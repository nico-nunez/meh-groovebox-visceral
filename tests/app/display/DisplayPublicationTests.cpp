#include "TestRunner.h"

#include "app/AppContext.h"
#include "app/ControlEvents.h"
#include "app/Transport.h"
#include "app/display/DisplayState.h"

#include "synth/events/Events.h"

namespace {

using app::display::DisplayRuntimeTelemetry;

} // namespace

static void test_publication_initial_snapshot_is_default() {
  TEST("publication_initial_snapshot_is_default");

  app::display::DisplayPublication publication{};

  const auto telemetry = app::display::readDisplayRuntimeTelemetry(publication);

  CHECK("sequence default", telemetry.sequence == 0);
  CHECK("transport stopped", telemetry.transport.mode == app::transport::TransportMode::Stopped);
  CHECK("bpm default", telemetry.transport.bpm == app::transport::DEFAULT_BPM);
}

static void test_publication_reads_latest_published_buffer() {
  TEST("publication_reads_latest_published_buffer");

  app::display::DisplayPublication publication{};

  DisplayRuntimeTelemetry first{};
  first.sequence = 1;
  first.transport.bpm = 111.0f;

  DisplayRuntimeTelemetry second{};
  second.sequence = 2;
  second.transport.bpm = 135.0f;

  app::display::publishDisplayRuntimeTelemetry(publication, first);
  CHECK("first sequence", app::display::readDisplayRuntimeTelemetry(publication).sequence == 1);

  app::display::publishDisplayRuntimeTelemetry(publication, second);
  const auto latest = app::display::readDisplayRuntimeTelemetry(publication);

  CHECK("second sequence", latest.sequence == 2);
  CHECK("second bpm", latest.transport.bpm == 135.0f);
}

static void test_publication_flips_between_two_buffers() {
  TEST("publication_flips_between_two_buffers");

  app::display::DisplayPublication publication{};
  const uint32_t initial = publication.readIndex.load();

  DisplayRuntimeTelemetry telemetry{};
  telemetry.sequence = 1;
  app::display::publishDisplayRuntimeTelemetry(publication, telemetry);

  const uint32_t afterFirst = publication.readIndex.load();
  CHECK("first flipped", afterFirst != initial);

  telemetry.sequence = 2;
  app::display::publishDisplayRuntimeTelemetry(publication, telemetry);

  const uint32_t afterSecond = publication.readIndex.load();
  CHECK("second flipped back", afterSecond == initial);
}

static void test_make_runtime_telemetry_copies_transport_and_mixer() {
  TEST("make_runtime_telemetry_copies_transport_and_mixer");

  app::AppContext app{};
  app.transport.mode = app::transport::TransportMode::Playing;
  app.transport.bpm = 128.0f;
  app.transport.beatPosition = 12.5;
  app.transport.samplePosition = 2048;
  app.mixer.tracks[0].enabled = false;
  app.mixer.tracks[0].gain = 0.25f;
  app.mixer.tracks[0].pan = -0.5f;

  const auto telemetry = app::display::makeDisplayRuntimeTelemetry(app);

  CHECK("sequence incremented", telemetry.sequence == 1);
  CHECK("mode copied", telemetry.transport.mode == app::transport::TransportMode::Playing);
  CHECK("bpm copied", telemetry.transport.bpm == 128.0f);
  CHECK("beat copied", telemetry.transport.beatPosition == 12.5);
  CHECK("sample copied", telemetry.transport.samplePosition == 2048);
  CHECK("mixer enabled copied", !telemetry.tracks[0].mixer.enabled);
  CHECK("mixer gain copied", telemetry.tracks[0].mixer.gain == 0.25f);
  CHECK("mixer pan copied", telemetry.tracks[0].mixer.pan == -0.5f);
}

static void test_make_runtime_telemetry_preserves_existing_midi_state() {
  TEST("make_runtime_telemetry_preserves_existing_midi_state");

  app::AppContext app{};
  DisplayRuntimeTelemetry prior{};
  prior.sequence = 4;
  prior.tracks[1].midi.eventCounter = 9;
  prior.tracks[1].midi.lastNote = 60;
  prior.tracks[1].midi.hasLastEvent = true;
  app::display::publishDisplayRuntimeTelemetry(app.displayPublication, prior);

  const auto telemetry = app::display::makeDisplayRuntimeTelemetry(app);

  CHECK("sequence incremented from prior", telemetry.sequence == 5);
  CHECK("midi counter preserved", telemetry.tracks[1].midi.eventCounter == 9);
  CHECK("midi note preserved", telemetry.tracks[1].midi.lastNote == 60);
  CHECK("midi flag preserved", telemetry.tracks[1].midi.hasLastEvent);
}

static void test_record_midi_event_captures_note_on() {
  TEST("record_midi_event_captures_note_on");

  app::display::DisplayPublication publication{};
  synth::MIDIEvent evt{};
  evt.type = synth::MIDIEvent::Type::NoteOn;
  evt.channel = 2;
  evt.data.noteOn.note = 64;
  evt.data.noteOn.velocity = 100;

  app::display::recordDisplayMIDIEvent(publication, 0, evt);
  const auto telemetry = app::display::readDisplayRuntimeTelemetry(publication);

  CHECK("counter", telemetry.tracks[0].midi.eventCounter == 1);
  CHECK("note", telemetry.tracks[0].midi.lastNote == 64);
  CHECK("velocity", telemetry.tracks[0].midi.lastVelocity == 100);
  CHECK("channel", telemetry.tracks[0].midi.lastChannel == 2);
  CHECK("has event", telemetry.tracks[0].midi.hasLastEvent);
}

static void test_push_midi_event_records_only_successful_queue_push() {
  TEST("push_midi_event_records_only_successful_queue_push");

  app::AppContext app{};
  app.currentTrack = 0;

  synth::MIDIEvent evt{};
  evt.type = synth::MIDIEvent::Type::NoteOn;
  evt.channel = app::MIDI_CHANNEL_UNASSIGNED;
  evt.data.noteOn.note = 67;
  evt.data.noteOn.velocity = 127;

  const bool ok = app::pushMIDIEvent(&app, evt);
  const auto telemetry = app::display::readDisplayRuntimeTelemetry(app.displayPublication);

  CHECK("queue push ok", ok);
  CHECK("counter", telemetry.tracks[0].midi.eventCounter == 1);
  CHECK("last note", telemetry.tracks[0].midi.lastNote == 67);
}

static void test_push_midi_event_records_mapped_track() {
  TEST("push_midi_event_records_mapped_track");

  app::AppContext app{};
  app.currentTrack = 0;
  for (uint8_t ch = 0; ch < app::MAX_MIDI_CHANNELS; ++ch)
    app.midiChannelMap[ch] = app::MIDI_CHANNEL_UNASSIGNED;
  app.midiChannelMap[3] = 2;

  synth::MIDIEvent evt{};
  evt.type = synth::MIDIEvent::Type::NoteOn;
  evt.channel = 3;
  evt.data.noteOn.note = 72;
  evt.data.noteOn.velocity = 90;

  CHECK("queue push ok", app::pushMIDIEvent(&app, evt));
  const auto telemetry = app::display::readDisplayRuntimeTelemetry(app.displayPublication);

  CHECK("track zero untouched", telemetry.tracks[0].midi.eventCounter == 0);
  CHECK("mapped track counter", telemetry.tracks[2].midi.eventCounter == 1);
  CHECK("mapped track note", telemetry.tracks[2].midi.lastNote == 72);
}

static void test_format_musical_position_is_one_based() {
  TEST("format_musical_position_is_one_based");

  const auto start = app::transport::formatMusicalPosition(0.0);
  CHECK("bar 1", start.bar == 1);
  CHECK("beat 1", start.beat == 1);
  CHECK("fraction 0", start.beatFraction == 0.0);

  const auto fifthBeat = app::transport::formatMusicalPosition(4.25);
  CHECK("bar 2", fifthBeat.bar == 2);
  CHECK("beat 1", fifthBeat.beat == 1);
  CHECK("fraction", fifthBeat.beatFraction == 0.25);
}

static void test_format_musical_position_clamps_negative_to_start() {
  TEST("format_musical_position_clamps_negative_to_start");

  const auto pos = app::transport::formatMusicalPosition(-2.0);

  CHECK("bar 1", pos.bar == 1);
  CHECK("beat 1", pos.beat == 1);
  CHECK("fraction 0", pos.beatFraction == 0.0);
}

static void test_transport_control_factories_create_expected_types() {
  TEST("transport_control_factories_create_expected_types");

  CHECK("play type", app::events::createPlayEvent().type == app::events::ControlEvent::Type::Play);
  CHECK("pause type",
        app::events::createPauseEvent().type == app::events::ControlEvent::Type::Pause);
  CHECK("stop type", app::events::createStopEvent().type == app::events::ControlEvent::Type::Stop);
}

void runDisplayPublicationTests() {
  SUITE("DisplayPublication");
  test_publication_initial_snapshot_is_default();
  test_publication_reads_latest_published_buffer();
  test_publication_flips_between_two_buffers();
  test_make_runtime_telemetry_copies_transport_and_mixer();
  test_make_runtime_telemetry_preserves_existing_midi_state();
  test_record_midi_event_captures_note_on();
  test_push_midi_event_records_only_successful_queue_push();
  test_push_midi_event_records_mapped_track();
  test_format_musical_position_is_one_based();
  test_format_musical_position_clamps_negative_to_start();
  test_transport_control_factories_create_expected_types();
}
