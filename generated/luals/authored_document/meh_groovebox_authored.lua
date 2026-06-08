--- @generated from C++ metadata. Do not edit.
--- Source of truth: C++ metadata descriptors.
---@meta meh_groovebox_authored

--- Bounds:
--- trackNumber: 1..8
--- activeSlot: 1..8
--- numSteps: 1..64
--- stepsPerBeat: 1..48
--- locks: at most 4

---@alias TempoSubdivision '1/1' | '1/2' | '1/4' | '1/8' | '1/16' | '1/32' | '1/64' | 'd1/2' | 'd1/4' | 'd1/8' | 'd1/16' | '1/2t' | '1/4t' | '1/8t' | '1/16t'

---@alias PatternSlots table <integer, Pattern>

---@alias SynthOscBank 'sine' | 'saw' | 'square' | 'triangle' | 'sine_to_saw' | 'sah'

---@alias SynthOscPhaseMode 'reset' | 'free' | 'random' | 'spread'

---@alias SynthNoiseType 'white' | 'pink'

---@alias SynthSVFMode 'hp' | 'lp' | 'bp' | 'notch'

---@alias SynthModSource 'ampEnv' | 'filterEnv' | 'modEnv' | 'lfo1' | 'lfo2' | 'lfo3' | 'velocity' | 'noise' | 'modWheel' | 'keyTrack'

---@alias SynthModDestination 'svf.cutoff' | 'ladder.cutoff' | 'svf.resonance' | 'ladder.resonance' | 'osc1.pitch' | 'osc2.pitch' | 'osc3.pitch' | 'osc4.pitch' | 'osc1.mixLevel' | 'osc2.mixLevel' | 'osc3.mixLevel' | 'osc4.mixLevel' | 'osc1.scanPos' | 'osc2.scanPos' | 'osc3.scanPos' | 'osc4.scanPos' | 'osc1.fmDepth' | 'osc2.fmDepth' | 'osc3.fmDepth' | 'osc4.fmDepth' | 'lfo1.rate' | 'lfo2.rate' | 'lfo3.rate' | 'lfo1.amplitude' | 'lfo2.amplitude' | 'lfo3.amplitude'

---@alias SynthFMSource 'none' | 'osc1' | 'osc2' | 'osc3' | 'osc4'

---@alias SynthFXDistortionType 'soft' | 'hard'

---@class Pattern
---@field numSteps integer -- 1..64
---@field stepsPerBeat integer -- 1..48
---@field steps Step[]

---@class StepNote
---@field noteOn? boolean
---@field tie? boolean
---@field note? integer -- 0..127
---@field velocity? integer -- 0..127
---@field gate? number

---@class StepLock
---@field param string
---@field value number

---@class Step
---@field active? boolean
---@field notes? StepNote[]
---@field locks? StepLock[]

---@class SynthSettings
---@field osc1? SynthOscSettings
---@field osc2? SynthOscSettings
---@field osc3? SynthOscSettings
---@field osc4? SynthOscSettings
---@field lfo1? SynthLfoSettings
---@field lfo2? SynthLfoSettings
---@field lfo3? SynthLfoSettings
---@field noise? SynthNoiseSettings
---@field ampEnv? SynthAmpEnvSettings
---@field modEnv? SynthModEnvSettings
---@field filterEnv? SynthFilterEnvSettings
---@field svf? SynthSVFSettings
---@field ladder? SynthLadderSettings
---@field saturator? SynthSaturatorSettings
---@field mono? SynthMonoSettings
---@field porta? SynthPortaSettings
---@field unison? SynthUnisonSettings
---@field pitchBend? SynthPitchBendSettings
---@field modMatrix? SynthModRouteEntry[]
---@field fmRoutes? SynthFMRouteEntry[]
---@field signalChain? string[]
---@field master? SynthMasterSettings
---@field fx? SynthFXSettings

---@class SynthOscSettings
---@field enabled? boolean
---@field bank? SynthOscBank
---@field mixLevel? number
---@field detuneAmount? number
---@field octaveOffset? integer
---@field scanPos? number
---@field fmDepth? number
---@field randomRange? number
---@field resetPhase? number
---@field phaseMode? SynthOscPhaseMode
---@field ratio? number
---@field fixed? boolean
---@field fixedFreq? number

---@class SynthLfoSettings
---@field bank? SynthOscBank
---@field rate? number
---@field amplitude? number
---@field retrigger? boolean
---@field delay? number
---@field attack? number
---@field subdivision? TempoSubdivision
---@field tempoSync? boolean

---@class SynthNoiseSettings
---@field type? SynthNoiseType
---@field enabled? boolean
---@field mixLevel? number

---@class SynthAmpEnvSettings
---@field attack? number
---@field attackCurve? number
---@field decay? number
---@field decayCurve? number
---@field sustain? number
---@field release? number
---@field releaseCurve? number

---@class SynthModEnvSettings
---@field attack? number
---@field attackCurve? number
---@field decay? number
---@field decayCurve? number
---@field sustain? number
---@field release? number
---@field releaseCurve? number

---@class SynthFilterEnvSettings
---@field attack? number
---@field attackCurve? number
---@field decay? number
---@field decayCurve? number
---@field sustain? number
---@field release? number
---@field releaseCurve? number

---@class SynthSVFSettings
---@field mode? SynthSVFMode
---@field cutoff? number
---@field resonance? number
---@field enabled? boolean

---@class SynthLadderSettings
---@field cutoff? number
---@field resonance? number
---@field drive? number
---@field enabled? boolean

---@class SynthSaturatorSettings
---@field drive? number
---@field mix? number
---@field enabled? boolean

---@class SynthMonoSettings
---@field enabled? boolean
---@field legato? boolean

---@class SynthPortaSettings
---@field time? number
---@field legato? boolean
---@field enabled? boolean

---@class SynthUnisonSettings
---@field voices? integer
---@field detune? number
---@field spread? number
---@field enabled? boolean

---@class SynthPitchBendSettings
---@field range? number

---@class SynthModRouteEntry
---@field src SynthModSource
---@field dest SynthModDestination
---@field amount number

---@class SynthFMRouteEntry
---@field carrier SynthFMSource
---@field mod SynthFMSource
---@field depth number

---@class SynthMasterSettings
---@field gain? number

---@class SynthFXSettings
---@field distortion? SynthFXDistortionSettings
---@field chorus? SynthFXChorusSettings
---@field phaser? SynthFXPhaserSettings
---@field delay? SynthFXDelaySettings
---@field reverb? SynthFXReverbSettings

---@class SynthFXDistortionSettings
---@field drive? number
---@field mix? number
---@field type? SynthFXDistortionType
---@field enabled? boolean

---@class SynthFXChorusSettings
---@field rate? number
---@field depth? number
---@field mix? number
---@field feedback? number
---@field enabled? boolean

---@class SynthFXPhaserSettings
---@field stages? integer
---@field rate? number
---@field depth? number
---@field feedback? number
---@field mix? number
---@field enabled? boolean

---@class SynthFXDelaySettings
---@field time? number
---@field subdivision? TempoSubdivision
---@field tempoSync? boolean
---@field feedback? number
---@field damping? number
---@field hpDamping? number
---@field mix? number
---@field pingPong? boolean
---@field enabled? boolean

---@class SynthFXReverbSettings
---@field preDelay? number
---@field decay? number
---@field damping? number
---@field lowDamping? number
---@field diffusion? number
---@field bandwidth? number
---@field modRate? number
---@field modDepth? number
---@field mix? number
---@field enabled? boolean

---@class MixerSettings
---@field gain? number
---@field pan? number
---@field mute? boolean

---@class MixerTrackSettings
---@field gain? number
---@field pan? number
---@field mute? boolean

---@class TrackSettings
---@field patterns? PatternSlots
---@field activeSlot? integer -- 1..8
---@field synth? SynthSettings
---@field mixer? MixerSettings

---@param settings TrackSettings?
---@return TrackSettings
function TrackSettings(settings) end

---@param settings SynthSettings?
---@return SynthSettings
function SynthSettings(settings) end

---@param settings MixerSettings?
---@return MixerSettings
function MixerSettings(settings) end

---@param trackNumber integer -- 1..8
---@param settings MixerSettings
function mixer(trackNumber, settings) end

---@param trackNumber integer -- 1..8
---@param settings TrackSettings
function track(trackNumber, settings) end

---@param trackNumber integer -- 1..8
---@param settings SynthSettings
function synth(trackNumber, settings) end

