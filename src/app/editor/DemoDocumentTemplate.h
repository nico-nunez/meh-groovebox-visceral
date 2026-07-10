#pragma once

namespace app::editor {

// Bundled 5-track demo patch (kick/hi-hat/tom/lead/pad), embedded so it can be
// materialized into the user's config directory without depending on the
// repo tree being present alongside the compiled binary.
inline constexpr const char* demoDocumentTemplate() {
  return R"demo(--*================*--
--*      Kick      *--
--*================*--
--- @type SynthSettings
local kickSynth = {
  osc1 = { bank = "sine", octaveOffset = -2, enabled = true },
  osc2 = { enabled = false },
  osc3 = { enabled = false },
  osc4 = { enabled = false },
  ampEnv = { attack = 1.0, decay = 0.0, sustain = 1.0, release = 200.0 },
  modEnv = { attack = 1.0, decay = 600.0, sustain = 0.0, release = 0.0 },
  modMatrix = {
    { src = "modEnv", dest = "osc1.pitch", amount = 24.0 },
  },
  saturator = { drive = 1.5 },
}

--- @type TrackSettings
local kickTrack = {
  patterns = {
    [1] = {
      numSteps = 1,
      stepsPerBeat = 1,
      steps = {
        { active = true, notes = { { active = true, note = 54, velocity = 100, gate = 0.8 } } },
      }
    }
  },
  activeSlot = 1,
  synth = kickSynth,
  mixer = { mute = false, gain = 0.7 }
};

--*==================*--
--*      Hi-Hat      *--
--*==================*--
--- @type SynthSettings
local hatSynth = {
  osc1 = { bank = 'square', mixLevel = 0.03, fixed = true, fixedFreq = 6400.0 },
  osc2 = { enabled = false },
  osc3 = { enabled = false },
  osc4 = { enabled = false },
  noise = { mixLevel = 1.0, type = 'white', enabled = true },
  ampEnv = { attack = 0.0, attackCurve = -1.0, decay = 85.0, decayCurve = -7.5, sustain = 0.0, release = 10.0, releaseCurve = -7.0 },
  filterEnv = { attack = 0.0, attackCurve = -1.0, decay = 24.0, sustain = 0.0, release = 10.0, },
  svf = { mode = 'hp', cutoff = 7200.0, resonance = 0.2, enabled = true },
  modMatrix = { { src = "filterEnv", dest = "svf.cutoff", amount = 0.3 } },
  fx = { distortion = { drive = 1.25, mix = 0.08, type = "soft", enabled = true } },
  saturator = { drive = 1.7, mix = 0.38, enabled = true },
  mono = { enabled = true },
  signalChain = { "saturator", "svf", "ladder" }
}

--- @type TrackSettings
local hatTrack = {
  patterns = {
    [1] = {
      numSteps = 2,
      stepsPerBeat = 4,
      steps = {
        { active = true, notes = { { active = true, note = 66, velocity = 80, gate = 0.4 } } },
        { active = true, notes = { { active = true, note = 66, velocity = 50, gate = 0.4 } } },
      }
    }
  },
  activeSlot = 1,
  synth = hatSynth,
  mixer = { gain = 0.6 }
}

--*===============*--
--*      Tom      *--
--*===============*--
--- @type SynthSettings
local tomSynth = {
  osc1 = { bank = "sine", octaveOffset = -2, enabled = true },
  osc2 = { enabled = false },
  osc3 = { enabled = false },
  osc4 = { enabled = false },
  ampEnv = { attack = 1.0, decay = 0.0, sustain = 1.0, release = 200.0 },
  modEnv = { attack = 10.0, decay = 650.0, sustain = 0.0, release = 20.0 },
  modMatrix = {
    { src = "modEnv", dest = "osc1.pitch", amount = 24.0 },
  },
  svf = { mode = 'hp', cutoff = 400, resonance = 0.0, enabled = true },
  mono = { enabled = true },
  saturator = { drive = 1.5 },
}

--- @type TrackSettings
local tomTrack = {
  patterns = {
    [1] = {
      numSteps = 7,
      stepsPerBeat = 4,
      steps = {
        { active = false, notes = { { active = true, note = 0, velocity = 0, gate = 0.0 } } },
        { active = false, notes = { { active = true, note = 0, velocity = 0, gate = 0.0 } } },
        { active = true,  notes = { { active = true, note = 66, velocity = 100, gate = 0.5 } } },
        { active = false, notes = { { active = true, note = 0, velocity = 0, gate = 0.0 } } },
        { active = true,  notes = { { active = true, note = 66, velocity = 100, gate = 0.5 } } },
        { active = true,  notes = { { active = true, note = 66, velocity = 100, gate = 0.5 } } },
        { active = false, notes = { { active = true, note = 0, velocity = 0, gate = 0.0 } } },
        { active = true,  notes = { { active = true, note = 66, velocity = 100, gate = 1.5 } } },
      }
    }
  },
  activeSlot = 1,
  synth = tomSynth,
  mixer = { gain = 0.8 }
};


--*================*--
--*      Lead      *--
--*================*--
--- @type SynthSettings
local synthLead = {
  osc1 = { bank = "sine_to_saw", scanPos = 0.2, mixLevel = 1.0, detuneAmount = 5.0 },
  osc2 = { bank = "sine_to_saw", scanPos = 0.2, mixLevel = 0.8, detuneAmount = -5.0 },
  osc3 = { enabled = false },
  osc4 = { enabled = false },
  ampEnv = { attack = 5.0, decay = 200.0, sustain = 0.75, release = 300.0, attackCurve = -3.0, decayCurve = -4.0 },
  filterEnv = { attack = 3.0, decay = 300.0, sustain = 0.2, release = 200.0, attackCurve = -2.0, decayCurve = -6.0 },
  modEnv = { attack = 3.0, decay = 400.0, sustain = 0.0, release = 200.0, attackCurve = -2.0 },
  svf = { mode = "lp", cutoff = 1000.0, resonance = 0.1, enabled = true },
  lfo1 = { bank = "sine", rate = 0.8, amplitude = 1.0, retrigger = true },
  modMatrix = {
    { src = "filterEnv", dest = "svf.cutoff",   amount = 2.0 },
    { src = "keyTrack",  dest = "svf.cutoff",   amount = 1.0 },
    { src = "velocity",  dest = "svf.cutoff",   amount = 0.8 },
    { src = "modEnv",    dest = "osc1.scanPos", amount = 0.8 },
    { src = "modEnv",    dest = "osc2.scanPos", amount = 0.8 },
    { src = "lfo1",      dest = "osc1.scanPos", amount = 0.12 },
    { src = "lfo1",      dest = "osc2.scanPos", amount = 0.12 }
  },
  signalChain = { "svf", "ladder", "saturator" },
  fx = {
    distortion = { drive = 2.5, mix = 0.25, type = "soft", enabled = true },
    chorus = { rate = 0.5, depth = 0.3, mix = 0.2, feedback = 0.0, enabled = true },
    delay = {
      time = 0.5,
      tempoSync = true,
      subdivision = "1/8",
      feedback = 0.35,
      damping = 0.3,
      hpDamping = 0.05,
      pingPong = true,
      mix = 0.2,
      enabled = true
    },
    reverb = {
      preDelay = 10.0,
      decay = 2.5,
      damping = 0.4,
      lowDamping = 0.25,
      diffusion = 0.78,
      bandwidth = 0.82,
      modRate = 0.35,
      modDepth = 0.25,
      mix = 0.2,
      enabled = true
    },
  },
  pitchBend = { range = 2.0 },
  mono = { enabled = true, legato = true },
  porta = { time = 30.0, legato = true, enabled = true },
}

--- @type TrackSettings
local synthTrack = {
  patterns = {
    [1] = {
      numSteps = 8,
      stepsPerBeat = 2,
      steps = {
        { active = true,  notes = { { active = true, tie = true, note = 50, velocity = 100, gate = 2.5 } } },
        { active = false, notes = { { active = true, note = 0, velocity = 0, gate = 0.0 } } },
        { active = true,  notes = { { active = true, note = 52, velocity = 100, gate = 1.5 } } },
        { active = true,  notes = { { active = true, note = 50, velocity = 100, gate = 1.5 } } },
        { active = true,  notes = { { active = true, note = 53, velocity = 100, gate = 1.5 } } },
        { active = true,  notes = { { active = true, note = 50, velocity = 100, gate = 1.5 } } },
        { active = true,  notes = { { active = true, note = 52, velocity = 100, gate = 1.5 } } },
        { active = true,  notes = { { active = true, note = 50, velocity = 100, gate = 1.5 } } },
      }
    }
  },
  activeSlot = 1,
  synth = synthLead,
  mixer = { gain = 0.30 }
}

--*================*--
--*      Pads      *--
--*================*--
--- @type SynthSettings
local padSynth = {
  osc1 = { bank = "sine_to_saw", scanPos = 0.3, mixLevel = 1.0, detuneAmount = 6.0, enabled = true },
  osc2 = { bank = "sine_to_saw", scanPos = 0.3, mixLevel = 0.8, detuneAmount = -6.0, enabled = true },
  osc3 = { bank = "triangle", mixLevel = 0.5, octaveOffset = 1, detuneAmount = 3.0, enabled = true },
  osc4 = { bank = "sine", mixLevel = 0.4, octaveOffset = -1, enabled = true },
  ampEnv = {
    attack = 800.0,
    decay = 1000.0,
    sustain = 0.85,
    release = 3000.0,
    attackCurve = -4.0,
    decayCurve = -3.0,
    releaseCurve = -4.0
  },
  filterEnv = {
    attack = 1200.0,
    decay = 2000.0,
    sustain = 0.4,
    release = 2000.0,
    attackCurve = -3.0,
    decayCurve = -4.0,
    releaseCurve = -4.0
  },
  modEnv = { attack = 10.0, decay = 100.0, sustain = 0.7, release = 200.0 },
  svf = { mode = "lp", cutoff = 3000.0, resonance = 0.2, enabled = true },
  lfo1 = { bank = "sine", rate = 0.15, amplitude = 0.7 },
  lfo2 = { bank = "triangle", rate = 0.08, amplitude = 0.5 },
  modMatrix = {
    { src = "filterEnv", dest = "svf.cutoff",   amount = 1.5 },
    { src = "lfo1",      dest = "osc1.scanPos", amount = 0.4 },
    { src = "lfo1",      dest = "osc2.scanPos", amount = 0.4 },
    { src = "lfo2",      dest = "svf.cutoff",   amount = 0.3 }
  },
  signalChain = { "svf", "ladder", "saturator" },
  fx = {
    chorus = {
      rate = 0.2,
      depth = 0.3,
      mix = 0.18,
      feedback = 0.0,
      enabled = true
    },
    phaser = {
      stages = 6,
      rate = 0.1,
      depth = 0.7,
      feedback = 0.35,
      mix = 0.25,
      enabled = true
    },
    reverb = {
      preDelay = 20.0,
      decay = 7.0,
      damping = 0.35,
      lowDamping = 0.15,
      diffusion = 0.88,
      bandwidth = 0.8,
      modRate = 0.25,
      modDepth = 0.4,
      mix = 0.35,
      enabled = true
    }
  },
  unison = {
    voices = 4,
    detune = 15.0,
    spread = 0.7,
    enabled = true
  },
}

--- @type TrackSettings
local padTrack = {
  patterns = {
    [1] = {
      numSteps = 32,
      stepsPerBeat = 1,
      steps = {
        {
          active = true,
          notes = {
            { active = true, tie = true, note = 69, velocity = 120, gate = 14.0 },
            { active = true, tie = true, note = 72, velocity = 120, gate = 14.0 },
            { active = true, tie = true, note = 79, velocity = 120, gate = 14.0 },
          }
        },
        { active = false, notes = {} }, { active = false, notes = {} }, { active = false, notes = {} },
        { active = false, notes = {} }, { active = false, notes = {} }, { active = false, notes = {} },
        { active = false, notes = {} }, { active = false, notes = {} }, { active = false, notes = {} },
        { active = false, notes = {} }, { active = false, notes = {} }, { active = false, notes = {} },
        { active = false, notes = {} }, { active = false, notes = {} },
        {
          active = true,
          notes = {
            { active = true, tie = true, note = 69, velocity = 120, gate = 16.0 },
            { active = true, tie = true, note = 72, velocity = 120, gate = 16.0 },
            { active = true, tie = true, note = 76, velocity = 120, gate = 16.0 },
          }
        },
        { active = false, notes = {} }, { active = false, notes = {} }, { active = false, notes = {} },
        { active = false, notes = {} }, { active = false, notes = {} }, { active = false, notes = {} },
        { active = false, notes = {} }, { active = false, notes = {} }, { active = false, notes = {} },
        { active = false, notes = {} }, { active = false, notes = {} }, { active = false, notes = {} },
        { active = false, notes = {} }, { active = false, notes = {} }, { active = false, notes = {} },
        { active = false, notes = {} },
      }
    }
  },
  activeSlot = 1,
  synth = padSynth,
  mixer = { gain = 0.70 }
}

track(1, kickTrack)
track(2, hatTrack)
track(3, tomTrack)
track(4, synthTrack)
track(5, padTrack)
)demo";
}

} // namespace app::editor
