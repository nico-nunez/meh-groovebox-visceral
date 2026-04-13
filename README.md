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

Important limitation: the host is still effectively single-track at runtime today. The app has track abstractions and shared transport scaffolding, but audio rendering still runs only the currently selected track.

## Planned Groovebox Features

The list below is intentionally written as a project checklist. Items are pending unless marked done.

- [x] Core synth engine foundation
- [x] Standalone audio output and MIDI input
- [x] Lua REPL for live control
- [x] Basic transport/BPM control
- [x] Preset system
- [ ] Single-track step sequencer
- [ ] Per-step parameter locks
- [ ] Multi-part playback and internal mixing
- [ ] Track selection and addressing beyond the current single-track scaffold
- [ ] Pattern creation, editing, and switching
- [ ] Mute / solo / performance mix controls
- [ ] Session save and recall for groovebox state
- [ ] Sample playback / drum-part engine
- [ ] Resampling / recording workflow
- [ ] External control protocol for editor/UI clients
- [ ] ImGui or equivalent performance display

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
