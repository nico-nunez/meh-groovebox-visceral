--- @generated from C++ metadata. Do not edit.
--- Source of truth: C++ metadata descriptors.
---@meta meh_groovebox_authored

--- Bounds:
--- trackNumber: 1..8
--- activeSlot: 1..8
--- numSteps: 1..64
--- stepsPerBeat: 1..48
--- locks: at most 4

---@class TrackSettings
---@field patterns? PatternSlots
---@field activeSlot? integer -- 1..8
---@field synth? SynthSettings
---@field mixer? MixerSettings

---@class PatternSlots

---@class Pattern
---@field numSteps integer -- 1..64
---@field stepsPerBeat integer -- 1..48
---@field steps Step

---@class Step
---@field active? boolean
---@field note? integer -- 0..127
---@field velocity? integer -- 0..127
---@field gate? number
---@field legato? boolean
---@field locks? StepLock

---@class StepLock
---@field param string
---@field value number

---@class SynthSettings
---@field osc1? SynthOscSettings
---@field osc2? SynthOscSettings
---@field osc3? SynthOscSettings
---@field osc4? SynthOscSettings
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
---@field master? SynthMasterSettings
---@field fx? SynthFXSettings

---@class SynthOscSettings
---@field enabled? boolean
---@field bank? string
---@field mix? number
---@field detune? number
---@field octave? integer
---@field scan? number
---@field fmDepth? number
---@field phaseMode? string
---@field randomRange? number
---@field resetPhase? number
---@field phaseMode? string
---@field ratio? number
---@field fixed? boolean
---@field fixedFreq? number

---@class SynthNoiseSettings
---@field enabled? boolean
---@field type? string
---@field mix? number

---@class SynthAmpEnvSettings
---@field attack? number
---@field decay? number
---@field sustain? number
---@field release? number

---@class SynthModEnvSettings
---@field attack? number
---@field decay? number
---@field sustain? number
---@field release? number

---@class SynthFilterEnvSettings
---@field attack? number
---@field decay? number
---@field sustain? number
---@field release? number

---@class SynthSVFSettings
---@field enabled? boolean
---@field mode? string
---@field cutoff? number
---@field resonance? number

---@class SynthLadderSettings
---@field enabled? boolean
---@field cutoff? number
---@field resonance? number
---@field drive? number

---@class SynthSaturatorSettings
---@field enabled? boolean
---@field drive? number
---@field mix? number

---@class SynthMonoSettings
---@field enabled? boolean
---@field legato? boolean

---@class SynthPortaSettings
---@field enabled? boolean
---@field time? number
---@field legato? boolean

---@class SynthUnisonSettings
---@field enabled? boolean
---@field voices? integer
---@field detune? number
---@field spread? number

---@class SynthPitchBendSettings
---@field range? number

---@class SynthMasterSettings
---@field gain? number

---@class SynthFXSettings
---@field distortion? SynthFXUnitSettings
---@field chorus? SynthFXUnitSettings
---@field phaser? SynthFXUnitSettings
---@field delay? SynthFXUnitSettings
---@field reverb? SynthFXUnitSettings

---@class SynthFXUnitSettings
---@field enabled? boolean
---@field mix? number

---@class MixerSettings
---@field gain? number
---@field pan? number
---@field mute? boolean

---@class MixerTrackSettings
---@field gain? number
---@field pan? number
---@field mute? boolean

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

