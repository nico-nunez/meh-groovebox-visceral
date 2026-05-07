local bass = TrackSettings()

bass.synth = SynthSettings {
  osc1 = {
    enabled = true,
    bank = "saw",
    mix = 0.8,
    octave = -1,
    scan = 0.1,
  },

  ampEnv = {
    attack = 5,
    decay = 120,
    sustain = 0.7,
    release = 180,
  },

  svf = {
    enabled = true,
    mode = "lp",
    cutoff = 1200,
    resonance = 0.2,
  },

  fx = {
    delay = { enabled = true, mix = 0.25 },
    reverb = { enabled = true, mix = 0.15 },
  },
}

track(1, bass)

synth(2, SynthSettings {
  master = { gain = 0.9 },
  unison = { enabled = true, voices = 3 },
})

track(1, TrackSettings {
  patterns = {
    [1] = {
      numSteps = 1,
      stepsPerBeat = 4,
      steps = {
        { active = true, note = 60, velocity = 100, gate = 0.8 }
      }
    }
  },
  activeSlot = 1
})
