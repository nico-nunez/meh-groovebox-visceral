# Quick Start

## Table of Contents
- [System Requirements](#system-requirements)
- [Running the App](#running-the-app)
- [Layout & View Switching](#layout--view-switching)
- [Connecting a MIDI Controller](#connecting-a-midi-controller)
- [Keyboard MIDI Input](#keyboard-midi-input)
- [MIDI Routing](#midi-routing)
- [The Editor](#the-editor)
  - [Internal vs External Editing](#internal-vs-external-editing)
  - [File Controls](#file-controls)
  - [Try the Demo Patch](#try-the-demo-patch)
- [The Lua REPL and `help()`](#the-lua-repl-and-help)
- [Using an External Editor (Autocomplete/Diagnostics)](#using-an-external-editor-autocompletediagnostics)
- [Where Things Live on Disk](#where-things-live-on-disk)

## System Requirements

Developed and tested on **Apple Silicon (M1+), macOS 15.0+**. The build
itself doesn't pin an architecture or a minimum OS version (no
`-mmacosx-version-min`, and the bundled GLFW lib is a universal x86_64/arm64
binary), so it may well build and run on Intel Macs or older macOS — that's
just untested, not unsupported. If you try one of those and it works (or
doesn't), that's useful to know.

## Running the App

```bash
./main
```

You can also launch with an explicit session file:

```bash
./main /path/to/some_patch.lua
```

Run it from anywhere — the repo isn't required. Everything the app needs
(your session file, the demo patch, LuaLS type stubs) lives under
`~/.config/groovebox/` and is created/refreshed there automatically on
launch.

On startup you'll see:
- A MIDI device prompt in the terminal, *before* the GUI window appears (see
  [Connecting a MIDI Controller](#connecting-a-midi-controller)).
- A GUI window (title bar "Meh Groovebox").
- An interactive Lua REPL in the *same terminal* you launched from — both run
  concurrently. Type `help()` there any time (see below).

## Layout & View Switching

The GUI has one window with six views, switchable via the button row at the
top or these shortcuts (`Cmd` on macOS, `Ctrl` also works):

| Shortcut | View |
|---|---|
| `Cmd+1` | Synth |
| `Cmd+2` | Mixer |
| `Cmd+3` | Sequencer |
| `Cmd+4` | Transport |
| `Cmd+5` | Routing |
| `Cmd+6` | Editor |

`Esc` quits the app.

## Connecting a MIDI Controller

Before the GUI window appears, the app scans available MIDI sources and
prompts you in the terminal:

```
1. Axiom 49
2. Axiom 49
Enter midi device number:
```

Type the number of your device and hit enter. If no MIDI devices are found,
the app logs `No MIDI devices found` and continues without one — you can
still use [Keyboard MIDI Input](#keyboard-midi-input) below.

A few gotchas:
- **A single physical controller can show up as more than one option.**
  Many USB MIDI interfaces expose one CoreMIDI source per port/cable, so a
  keyboard with multiple I/O ports (or a combined DIN + USB setup) can list
  twice under the same name. If one entry doesn't respond, try the other —
  e.g. on an Axiom 49, entry "1" works and "2" doesn't.
- **Only one device can be connected at a time**, and only at startup —
  there's currently no in-app way to reconnect or switch devices without
  restarting the app.
- If you answer with an out-of-range number, the app logs an error and
  continues with no MIDI device connected (again, restart to retry).

## Keyboard MIDI Input

Outside the Editor view, the QWERTY keyboard doubles as a MIDI controller
(only while ImGui isn't capturing keyboard focus, e.g. not while typing in a
text field):

```
a w s e d f t g y h u j k o l p
```
maps chromatically starting at middle C (`a` = C, `w` = C#, `s` = D, ...).
`z` / `x` shift the octave down/up.

This is disabled while the Editor view is active, since typing there needs
the keys.

## MIDI Routing

By default, incoming MIDI (from a connected controller or the QWERTY
keyboard) routes to whichever track is currently selected. Two REPL commands
override that (see also `help("midi")`):

- **Sticky track** — pin all MIDI input to one track regardless of what's
  selected in the GUI:
  ```
  midi.sticky(3)     -- all MIDI goes to track 3
  midi.unsticky()    -- back to following the selected track
  ```
- **Per-channel routing** — map specific MIDI channels (1-16) to specific
  tracks, e.g. to play several tracks from one keyboard split across
  channels:
  ```
  midi.channel(1, 2)   -- channel 1 -> track 2
  midi.unchannel(1)    -- channel 1 back to selected/sticky track
  midi.routes()        -- print the current sticky + channel routing table
  ```

The **Routing** view (`Cmd+5`) shows the current sticky/channel state and the
last MIDI event received per track, but is read-only — make routing changes
with the REPL commands above.

## The Editor

### Internal vs External Editing

The Editor view has an "Internal" / "External" toggle:

- **Internal**: edit the authored document directly in the built-in text
  buffer, then click **Apply** to submit it.
- **External**: point any text editor (Neovim, VS Code, whatever you like —
  nothing is hardcoded) at the session file's path. The app watches the file
  on disk and re-applies automatically whenever it changes. There's no
  requirement to use a specific editor or plugin.

### File Controls

Above the buffer:
- **Path** — shows the currently loaded file's path.
- **New** / **New Template** — start a blank or minimally-scaffolded document.
- **Open** / **Save** / **Save As** / **Reload** — standard file operations
  against the path shown above.
- **Load Demo** — see below.

### Try the Demo Patch

Click **Load Demo** to load a working 5-track example (kick, hi-hat, tom,
lead, pad) into the buffer, showcasing oscillators, mod matrix routing, FX
chains, unison, and tempo-synced effects, without having to build a patch
from scratch first. The demo content is bundled into the binary and written
to `~/.config/groovebox/demo_session.lua` on every launch.

Loading the demo does **not** change your current file path — clicking
**Save** afterward requires **Save As**, so you can't accidentally overwrite
the bundled example. Click **Apply** to hear it.

## The Lua REPL and `help()`

The terminal REPL (running alongside the GUI) accepts live Lua commands —
parameter changes, preset commands, routing, BPM, etc. Type:

```
help()
```

for an overview of top-level commands and functions, or `help("topic")` for
a detailed reference on a specific area:

| Topic | Covers |
|---|---|
| `params` | Parameter groups and addressing |
| `mod` | Modulation matrix |
| `fm` | FM routing |
| `preset` | Preset load/save commands |
| `fx` | Effects chain commands |
| `signal` | Signal chain / routing |
| `midi` | MIDI sticky/channel routing |

Tab-completion works for global functions in the REPL.

## Using an External Editor (Autocomplete/Diagnostics)

If you use "External" editing mode with an LSP-aware editor (Neovim +
`lua-language-server`, VS Code + Lua extension, etc.), you get autocomplete
and inline diagnostics for the authored-document API (`track()`,
`TrackSettings`, `synth()`, `mixer()`, etc.) for free — **no manual setup
required, and no repo needed**. The type-stub content is generated from the
same compiled-in metadata as the app itself, so on every launch it:

1. Writes the type-stub file to
   `~/.config/groovebox/generated/authored_document/`.
2. Writes `~/.config/groovebox/.luarc.json` pointing your LSP at that
   directory.

Point your editor at the session file inside `~/.config/groovebox/` (or open
that folder as your editor's workspace) and the `.luarc.json` there will be
picked up automatically.

If completions aren't showing up:
- Confirm the app printed no `[editor] ...` warning on startup.
- Confirm your editor's Lua LSP is actually running and has picked up
  `.luarc.json` (most editors auto-discover it if you open the containing
  folder).

## Where Things Live on Disk

| Path | What |
|---|---|
| `~/.config/groovebox/session.lua` | Your current authored document (default, unless overridden — see [Running the App](#running-the-app)) |
| `~/.config/groovebox/last_session` | Path of the last file you had open; reused on next launch |
| `~/.config/groovebox/demo_session.lua` | Bundled demo patch, rewritten from the binary on every launch |
| `~/.config/groovebox/.luarc.json` | Auto-generated LSP config for external editors |
| `~/.config/groovebox/generated/authored_document/` | Auto-generated LuaLS type stubs |
| `~/.meh_synth_history` | REPL command history |
