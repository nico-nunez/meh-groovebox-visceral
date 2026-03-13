# Preset Authoring Reference

A complete reference for crafting presets for the meh-synth-wave synthesizer. This document describes the synth's capabilities, every configurable parameter, and the JSON preset format.

## Table of Contents
- [Synth Capabilities](#synth-capabilities)
- [Synth Limitations](#synth-limitations)
- [JSON Preset Format](#json-preset-format)
- [Parameter Reference](#parameter-reference)
  - [Oscillators](#oscillators)
  - [Noise](#noise)
  - [Envelopes](#envelopes)
  - [Filters](#filters)
  - [Saturator](#saturator)
  - [LFOs](#lfos)
  - [Modulation Matrix](#modulation-matrix)
  - [Signal Chain](#signal-chain)
  - [Voice](#voice)
- [String Value Reference](#string-value-reference)
- [Annotated Example Preset](#annotated-example-preset)
- [Init Preset](#init-preset)

---

## Synth Capabilities

### Sound Generation
- **4 wavetable oscillators** (osc1–osc4), each independently configurable
- **5 factory wavetable banks**: sine, saw, square, triangle, sine_to_saw (sine-to-saw morph)
- **Wavetable scanning**: smoothly interpolate between frames within a bank via `scanPos`
- **FM synthesis**: any oscillator can frequency-modulate any other oscillator's phase, including self-feedback
- **FM ratio**: frequency multiplier per oscillator (0.5x–16x) for precise carrier:modulator tuning
- **Noise oscillator**: white noise (pink noise type exists but is not yet wired to params)
- **Per-oscillator enable/disable**: unused oscillators cost nothing; mix gain auto-normalizes

### Modulation
- **3 global LFOs** with wavetable waveforms (same banks as oscillators) or Sample & Hold mode
- **3 envelopes**: amplitude, filter, and general-purpose mod envelope
- **Envelope curve shaping**: per-stage curve parameter (concave/linear/convex) via precomputed lookup tables
- **Mod matrix**: up to 16 routes, any source → any destination with bipolar amount
- **Mod sources**: ampEnv, filterEnv, modEnv, lfo1–lfo3, velocity, noise, modWheel, keyTrack
- **Mod destinations**: filter cutoff/resonance (both filters), oscillator pitch/mix/scanPos/fmDepth (all 4 oscs), LFO rate/amplitude (all 3 LFOs)

### Filtering & Effects
- **SVF (State Variable Filter)**: lowpass, highpass, bandpass, notch modes
- **Ladder filter**: Moog-style 4-pole lowpass with drive/saturation
- **Saturator**: soft-clip distortion with drive and wet/dry mix
- **Configurable signal chain**: processors can be reordered (default: SVF → Ladder → Saturator)
- **Stereo output**: per-voice panning, stereo filter processing (independent L/R state)

### Voice Architecture
- **64-voice polyphony** (pool-based allocation)
- **Mono mode**: single-voice with last-note priority, legato or hard retrigger
- **Portamento**: exponential pitch glide between notes, legato-only or always-on
- **Unison**: 1–8 sub-voices per voice slot with symmetric detune, stereo spread, and gain compensation

### Control
- **MIDI**: note on/off, velocity, pitch bend, mod wheel (CC1), sustain pedal (CC64)
- **MIDI CC bindings**: CC7 → master gain, CC74 → SVF cutoff, CC71 → SVF resonance
- **Velocity** is available as a mod source (per-voice, 0.0–1.0)

---

## Synth Limitations

Things the synth **cannot** do — useful context for realistic preset design:

- **No effects beyond saturation** — no reverb, delay, chorus, or phaser (planned for step 10)
- **No tempo sync** — LFO rates are in Hz only, no BPM-synced subdivisions
- **No ring modulation** — oscillators can FM each other but not multiply signals
- **No hard sync** — oscillator phase reset from another oscillator is not implemented
- **No sample playback** — wavetable-only; no .wav file loading
- **No user-imported wavetables** — limited to the 5 factory banks
- **No per-voice LFOs** — LFOs are global (all voices share the same LFO phase)
- **No oscillator phase randomization** — all voices start at phase 0; can cause comb filtering on pads
- **No pulse width modulation** — square wave is fixed 50% duty cycle
- **No aftertouch** — channel pressure is not parsed
- **No per-oscillator unison count** — unison applies uniformly to all 4 oscillators
- **No fixed-frequency oscillator mode** — all oscillators track MIDI pitch (can't set an absolute Hz)
- **No filter envelope amount as a direct param** — filter modulation depth is set via mod matrix route amount (e.g., `filterEnv → svf.cutoff` with amount 2.0)
- **Pink noise** type exists in code but is not yet wirable via params

---

## JSON Preset Format

Presets are JSON files with the `.json` extension. Every field must be present (full snapshot — no implicit defaults).

### Top-Level Structure

```json
{
  "version": 1,
  "metadata": { ... },
  "oscillators": {
    "osc1": { ... },
    "osc2": { ... },
    "osc3": { ... },
    "osc4": { ... },
    "noise": { ... }
  },
  "envelopes": { "ampEnv": { ... }, "filterEnv": { ... }, "modEnv": { ... } },
  "filters": { "svf": { ... }, "ladder": { ... } },
  "fx": { "saturator": { ... } },
  "lfos": { "lfo1": { ... }, "lfo2": { ... }, "lfo3": { ... } },
  "modMatrix": [ ... ],
  "signalChain": [ ... ],
  "voice": {
    "pitchBend": { ... },
    "mono": { ... },
    "portamento": { ... },
    "unison": { ... }
  }
}
```

**Key structural notes:**
- `noise` is nested inside `oscillators`, not at the root
- `saturator` is nested inside `fx`, not at the root
- `mono`, `portamento`, `unison`, and `pitchBend` are all nested inside `voice`

---

## Parameter Reference

### Oscillators

Each oscillator (osc1–osc4) has the same fields:

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `bank` | string | see [banks](#wavetable-banks) | `"sine"` | Wavetable bank name |
| `scanPos` | float | 0.0 – 1.0 | 0.0 | Position within the wavetable (frame interpolation) |
| `mixLevel` | float | 0.0 – 4.0 | 1.0 | Output level (>1.0 overdrives into chain) |
| `fmDepth` | float | 0.0 – 5.0 | 0.0 | FM modulation intensity from the FM source |
| `fmRatio` | float | 0.5 – 16.0 | 1.0 | Frequency multiplier (carrier:modulator ratio) |
| `fmSource` | string | see [FM sources](#fm-sources) | `"none"` | Which oscillator modulates this one's phase |
| `octaveOffset` | int | -2 – 2 | 0 | Octave transpose in semitone steps of 12 |
| `detuneAmount` | float | -100.0 – 100.0 | 0.0 | Pitch offset in cents |
| `enabled` | bool | | true | Whether the oscillator is active |

**Notes:**
- `mixLevel` feeds into the signal chain. Values above 1.0 can overdrive filters/saturator for harmonic content.
- `scanPos` only matters for multi-frame banks (currently only `sine_to_saw`). For single-frame banks (sine, saw, square, triangle), `scanPos` has no audible effect.
- `fmRatio` multiplies the oscillator's frequency. At ratio 2.0, the oscillator runs an octave above its MIDI pitch. This is applied before FM modulation — it determines the carrier or modulator frequency relationship.
- Mix gain is auto-normalized: `1.0 / enabledOscCount`. If only osc1 is enabled, it gets full level. If all 4 are enabled, each gets 0.25x before the per-voice gain stage.
- Per-voice gain is fixed at `1/8` (0.125) — this is a system constant, not a preset parameter.

### Noise

Noise is serialized inside the `oscillators` JSON object (alongside osc1–osc4).

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `mixLevel` | float | 0.0 – 1.0 | 0.0 | Noise output level |
| `type` | string | `"white"`, `"pink"` | `"white"` | Noise color |
| `enabled` | bool | | false | Whether noise is active |

**Note:** Pink noise type is defined but not fully wired. Use `"white"` for now.

### Envelopes

Three envelopes share the same field structure: `ampEnv` (amplitude), `filterEnv` (filter modulation), `modEnv` (general purpose modulation).

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `attackMs` | float | 0.0 – 10000.0 | 10.0 | Attack time in milliseconds |
| `decayMs` | float | 0.0 – 10000.0 | 100.0 | Decay time in milliseconds |
| `sustainLevel` | float | 0.0 – 1.0 | 0.7 | Sustain level (1.0 = full) |
| `releaseMs` | float | 0.0 – 10000.0 | 200.0 | Release time in milliseconds |
| `attackCurve` | float | -10.0 – 10.0 | -5.0 | Attack curve shape |
| `decayCurve` | float | -10.0 – 10.0 | -5.0 | Decay curve shape |
| `releaseCurve` | float | -10.0 – 10.0 | -5.0 | Release curve shape |

**Curve parameter guide:**
- **Negative values** (e.g., -5.0): concave/natural feel — fast initial change, slow approach to target. This is the most common shape for musical envelopes.
- **0.0**: linear ramp
- **Positive values** (e.g., 5.0): convex/punchy — slow initial change, fast snap to target. Useful for percussive attacks.
- Typical range: -8.0 to 8.0. Values near ±10 are extreme.

**How envelopes are used:**
- `ampEnv`: always active, directly controls voice amplitude (0.0–1.0). This is not routable — it's hardwired.
- `filterEnv` and `modEnv`: only produce output when routed via the mod matrix. By themselves they do nothing — you must add a mod route like `filterEnv → svf.cutoff` with a non-zero amount.

### Filters

#### SVF (State Variable Filter)

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `mode` | string | `"lp"`, `"hp"`, `"bp"`, `"notch"` | `"lp"` | Filter mode |
| `cutoff` | float | 20.0 – 20000.0 | 1000.0 | Cutoff frequency in Hz |
| `resonance` | float | 0.0 – 1.0 | 0.5 | Resonance (mapped to Q internally) |
| `enabled` | bool | | false | Whether the filter is active |

#### Ladder Filter (Moog-style)

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `cutoff` | float | 20.0 – 20000.0 | 1000.0 | Cutoff frequency in Hz |
| `resonance` | float | 0.0 – 1.0 | 0.3 | Resonance (mapped to 0–4 internally; high values self-oscillate) |
| `drive` | float | 1.0 – 10.0 | 1.0 | Input drive (1.0 = clean, higher = saturated harmonics) |
| `enabled` | bool | | false | Whether the filter is active |

**Notes:**
- Both filters are **disabled by default**. Enable them explicitly.
- Filters process stereo independently (separate L/R state) — no mono-summing artifacts.
- Filter cutoff modulation from the mod matrix is in **octaves** (bipolar ±4). An amount of 2.0 on `filterEnv → svf.cutoff` sweeps the cutoff up by 2 octaves at full envelope output.
- Filter resonance modulation is **linear ±1.0**.

### Saturator

Saturator is serialized inside the `fx` JSON object.

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `drive` | float | 1.0 – 5.0 | 1.0 | Distortion intensity (1.0 = clean) |
| `mix` | float | 0.0 – 1.0 | 1.0 | Wet/dry blend (0.0 = dry, 1.0 = fully saturated) |
| `enabled` | bool | | false | Whether saturation is active |

### LFOs

Three LFOs (lfo1–lfo3) share the same fields:

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `bank` | string | see [banks](#wavetable-banks) + `"sah"` | `"sine"` | Waveform bank or Sample & Hold |
| `rate` | float | 0.0 – 20.0 | 1.0 | Frequency in Hz |
| `amplitude` | float | 0.0 – 1.0 | 1.0 | Output range: -amplitude to +amplitude |
| `retrigger` | bool | | false | Reset LFO phase to 0 on first note-on |

**Notes:**
- `"sah"` (Sample & Hold) outputs a random value that changes at the LFO rate. Useful for random modulation.
- LFOs are **global** — all voices share the same LFO phase. No per-voice phase offset.
- LFOs output a **bipolar** signal: the range is [-amplitude, +amplitude]. The mod matrix `amount` scales this further.
- LFO rate and amplitude are themselves modulatable via the mod matrix (LFO-to-LFO modulation is supported with one-sample feedback delay).
- `retrigger` resets phase on the **first** note-on (when going from 0 active voices to 1), not on every note.

### Modulation Matrix

The `modMatrix` is an array of route objects. Maximum 16 routes.

```json
"modMatrix": [
  { "source": "filterEnv", "destination": "svf.cutoff", "amount": 2.0 },
  { "source": "lfo1", "destination": "osc1.scanPos", "amount": 0.5 }
]
```

| Field | Type | Description |
|-------|------|-------------|
| `source` | string | Mod source name (see [mod sources](#modulation-sources)) |
| `destination` | string | Mod destination name (see [mod destinations](#modulation-destinations)) |
| `amount` | float | Modulation depth (bipolar; range depends on destination) |

**Amount scaling by destination type:**

| Destination type | Amount unit | Typical range | Example |
|-----------------|-------------|---------------|---------|
| Filter cutoff | Octaves | ±4.0 | `2.0` = sweep up 2 octaves |
| Filter resonance | Linear | ±1.0 | `0.3` = slight resonance bump |
| Oscillator pitch | Semitones | ±24.0 | `12.0` = one octave vibrato depth |
| Oscillator mix | Linear | ±1.0 | `0.5` = tremolo |
| Scan position | Normalized | 0.0–1.0 | `0.8` = near-full scan sweep |
| FM depth | Linear | ±5.0 | `1.0` = moderate FM modulation |
| LFO rate | Hz | ±20.0 | `2.0` = rate modulation |
| LFO amplitude | Linear | ±1.0 | `0.5` = amplitude modulation |

**Important:** `filterEnv` and `modEnv` do nothing unless routed. If you want filter envelope modulation, you **must** add a mod route. There is no `envAmount` parameter on the filter — the mod matrix replaces that concept.

### Signal Chain

The `signalChain` array defines the order of audio processors. Each voice's oscillator output passes through these in sequence.

```json
"signalChain": ["svf", "ladder", "saturator"]
```

Valid processor names: `"svf"`, `"ladder"`, `"saturator"`

- Maximum 8 slots
- Processors can be reordered (e.g., `["saturator", "svf"]` for pre-filter distortion)
- A processor in the chain but with `enabled: false` is skipped
- Duplicate entries are allowed (e.g., two saturator stages)

### Voice

All voice behavior fields are grouped under the `"voice"` key in JSON.

#### Pitch Bend

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `range` | float | 0.0 – 48.0 | 2.0 | Pitch bend range in semitones (applied symmetrically up/down) |

**Note:** Master gain is intentionally excluded from presets — it's a session-level control.

#### Mono Mode

| Field | Type | Default | Description |
|-------|------|---------|-------------|
| `enabled` | bool | false | Single-voice mode (last-note priority) |
| `legato` | bool | true | When true: overlapping notes redirect pitch without retriggering envelopes |

#### Portamento

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `time` | float | 0.0 – 5000.0 | 50.0 | Glide time in milliseconds |
| `legato` | bool | | true | When true: only glide when notes overlap |
| `enabled` | bool | | false | Whether portamento is active |

**Note:** Portamento uses exponential decay — the pitch approaches the target asymptotically. Short times (10–50ms) give a subtle slide; long times (500ms+) give dramatic pitch sweeps.

#### Unison

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `voices` | int | 1 – 8 | 4 | Number of sub-voices per voice slot |
| `detune` | float | 0.0 – 100.0 | 20.0 | Total detune spread in cents |
| `spread` | float | 0.0 – 1.0 | 0.5 | Stereo width (0.0 = mono, 1.0 = hard panned) |
| `enabled` | bool | | false | Whether unison is active |

**Notes:**
- Sub-voices are symmetric: they spread evenly around the center pitch. With 4 voices and 20 cents detune, offsets are approximately -10, -3.3, +3.3, +10 cents.
- Gain compensation is `1/sqrt(N)` — so 4 unison voices are at ~0.5x gain each, not 0.25x. This preserves perceived loudness.
- Unison applies to **all** oscillators uniformly — you can't have osc1 with 4 voices and osc2 with 2.
- Setting `voices: 1` effectively disables unison even if `enabled: true`.

---

## String Value Reference

### Wavetable Banks

| String | Description |
|--------|-------------|
| `"sine"` | Pure sine wave (single frame) |
| `"saw"` | Sawtooth wave (single frame, mip-mapped for anti-aliasing) |
| `"square"` | Square wave (single frame, mip-mapped) |
| `"triangle"` | Triangle wave (single frame, mip-mapped) |
| `"sine_to_saw"` | Multi-frame morph from sine to saw — **the only bank where `scanPos` is audible** |

### FM Sources

| String | Description |
|--------|-------------|
| `"none"` | No FM modulation |
| `"osc1"` | Osc1's output modulates this oscillator's phase |
| `"osc2"` | Osc2's output modulates this oscillator's phase |
| `"osc3"` | Osc3's output modulates this oscillator's phase |
| `"osc4"` | Osc4's output modulates this oscillator's phase |

Self-modulation is valid (e.g., `osc1.fmSource = "osc1"`) — uses averaged previous sample for feedback stability.

### SVF Modes

| String | Description |
|--------|-------------|
| `"lp"` | Lowpass — passes frequencies below cutoff |
| `"hp"` | Highpass — passes frequencies above cutoff |
| `"bp"` | Bandpass — passes frequencies around cutoff |
| `"notch"` | Notch/band-reject — removes frequencies around cutoff |

### LFO Banks

Same as [wavetable banks](#wavetable-banks), plus:

| String | Description |
|--------|-------------|
| `"sah"` | Sample & Hold — outputs a new random value at each LFO cycle |

### Modulation Sources

| String | Output Range | Per-voice? | Description |
|--------|-------------|------------|-------------|
| `"ampEnv"` | 0.0 – 1.0 | yes | Amplitude envelope |
| `"filterEnv"` | 0.0 – 1.0 | yes | Filter envelope |
| `"modEnv"` | 0.0 – 1.0 | yes | Mod envelope (general purpose) |
| `"lfo1"` | -amp – +amp | no | LFO 1 (global) |
| `"lfo2"` | -amp – +amp | no | LFO 2 (global) |
| `"lfo3"` | -amp – +amp | no | LFO 3 (global) |
| `"velocity"` | 0.0 – 1.0 | yes | MIDI note-on velocity |
| `"noise"` | -1.0 – 1.0 | no | White noise (global) |
| `"modWheel"` | 0.0 – 1.0 | no | MIDI CC1 mod wheel (global) |
| `"keyTrack"` | octaves from A4 | yes | MIDI note position: A4=0, A3=-1, A5=+1 |

**Key tracking amounts:**
- `amount = 1.0` → full 1:1 tracking (filter cutoff rises/falls at the same rate as pitch)
- `amount = 0.5` → 50% tracking (common for pads — brightens toward high notes but not fully)
- `amount = 0.0` → no tracking (same as not having the route)

Key tracking is most impactful on presets with low filter cutoffs — without it, high notes sound progressively duller because the fixed cutoff sits below the note's upper harmonics.

### Modulation Destinations

| String | Unit | Description |
|--------|------|-------------|
| `"svf.cutoff"` | octaves (±4) | SVF filter cutoff |
| `"ladder.cutoff"` | octaves (±4) | Ladder filter cutoff |
| `"svf.resonance"` | linear (±1) | SVF resonance |
| `"ladder.resonance"` | linear (±1) | Ladder resonance |
| `"osc1.pitch"` | semitones (±24) | Osc1 pitch |
| `"osc2.pitch"` | semitones (±24) | Osc2 pitch |
| `"osc3.pitch"` | semitones (±24) | Osc3 pitch |
| `"osc4.pitch"` | semitones (±24) | Osc4 pitch |
| `"osc1.mixLevel"` | linear (±1) | Osc1 volume |
| `"osc2.mixLevel"` | linear (±1) | Osc2 volume |
| `"osc3.mixLevel"` | linear (±1) | Osc3 volume |
| `"osc4.mixLevel"` | linear (±1) | Osc4 volume |
| `"osc1.scanPos"` | normalized (0–1) | Osc1 wavetable position |
| `"osc2.scanPos"` | normalized (0–1) | Osc2 wavetable position |
| `"osc3.scanPos"` | normalized (0–1) | Osc3 wavetable position |
| `"osc4.scanPos"` | normalized (0–1) | Osc4 wavetable position |
| `"osc1.fmDepth"` | linear (±5) | Osc1 FM intensity |
| `"osc2.fmDepth"` | linear (±5) | Osc2 FM intensity |
| `"osc3.fmDepth"` | linear (±5) | Osc3 FM intensity |
| `"osc4.fmDepth"` | linear (±5) | Osc4 FM intensity |
| `"lfo1.rate"` | Hz (±20) | LFO1 speed |
| `"lfo2.rate"` | Hz (±20) | LFO2 speed |
| `"lfo3.rate"` | Hz (±20) | LFO3 speed |
| `"lfo1.amplitude"` | linear (±1) | LFO1 depth |
| `"lfo2.amplitude"` | linear (±1) | LFO2 depth |
| `"lfo3.amplitude"` | linear (±1) | LFO3 depth |

---

## Annotated Example Preset

A dub techno chord stab with filter envelope sweep and LFO wobble:

```json
{
  "version": 1,
  "metadata": {
    "name": "Dub Techno Stab",
    "author": "",
    "category": "Techno",
    "description": "Filtered chord stab with slow LFO movement"
  },

  "oscillators": {
    "osc1": {
      "bank": "saw",
      "scanPos": 0.0,
      "mixLevel": 1.0,
      "fmDepth": 0.0,
      "fmRatio": 1.0,
      "fmSource": "none",
      "octaveOffset": 0,
      "detuneAmount": 8.0,
      "enabled": true
    },
    "osc2": {
      "bank": "saw",
      "scanPos": 0.0,
      "mixLevel": 0.8,
      "fmDepth": 0.0,
      "fmRatio": 1.0,
      "fmSource": "none",
      "octaveOffset": 0,
      "detuneAmount": -8.0,
      "enabled": true
    },
    "osc3": {
      "bank": "sine",
      "scanPos": 0.0,
      "mixLevel": 0.4,
      "fmDepth": 0.0,
      "fmRatio": 1.0,
      "fmSource": "none",
      "octaveOffset": -1,
      "detuneAmount": 0.0,
      "enabled": true
    },
    "osc4": {
      "bank": "sine",
      "scanPos": 0.0,
      "mixLevel": 0.0,
      "fmDepth": 0.0,
      "fmRatio": 1.0,
      "fmSource": "none",
      "octaveOffset": 0,
      "detuneAmount": 0.0,
      "enabled": false
    },
    "noise": { "mixLevel": 0.0, "type": "white", "enabled": false }
  },

  "envelopes": {
    "ampEnv": {
      "attackMs": 5.0,
      "decayMs": 400.0,
      "sustainLevel": 0.0,
      "releaseMs": 300.0,
      "attackCurve": -3.0,
      "decayCurve": -6.0,
      "releaseCurve": -5.0
    },
    "filterEnv": {
      "attackMs": 2.0,
      "decayMs": 500.0,
      "sustainLevel": 0.1,
      "releaseMs": 200.0,
      "attackCurve": -2.0,
      "decayCurve": -7.0,
      "releaseCurve": -5.0
    },
    "modEnv": {
      "attackMs": 10.0,
      "decayMs": 100.0,
      "sustainLevel": 0.7,
      "releaseMs": 200.0,
      "attackCurve": -5.0,
      "decayCurve": -5.0,
      "releaseCurve": -5.0
    }
  },

  "filters": {
    "svf": { "mode": "lp", "cutoff": 600.0, "resonance": 0.55, "enabled": true },
    "ladder": { "cutoff": 1000.0, "resonance": 0.3, "drive": 1.0, "enabled": false }
  },

  "fx": {
    "saturator": { "drive": 1.5, "mix": 0.4, "enabled": true }
  },

  "lfos": {
    "lfo1": { "bank": "sine", "rate": 0.3, "amplitude": 1.0, "retrigger": false },
    "lfo2": { "bank": "sine", "rate": 1.0, "amplitude": 1.0, "retrigger": false },
    "lfo3": { "bank": "sine", "rate": 1.0, "amplitude": 1.0, "retrigger": false }
  },

  "modMatrix": [
    { "source": "filterEnv", "destination": "svf.cutoff", "amount": 2.5 },
    { "source": "lfo1",      "destination": "svf.cutoff", "amount": 0.4 }
  ],

  "signalChain": ["svf", "ladder", "saturator"],

  "voice": {
    "pitchBend":  { "range": 2.0 },
    "mono":       { "enabled": false, "legato": true },
    "portamento": { "time": 50.0, "legato": true, "enabled": false },
    "unison":     { "voices": 4, "detune": 20.0, "spread": 0.5, "enabled": false }
  }
}
```

**What's happening:**
- Two detuned saws (±8 cents) for width, plus a sub sine one octave down for weight
- Fast attack, medium decay, zero sustain → stab character (notes die out even when held)
- SVF lowpass at 600 Hz with filter envelope sweeping up ~2.5 octaves → bright attack that closes
- Slow LFO (0.3 Hz) adding gentle cutoff movement for the dub techno wash
- Light saturation for warmth

---

## Init Preset

The init preset is a clean starting point — one sine oscillator, no filters, no effects, no modulation.

```json
{
  "version": 1,
  "metadata": {
    "name": "Init",
    "author": "",
    "category": "Init",
    "description": "Clean starting point"
  },
  "oscillators": {
    "osc1": {
      "bank": "sine", "scanPos": 0.0, "mixLevel": 1.0,
      "fmDepth": 0.0, "fmRatio": 1.0, "fmSource": "none",
      "octaveOffset": 0, "detuneAmount": 0.0, "enabled": true
    },
    "osc2": {
      "bank": "sine", "scanPos": 0.0, "mixLevel": 0.0,
      "fmDepth": 0.0, "fmRatio": 1.0, "fmSource": "none",
      "octaveOffset": 0, "detuneAmount": 0.0, "enabled": false
    },
    "osc3": {
      "bank": "sine", "scanPos": 0.0, "mixLevel": 0.0,
      "fmDepth": 0.0, "fmRatio": 1.0, "fmSource": "none",
      "octaveOffset": 0, "detuneAmount": 0.0, "enabled": false
    },
    "osc4": {
      "bank": "sine", "scanPos": 0.0, "mixLevel": 0.0,
      "fmDepth": 0.0, "fmRatio": 1.0, "fmSource": "none",
      "octaveOffset": 0, "detuneAmount": 0.0, "enabled": false
    },
    "noise": { "mixLevel": 0.0, "type": "white", "enabled": false }
  },
  "envelopes": {
    "ampEnv": {
      "attackMs": 10.0, "decayMs": 100.0, "sustainLevel": 0.7,
      "releaseMs": 200.0, "attackCurve": -5.0, "decayCurve": -5.0, "releaseCurve": -5.0
    },
    "filterEnv": {
      "attackMs": 10.0, "decayMs": 100.0, "sustainLevel": 0.7,
      "releaseMs": 200.0, "attackCurve": -5.0, "decayCurve": -5.0, "releaseCurve": -5.0
    },
    "modEnv": {
      "attackMs": 10.0, "decayMs": 100.0, "sustainLevel": 0.7,
      "releaseMs": 200.0, "attackCurve": -5.0, "decayCurve": -5.0, "releaseCurve": -5.0
    }
  },
  "filters": {
    "svf": { "mode": "lp", "cutoff": 1000.0, "resonance": 0.5, "enabled": false },
    "ladder": { "cutoff": 1000.0, "resonance": 0.3, "drive": 1.0, "enabled": false }
  },
  "fx": {
    "saturator": { "drive": 1.0, "mix": 1.0, "enabled": false }
  },
  "lfos": {
    "lfo1": { "bank": "sine", "rate": 1.0, "amplitude": 1.0, "retrigger": false },
    "lfo2": { "bank": "sine", "rate": 1.0, "amplitude": 1.0, "retrigger": false },
    "lfo3": { "bank": "sine", "rate": 1.0, "amplitude": 1.0, "retrigger": false }
  },
  "modMatrix": [],
  "signalChain": ["svf", "ladder", "saturator"],
  "voice": {
    "pitchBend":  { "range": 2.0 },
    "mono":       { "enabled": false, "legato": true },
    "portamento": { "time": 50.0, "legato": true, "enabled": false },
    "unison":     { "voices": 4, "detune": 20.0, "spread": 0.5, "enabled": false }
  }
}
```
