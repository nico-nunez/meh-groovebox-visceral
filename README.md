# meh-groovebox-visceral

An in-progress standalone groovebox for macOS: a code-driven performance instrument built around a shared clock, pattern-based sequencing, and deep synth engines per part.

This repo started from a standalone synth app. That synth engine still forms the core sound source, but the project goal is now a live-performance groovebox rather than a single playable synth.

## What This Project Is Becoming

- A standalone groovebox for live electronic performance
- A multi-part instrument with a shared transport and tempo-synced behavior
- A code-first performance surface driven by Lua, keyboard input, MIDI, and an eventual visual control layer
- A loop-based system focused on patterns, variation, and real-time manipulation rather than linear song arrangement

## Current State

Right now the repo is in the transition from "standalone synth" to "groovebox runtime."

Already present in the codebase:

- A production-oriented synth engine extracted under [`engine/`](engine/README.md)
- Standalone macOS audio and MIDI runtime built on CoreAudio/CoreMIDI
- Lua REPL control surface for live parameter changes, preset commands, routing commands, and BPM updates
- Basic transport state and action queue
- Track-oriented app scaffolding in the host runtime
- Preset loading/saving infrastructure
- Keyboard and MIDI note input

Important limitations: the host now renders and mixes multiple tracks, and the step sequencer/p-lock runtime is present, but the groovebox layer is still incomplete. Pattern management and switching are still minimal, session recall is not in place, solo/performance-mix behavior is unfinished, and the current ImGui window is still a basic runtime shell rather than a real performance display.

## Planned Groovebox Features

The list below is intentionally written as a project checklist. Items are pending unless marked done.

- [x] Core synth engine foundation
- [x] Standalone audio output and MIDI input
- [x] Lua REPL for live control
- [x] Basic transport/BPM control
- [x] Preset system
- [x] Step sequencer runtime
- [x] Per-step parameter locks
- [x] Multi-track render path and internal mixing
- [ ] Expanded track addressing and direct per-track operations
- [ ] Pattern management, storage, and switching
- [ ] Solo and broader performance-mix controls
- [x] Basic mute / gain / pan / master mix controls
- [ ] Session save and recall for groovebox state
- [ ] Sample playback / drum-part engine
- [ ] Resampling / recording workflow
- [ ] External control protocol for editor/UI clients
- [ ] Performance-oriented visual display

## Build

```bash
make debug
make release
make clean
```

Requirements:

- macOS
- Xcode command line tools

## Run

```bash
./main
```

On startup the app connects to the default audio device, initializes MIDI, and opens the Lua-driven terminal control surface.

## Project Layout

- [`engine/`](engine/README.md): reusable synth engine library
- [`src/app/`](src/app): standalone host runtime, transport, app context, audio/MIDI session wiring
- [`src/lua/`](src/lua): Lua REPL and command bindings
- [`presets/`](presets): factory presets and preset authoring notes
- [`_docs_/architecture/`](_docs_/architecture): groovebox architecture and roadmap docs

## Notes

This README is now groovebox-first on purpose. The synth engine is still a major part of the project, but it is no longer the full product description.
