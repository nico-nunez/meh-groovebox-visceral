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
  - [FX Chain](#fx-chain)
  - [Voice](#voice)
- [String Value Reference](#string-value-reference)
  - [Wavetable Banks](#wavetable-banks)
  - [FM Routes](#fm-routes)
  - [Phase Modes](#phase-modes)
  - [SVF Modes](#svf-modes)
  - [Distortion Types](#distortion-types)
  - [LFO Banks](#lfo-banks)
  - [Subdivisions](#subdivisions)
  - [Modulation Sources](#modulation-sources)
  - [Modulation Destinations](#modulation-destinations)
- [Annotated Example Preset](#annotated-example-preset)
- [Init Preset](#init-preset)

---

## Synth Capabilities

### Sound Generation
- **4 wavetable oscillators** (osc1–osc4), each independently configurable
- **5 factory wavetable banks**: sine, saw, square, triangle, sine_to_saw (sine-to-saw morph)
- **Wavetable scanning**: smoothly interpolate between frames within a bank via `scanPos`
- **FM synthesis**: up to 4 FM routes per oscillator (carrier), each with its own source and depth. Any oscillator can modulate any other, including self-feedback. Routes are summed before phase application.
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

### Filtering & Signal Processing
- **SVF (State Variable Filter)**: lowpass, highpass, bandpass, notch modes
- **Ladder filter**: Moog-style 4-pole lowpass with drive/saturation
- **Saturator**: per-voice soft-clip distortion with drive and wet/dry mix (pre-filter, part of signal chain)
- **Configurable signal chain**: processors can be reordered (default: SVF → Ladder → Saturator)
- **Stereo output**: per-voice panning, stereo filter processing (independent L/R state)

### Effects
- **5 post-mix stereo effects**: distortion, chorus, phaser, delay, reverb
- **Configurable FX chain**: effects can be reordered by the user (default: distortion → chorus)
- **Delay**: tempo-sync or free-running, ping-pong, LP/HP damping in feedback path
- **Reverb**: Dattorro plate algorithm with pre-delay, diffusion, modulation, and damping controls

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

- **No ring modulation** — oscillators can FM each other but not multiply signals
- **No hard sync** — oscillator phase reset from another oscillator is not implemented
- **No sample playback** — wavetable-only; no .wav file loading
- **No user-imported wavetables** — limited to the 5 factory banks
- **No per-voice LFOs** — LFOs are global (all voices share the same LFO phase)
- **No per-oscillator unison phase spread for non-unison voices** — phase spread mode only applies to unison sub-voices, not across polyphonic voice slots
- **No pulse width modulation** — square wave is fixed 50% duty cycle
- **No aftertouch** — channel pressure is not parsed
- **No per-oscillator unison count** — unison applies uniformly to all 4 oscillators
- **No filter envelope amount as a direct param** — filter modulation depth is set via mod matrix route amount (e.g., `filterEnv → svf.cutoff` with amount 2.0)

---

## JSON Preset Format

Presets are JSON files with the `.json` extension. Every field must be present (full snapshot — no implicit defaults).

### Top-Level Structure

```json
{
  "version": 1,
  "bpm": 120.0,
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
  "lfos": { "lfo1": { ... }, "lfo2": { ... }, "lfo3": { ... } },
  "modMatrix": [ ... ],
  "signalChain": [ ... ],
  "fxChain": [ ... ],
  "fx": {
    "distortion": { ... },
    "chorus": { ... },
    "phaser": { ... },
    "delay": { ... },
    "reverb": { ... }
  },
  "voice": {
    "saturator": { ... },
    "pitchBend": { ... },
    "mono": { ... },
    "portamento": { ... },
    "unison": { ... }
  }
}
```

**Key structural notes:**
- `noise` is nested inside `oscillators`, not at the root
- `saturator` is nested inside `voice` — it is a per-voice, pre-filter processor, not a post-mix effect
- `mono`, `portamento`, `unison`, `pitchBend`, and `saturator` are all nested inside `voice`
- `fxChain` is a top-level ordering array (like `signalChain`) — it controls the processing order of the effects
- `fx` holds the parameter objects for each effect; an effect's parameters are applied regardless of whether it appears in `fxChain`
- `master.gain` is intentionally absent — it is a session-level control, not a per-preset sound design parameter

---

## Parameter Reference

### Oscillators

Each oscillator (osc1–osc4) has the same fields:

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `bank` | string | see [banks](#wavetable-banks) | `"sine"` | Wavetable bank name |
| `scanPos` | float | 0.0 – 1.0 | 0.0 | Position within the wavetable (frame interpolation) |
| `mixLevel` | float | 0.0 – 4.0 | 1.0 | Output level (>1.0 overdrives into chain) |
| `fmRoutes` | array | see [FM routes](#fm-routes) | `[]` | FM modulation inputs — up to 4 routes, each with source + depth |
| `fmDepth` | float | 0.0 – 2.0 | 1.0 | Global FM depth multiplier — scales all routes together (1.0 = neutral) |
| `ratio` | float | 0.5 – 16.0 | 1.0 | Frequency multiplier (carrier:modulator ratio) |
| `fixed` | bool | | false | Fixed frequency mode |
| `fixedFreq` | float | 20.0 – 8000.0 | 440.0 | Fixed frequency value (Hz) — only used when `fixed` is true |
| `phaseMode` | string | see [phase modes](#phase-modes) | `"reset"` | Phase reset behavior on note-on |
| `octaveOffset` | int | -2 – 2 | 0 | Octave transpose in semitone steps of 12 |
| `detuneAmount` | float | -100.0 – 100.0 | 0.0 | Pitch offset in cents |
| `enabled` | bool | | true | Whether the oscillator is active |

**Notes:**
- `mixLevel` feeds into the signal chain. Values above 1.0 can overdrive filters/saturator for harmonic content.
- `scanPos` only matters for multi-frame banks (currently only `sine_to_saw`). For single-frame banks (sine, saw, square, triangle), `scanPos` has no audible effect.
- `ratio` multiplies the oscillator's frequency. At ratio 2.0, the oscillator runs an octave above its MIDI pitch. This is applied before FM modulation — it determines the carrier or modulator frequency relationship.
- Mix gain is auto-normalized: `1.0 / enabledOscCount`. If only osc1 is enabled, it gets full level. If all 4 are enabled, each gets 0.25x before the per-voice gain stage.
- `fmDepth` is a multiplier on all routes, useful as a mod matrix target for sweeping overall FM intensity. Set to 1.0 in presets with no mod matrix involvement (neutral). Setting it to 0.0 silences all FM routes.
- `fmRoutes` depths are **normalized 0–1**, mapped through a quadratic curve to raw phase deviation (0–5). Use moderate values (0.3–0.6) for musical FM; above 0.7 produces dense/aliasing spectra.

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

Serialized inside the `voice` JSON object. It is a **per-voice, pre-filter** processor — it runs before the SVF and Ladder filters on each individual voice, giving the "drive into the filter" character of classic hardware synths.

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `drive` | float | 1.0 – 5.0 | 1.0 | Distortion intensity (1.0 = clean) |
| `mix` | float | 0.0 – 1.0 | 1.0 | Wet/dry blend (0.0 = dry, 1.0 = fully saturated) |
| `enabled` | bool | | false | Whether saturation is active |

**Note:** The saturator's position in the signal chain is configurable — it can be placed before or after the filters by reordering `signalChain`. The default order is `["svf", "ladder", "saturator"]`.

### LFOs

Three LFOs (lfo1–lfo3) share the same fields:

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `bank` | string | see [banks](#wavetable-banks) + `"sah"` | `"sine"` | Waveform bank or Sample & Hold |
| `rate` | float | 0.0 – 20.0 | 1.0 | Frequency in Hz — used when `tempoSync` is false |
| `tempoSync` | bool | | false | When true, rate is derived from `bpm` + `subdivision` |
| `subdivision` | string | see [subdivisions](#subdivisions) | `"1/4"` | Beat fraction used when `tempoSync` is true |
| `amplitude` | float | 0.0 – 1.0 | 1.0 | Output range: -amplitude to +amplitude |
| `retrigger` | bool | | false | Reset LFO phase to 0 on first note-on |
| `attackMs` | float | 0.0 – 10000.0 | 10.0 | Attack time in milliseconds |
| `decayMs` | float | 0.0 – 10000.0 | 100.0 | Decay time in milliseconds |

**Notes:**
- `"sah"` (Sample & Hold) outputs a random value that changes at the LFO rate. Useful for random modulation.
- LFOs are **global** — all voices share the same LFO phase. No per-voice phase offset.
- LFOs output a **bipolar** signal: the range is [-amplitude, +amplitude]. The mod matrix `amount` scales this further.
- LFO rate and amplitude are themselves modulatable via the mod matrix (LFO-to-LFO modulation is supported with one-sample feedback delay).
- `retrigger` resets phase on the **first** note-on (when going from 0 active voices to 1), not on every note.
- When `tempoSync` is true, `rate` is ignored. The effective rate is `bpm / (60.0 * subdivisionBeats)`.

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
| FM depth | Normalized | ±1.0 | `0.5` = increase/decrease global FM multiplier by 0.5× |
| LFO rate | Hz | ±20.0 | `2.0` = rate modulation |
| LFO amplitude | Linear | ±1.0 | `0.5` = amplitude modulation |

**Important:** `filterEnv` and `modEnv` do nothing unless routed. If you want filter envelope modulation, you **must** add a mod route. There is no `envAmount` parameter on the filter — the mod matrix replaces that concept.

### Signal Chain

The `signalChain` array defines the order of **per-voice** audio processors. Each voice's oscillator output passes through these in sequence before being mixed to the stereo bus.

```json
"signalChain": ["svf", "ladder", "saturator"]
```

Valid processor names: `"svf"`, `"ladder"`, `"saturator"`

- Maximum 8 slots
- Processors can be reordered (e.g., `["saturator", "svf"]` for pre-filter distortion)
- A processor in the chain but with `enabled: false` is skipped
- Duplicate entries are allowed (e.g., two saturator stages)

### FX Chain

The `fxChain` array defines the order of **post-mix** stereo effects. After all voices are mixed to the stereo bus, the signal passes through the active effects in this sequence.

```json
"fxChain": ["distortion", "chorus", "phaser", "delay", "reverb"]
```

Valid processor names: `"distortion"`, `"chorus"`, `"phaser"`, `"delay"`, `"reverb"`

- Maximum 8 slots
- Effects can be reordered (e.g., `["reverb", "delay"]` for a pre-verb wash)
- An effect in the chain but with `enabled: false` is skipped
- The parameters for each effect live under `fx.<name>` regardless of chain position

**`signalChain` vs `fxChain`:**
- `signalChain` = per-voice, pre-mix, inside the voice render loop
- `fxChain` = global stereo, post-mix, applied once to the combined output

#### Distortion

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `drive` | float | 1.0 – 10.0 | 1.0 | Distortion intensity (1.0 = clean) |
| `mix` | float | 0.0 – 1.0 | 1.0 | Wet/dry blend |
| `type` | string | `"soft"`, `"hard"` | `"soft"` | Clipping algorithm |
| `enabled` | bool | | false | Whether distortion is active |

**Types:**
- `"soft"` — tanh soft clip, normalized to unity at full drive. Smooth saturation character.
- `"hard"` — hard clip at `1/drive` threshold. More aggressive, adds odd harmonics.

#### Chorus

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `rate` | float | 0.1 – 10.0 | 1.0 | LFO rate in Hz |
| `depth` | float | 0.0 – 1.0 | 0.5 | Modulation depth (scales delay sweep range) |
| `mix` | float | 0.0 – 1.0 | 0.5 | Wet/dry blend |
| `feedback` | float | 0.0 – 0.95 | 0.0 | Feedback amount — higher values add ring/resonance |
| `enabled` | bool | | false | Whether chorus is active |

**Notes:**
- 4 internal voices with evenly-distributed quadrature LFO phases for maximum stereo decorrelation.
- Base delay center: 7ms. Maximum depth sweep: ±7ms. Keep `feedback` below 0.5 for conventional chorus; higher values approach flanger territory.

#### Phaser

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `stages` | int | 2 – 12 | 4 | Number of allpass stages (more = more notches) |
| `rate` | float | 0.1 – 10.0 | 0.5 | LFO rate in Hz |
| `depth` | float | 0.0 – 1.0 | 1.0 | Notch sweep depth |
| `feedback` | float | 0.0 – 1.0 | 0.5 | Feedback intensity — increases notch prominence |
| `mix` | float | 0.0 – 1.0 | 0.5 | Wet/dry blend |
| `enabled` | bool | | false | Whether phaser is active |

**Notes:**
- L and R channels sweep at 90° phase offset — stereo width is built in.
- Notch sweep range: 100 Hz – 4000 Hz.
- Higher `stages` = more notches = more complex comb effect. Classic phaser sounds are 4–6 stages.

#### Delay

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `time` | float | 0.01 – 4.0 | 0.5 | Delay time in seconds — used when `tempoSync` is false |
| `tempoSync` | bool | | true | When true, time is derived from `bpm` + `subdivision` |
| `subdivision` | string | see [subdivisions](#subdivisions) | `"1/4"` | Beat fraction used when `tempoSync` is true |
| `feedback` | float | 0.0 – 0.99 | 0.4 | Feedback amount (approaching 1.0 = infinite repeat) |
| `damping` | float | 0.0 – 1.0 | 0.0 | LP filter in feedback path — 0 = clean, 1 = HF muted |
| `hpDamping` | float | 0.0 – 1.0 | 0.0 | HP filter in feedback path — 0 = clean, 1 = LF muted |
| `pingPong` | bool | | false | Alternates repeats between L and R channels |
| `mix` | float | 0.0 – 1.0 | 0.5 | Wet/dry blend |
| `enabled` | bool | | false | Whether delay is active |

**Notes:**
- Delay time changes are smoothed (20ms time constant) to prevent clicks.
- `damping` and `hpDamping` can be combined for a bandpass feedback character — useful for dub-style delays that thin out over repeats.
- Maximum delay time: 4 seconds.

#### Reverb

Dattorro plate reverb algorithm.

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `preDelay` | float | 0.0 – 100.0 | 0.0 | Pre-delay before the reverb in milliseconds |
| `decay` | float | 0.1 – 20.0 | 4.0 | Reverb tail length in seconds |
| `damping` | float | 0.0 – 1.0 | 0.5 | LP damping in tank — 0 = bright, 1 = dark/muted HF |
| `lowDamping` | float | 0.0 – 1.0 | 0.5 | HP damping in tank — 0 = full bass, 1 = thin/muted LF |
| `diffusion` | float | 0.0 – 1.0 | 0.75 | Input diffusion — higher = smoother attack, lower = grainier |
| `bandwidth` | float | 0.0 – 1.0 | 0.75 | Input LP pass-through — lower = darker overall reverb |
| `modRate` | float | 0.01 – 5.0 | 0.5 | Tank LFO rate in Hz — adds shimmer/movement |
| `modDepth` | float | 0.0 – 1.0 | 0.5 | Tank modulation depth |
| `mix` | float | 0.0 – 1.0 | 0.3 | Wet/dry blend |
| `enabled` | bool | | false | Whether reverb is active |

**Notes:**
- `decay` controls RT60 (time for tail to decay 60dB). Values above 10s produce ambient/drone-style reverbs.
- `damping` and `lowDamping` act inside the feedback tank — they shape the tail, not the initial attack.
- `bandwidth` darkens the input signal before the tank, affecting the whole reverb character.
- High `modDepth` + low `modRate` = lush shimmer. High `modDepth` + high `modRate` = more metallic.

---

### Voice

All per-voice behavior fields are grouped under the `"voice"` key in JSON.

#### Saturator

See [Saturator](#saturator) above. Serialized here because it is a per-voice processor.

#### Pitch Bend

| Field | Type | Range | Default | Description |
|-------|------|-------|---------|-------------|
| `range` | float | 0.0 – 48.0 | 2.0 | Pitch bend range in semitones (applied symmetrically up/down) |

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

### FM Routes

`fmRoutes` is an array of route objects on each oscillator. Up to 4 routes per carrier. All routes are summed before phase application.

```json
"fmRoutes": [
  { "source": "osc2", "depth": 0.55 },
  { "source": "osc1", "depth": 0.25 }
]
```

| Field | Type | Range | Description |
|-------|------|-------|-------------|
| `source` | string | see below | Which oscillator feeds this route |
| `depth` | float | 0.0 – 1.0 | Normalized FM depth for this route (quadratic curve: 1.0 → raw depth 5.0) |

**Valid source strings:**

| String | Description |
|--------|-------------|
| `"osc1"` | Osc1's output |
| `"osc2"` | Osc2's output |
| `"osc3"` | Osc3's output |
| `"osc4"` | Osc4's output |

**Notes:**
- Self-modulation (e.g., `"source": "osc1"` on osc1) is valid — uses averaged previous sample for feedback stability
- Duplicate sources in the same route array are rejected at load (silently skipped)
- An empty array (`[]`) means no FM on this oscillator
- Route depths are independent; `fmDepth` (the global multiplier) scales all routes together

**Depth guidance:**

| Normalized depth | Raw depth (approx) | Character |
|---|---|---|
| 0.1–0.3 | 0.05–0.45 | Subtle — light harmonic enrichment; safe zone for self-feedback |
| 0.3–0.55 | 0.45–1.5 | Noticeable FM character |
| 0.55–0.75 | 1.5–2.8 | Heavy FM |
| 0.75–1.0 | 2.8–5.0 | Extreme — dense sidebands, aliasing likely |

### Phase Modes

Controls how an oscillator's phase is handled on note-on.

| String | Description |
|--------|-------------|
| `"reset"` | Phase resets to 0 on every note-on. Consistent attack transient. Default. |
| `"free"` | Phase never resets — oscillator runs continuously regardless of notes. Useful for drones and textures where phase coherence isn't needed. |
| `"random"` | Phase randomized on each note-on. Avoids the comb-filtering that can occur when multiple voices start at the same phase. Recommended for pads with unison. |
| `"spread"` | Phases of unison sub-voices are evenly distributed around the cycle. Only meaningful when unison is active; falls back to reset behavior for non-unison voices. |

### SVF Modes

| String | Description |
|--------|-------------|
| `"lp"` | Lowpass — passes frequencies below cutoff |
| `"hp"` | Highpass — passes frequencies above cutoff |
| `"bp"` | Bandpass — passes frequencies around cutoff |
| `"notch"` | Notch/band-reject — removes frequencies around cutoff |

### Distortion Types

| String | Description |
|--------|-------------|
| `"soft"` | tanh soft clip, normalized to unity gain |
| `"hard"` | Hard clip at 1/drive threshold |

### LFO Banks

Same as [wavetable banks](#wavetable-banks), plus:

| String | Description |
|--------|-------------|
| `"sah"` | Sample & Hold — outputs a new random value at each LFO cycle |

### Subdivisions

Used by LFO `subdivision` and delay `subdivision`. Represents a beat fraction where 1.0 beat = one quarter note.

| String | Type | Beats | Description |
|--------|------|-------|-------------|
| `"1/1"` | straight | 4.0 | Whole note |
| `"1/2"` | straight | 2.0 | Half note |
| `"1/4"` | straight | 1.0 | Quarter note |
| `"1/8"` | straight | 0.5 | Eighth note |
| `"1/16"` | straight | 0.25 | Sixteenth note |
| `"1/32"` | straight | 0.125 | Thirty-second note |
| `"1/64"` | straight | 0.0625 | Sixty-fourth note |
| `"d1/2"` | dotted | 3.0 | Dotted half |
| `"d1/4"` | dotted | 1.5 | Dotted quarter |
| `"d1/8"` | dotted | 0.75 | Dotted eighth |
| `"d1/16"` | dotted | 0.375 | Dotted sixteenth |
| `"1/2t"` | triplet | 4/3 | Half triplet |
| `"1/4t"` | triplet | 2/3 | Quarter triplet |
| `"1/8t"` | triplet | 1/3 | Eighth triplet |
| `"1/16t"` | triplet | 1/6 | Sixteenth triplet |

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
| `"keyTrack"` | 0.0 – 1.0 | yes | Normalized MIDI note: 0.0 at C0, 1.0 at G10 |

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
| `"osc1.fmDepth"` | normalized (±1) | Osc1 global FM multiplier — adds to the 1.0 neutral; scales all routes |
| `"osc2.fmDepth"` | normalized (±1) | Osc2 global FM multiplier |
| `"osc3.fmDepth"` | normalized (±1) | Osc3 global FM multiplier |
| `"osc4.fmDepth"` | normalized (±1) | Osc4 global FM multiplier |
| `"lfo1.rate"` | Hz (±20) | LFO1 speed |
| `"lfo2.rate"` | Hz (±20) | LFO2 speed |
| `"lfo3.rate"` | Hz (±20) | LFO3 speed |
| `"lfo1.amplitude"` | linear (±1) | LFO1 depth |
| `"lfo2.amplitude"` | linear (±1) | LFO2 depth |
| `"lfo3.amplitude"` | linear (±1) | LFO3 depth |

---

## Annotated Example Preset

A dub techno chord stab with filter envelope sweep, LFO wobble, and post-mix delay + reverb:

```json
{
  "version": 1,
  "bpm": 120.0,
  "metadata": {
    "name": "Dub Techno Stab",
    "author": "",
    "category": "Techno",
    "description": "Filtered chord stab with slow LFO movement, delay, and reverb"
  },

  "oscillators": {
    "osc1": {
      "bank": "saw", "scanPos": 0.0, "mixLevel": 1.0,
      "fmDepth": 1.0, "ratio": 1.0, "fmRoutes": [],
      "phaseMode": "reset", "octaveOffset": 0, "detuneAmount": 8.0, "enabled": true
    },
    "osc2": {
      "bank": "saw", "scanPos": 0.0, "mixLevel": 0.8,
      "fmDepth": 1.0, "ratio": 1.0, "fmRoutes": [],
      "phaseMode": "reset", "octaveOffset": 0, "detuneAmount": -8.0, "enabled": true
    },
    "osc3": {
      "bank": "sine", "scanPos": 0.0, "mixLevel": 0.4,
      "fmDepth": 1.0, "ratio": 1.0, "fmRoutes": [],
      "phaseMode": "reset", "octaveOffset": -1, "detuneAmount": 0.0, "enabled": true
    },
    "osc4": {
      "bank": "sine", "scanPos": 0.0, "mixLevel": 0.0,
      "fmDepth": 1.0, "ratio": 1.0, "fmRoutes": [],
      "phaseMode": "reset", "octaveOffset": 0, "detuneAmount": 0.0, "enabled": false
    },
    "noise": { "mixLevel": 0.0, "type": "white", "enabled": false }
  },

  "envelopes": {
    "ampEnv": {
      "attackMs": 5.0, "decayMs": 400.0, "sustainLevel": 0.0, "releaseMs": 300.0,
      "attackCurve": -3.0, "decayCurve": -6.0, "releaseCurve": -5.0
    },
    "filterEnv": {
      "attackMs": 2.0, "decayMs": 500.0, "sustainLevel": 0.1, "releaseMs": 200.0,
      "attackCurve": -2.0, "decayCurve": -7.0, "releaseCurve": -5.0
    },
    "modEnv": {
      "attackMs": 10.0, "decayMs": 100.0, "sustainLevel": 0.7, "releaseMs": 200.0,
      "attackCurve": -5.0, "decayCurve": -5.0, "releaseCurve": -5.0
    }
  },

  "filters": {
    "svf": { "mode": "lp", "cutoff": 600.0, "resonance": 0.55, "enabled": true },
    "ladder": { "cutoff": 1000.0, "resonance": 0.3, "drive": 1.0, "enabled": false }
  },

  "lfos": {
    "lfo1": { "bank": "sine", "rate": 0.3, "tempoSync": false, "subdivision": "1/4", "amplitude": 1.0, "retrigger": false },
    "lfo2": { "bank": "sine", "rate": 1.0, "tempoSync": false, "subdivision": "1/4", "amplitude": 1.0, "retrigger": false },
    "lfo3": { "bank": "sine", "rate": 1.0, "tempoSync": false, "subdivision": "1/4", "amplitude": 1.0, "retrigger": false }
  },

  "modMatrix": [
    { "source": "filterEnv", "destination": "svf.cutoff", "amount": 2.5 },
    { "source": "lfo1",      "destination": "svf.cutoff", "amount": 0.4 }
  ],

  "signalChain": ["svf", "ladder", "saturator"],

  "fxChain": ["distortion", "chorus", "delay", "reverb"],

  "fx": {
    "distortion": { "drive": 1.0, "mix": 1.0, "type": "soft", "enabled": false },
    "chorus":     { "rate": 0.5, "depth": 0.3, "mix": 0.25, "feedback": 0.0, "enabled": false },
    "phaser":     { "stages": 4, "rate": 0.5, "depth": 1.0, "feedback": 0.5, "mix": 0.5, "enabled": false },
    "delay": {
      "time": 0.5, "tempoSync": true, "subdivision": "1/8",
      "feedback": 0.45, "damping": 0.35, "hpDamping": 0.1,
      "pingPong": true, "mix": 0.3, "enabled": true
    },
    "reverb": {
      "preDelay": 15.0, "decay": 3.5, "damping": 0.55, "lowDamping": 0.2,
      "diffusion": 0.75, "bandwidth": 0.8, "modRate": 0.4, "modDepth": 0.3,
      "mix": 0.2, "enabled": true
    }
  },

  "voice": {
    "saturator":  { "drive": 1.5, "mix": 0.4, "enabled": true },
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
- Light per-voice saturation for warmth (pre-filter)
- Ping-pong delay at 1/8 note with damped feedback for depth without clutter
- Short pre-delayed plate reverb for space without washing out the stab

---

## Init Preset

The init preset is a clean starting point — one sine oscillator, no filters, no effects, no modulation.

```json
{
  "version": 1,
  "bpm": 120.0,
  "metadata": {
    "name": "Init",
    "author": "",
    "category": "Init",
    "description": "Clean starting point — single sine oscillator, no processing"
  },
  "oscillators": {
    "osc1": {
      "bank": "sine", "scanPos": 0.0, "mixLevel": 1.0,
      "fmDepth": 1.0, "ratio": 1.0, "fmRoutes": [],
      "phaseMode": "reset", "octaveOffset": 0, "detuneAmount": 0.0, "enabled": true
    },
    "osc2": {
      "bank": "sine", "scanPos": 0.0, "mixLevel": 0.0,
      "fmDepth": 1.0, "ratio": 1.0, "fmRoutes": [],
      "phaseMode": "reset", "octaveOffset": 0, "detuneAmount": 0.0, "enabled": false
    },
    "osc3": {
      "bank": "sine", "scanPos": 0.0, "mixLevel": 0.0,
      "fmDepth": 1.0, "ratio": 1.0, "fmRoutes": [],
      "phaseMode": "reset", "octaveOffset": 0, "detuneAmount": 0.0, "enabled": false
    },
    "osc4": {
      "bank": "sine", "scanPos": 0.0, "mixLevel": 0.0,
      "fmDepth": 1.0, "ratio": 1.0, "fmRoutes": [],
      "phaseMode": "reset", "octaveOffset": 0, "detuneAmount": 0.0, "enabled": false
    },
    "noise": { "mixLevel": 0.0, "type": "white", "enabled": false }
  },
  "envelopes": {
    "ampEnv": {
      "attackMs": 10.0, "decayMs": 100.0, "sustainLevel": 0.7, "releaseMs": 200.0,
      "attackCurve": -5.0, "decayCurve": -5.0, "releaseCurve": -5.0
    },
    "filterEnv": {
      "attackMs": 10.0, "decayMs": 100.0, "sustainLevel": 0.7, "releaseMs": 200.0,
      "attackCurve": -5.0, "decayCurve": -5.0, "releaseCurve": -5.0
    },
    "modEnv": {
      "attackMs": 10.0, "decayMs": 100.0, "sustainLevel": 0.7, "releaseMs": 200.0,
      "attackCurve": -5.0, "decayCurve": -5.0, "releaseCurve": -5.0
    }
  },
  "filters": {
    "svf": { "mode": "lp", "cutoff": 1000.0, "resonance": 0.5, "enabled": false },
    "ladder": { "cutoff": 1000.0, "resonance": 0.3, "drive": 1.0, "enabled": false }
  },
  "lfos": {
    "lfo1": { "bank": "sine", "rate": 1.0, "tempoSync": false, "subdivision": "1/4", "amplitude": 1.0, "retrigger": false },
    "lfo2": { "bank": "sine", "rate": 1.0, "tempoSync": false, "subdivision": "1/4", "amplitude": 1.0, "retrigger": false },
    "lfo3": { "bank": "sine", "rate": 1.0, "tempoSync": false, "subdivision": "1/4", "amplitude": 1.0, "retrigger": false }
  },
  "modMatrix": [],
  "signalChain": ["svf", "ladder", "saturator"],
  "fxChain": ["distortion", "chorus", "phaser", "delay", "reverb"],
  "fx": {
    "distortion": { "drive": 1.0, "mix": 1.0, "type": "soft", "enabled": false },
    "chorus":     { "rate": 1.0, "depth": 0.5, "mix": 0.5, "feedback": 0.0, "enabled": false },
    "phaser":     { "stages": 4, "rate": 0.5, "depth": 1.0, "feedback": 0.5, "mix": 0.5, "enabled": false },
    "delay":      { "time": 0.5, "tempoSync": true, "subdivision": "1/4", "feedback": 0.4, "damping": 0.0, "hpDamping": 0.0, "pingPong": false, "mix": 0.5, "enabled": false },
    "reverb":     { "preDelay": 0.0, "decay": 4.0, "damping": 0.5, "lowDamping": 0.5, "diffusion": 0.75, "bandwidth": 0.75, "modRate": 0.5, "modDepth": 0.5, "mix": 0.3, "enabled": false }
  },
  "voice": {
    "saturator":  { "drive": 1.0, "mix": 1.0, "enabled": false },
    "pitchBend":  { "range": 2.0 },
    "mono":       { "enabled": false, "legato": true },
    "portamento": { "time": 50.0, "legato": true, "enabled": false },
    "unison":     { "voices": 4, "detune": 20.0, "spread": 0.5, "enabled": false }
  }
}
```
