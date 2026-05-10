-- Per-track via TrackSettings
local bass = TrackSettings()
bass.mixer = MixerSettings { gain = 0.8, pan = -0.2, mute = false }
track(1, bass)

-- Convenience form
mixer(2, MixerSettings { mute = true })
mixer(3, MixerSettings { gain = 0.6, pan = 0.4 })

-- Inline
track(4, TrackSettings {
  mixer = MixerSettings { gain = 1.0 },
})
