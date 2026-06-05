--- @generated from C++ metadata. Do not edit.
--- Source of truth: C++ metadata descriptors.
---@meta meh_groovebox_runtime

function panic() end

---@param filter? string
function params(filter) end

---@param fullName string
function get(fullName) end

---@param topic? string
function help(topic) end

---@param path string
function applyFile(path) end

function clear() end

function quit() end

transport = transport or {}

midi = midi or {}

seq = seq or {}

preset = preset or {}

mod = mod or {}

fm = fm or {}

fx = fx or {}

signal = signal or {}

mixer = mixer or {}

lp = lp

hp = hp

bp = bp

notch = notch

soft = soft

hard = hard

phaseReset = phaseReset

phaseFree = phaseFree

phaseRandom = phaseRandom

phaseSpread = phaseSpread

sine = sine

saw = saw

square = square

triangle = triangle

sineToSaw = sineToSaw

sah = sah

white = white

pink = pink

---@class transport
transport = transport or {}

---@param bpm number
function transport.setBPM(bpm) end

function transport.play() end

function transport.pause() end

function transport.stop() end

---@class midi
midi = midi or {}

---@param track integer
function midi.sticky(track) end

function midi.unsticky() end

---@param channel integer
---@param track integer
function midi.channel(channel, track) end

---@param channel integer
function midi.unchannel(channel) end

function midi.routes() end

---@class seq
seq = seq or {}

function seq.listTracks() end

---@param trackIndex integer
function seq.selectTrack(trackIndex) end

---@class preset
preset = preset or {}

---@param name string
function preset.load(name) end

---@param name string
function preset.save(name) end

function preset.init() end

function preset.list() end

function preset.dump() end

---@class mod
mod = mod or {}

---@param src string
---@param dest string
---@param amount number
function mod.add(src, dest, amount) end

---@param index integer
function mod.remove(index) end

function mod.clear() end

function mod.list() end

---@class fm
fm = fm or {}

---@param carrier string
---@param source string
---@param depth? number
function fm.add(carrier, source, depth) end

---@param carrier string
---@param source string
function fm.remove(carrier, source) end

---@param carrier string
function fm.clear(carrier) end

---@param carrier string
function fm.list(carrier) end

---@class fx
fx = fx or {}

---@param ... string
function fx.set(...) end

function fx.list() end

function fx.clear() end

---@class signal
signal = signal or {}

---@param ... string
function signal.set(...) end

function signal.list() end

function signal.clear() end

---@class mixer
mixer = mixer or {}

function mixer.list() end

---@class RuntimeAmpEnvParams
---@field attack number
---@field attackCurve number
---@field decay number
---@field decayCurve number
---@field release number
---@field releaseCurve number
---@field sustain number
---@type RuntimeAmpEnvParams
ampEnv = ampEnv or {}

---@class RuntimeFilterEnvParams
---@field attack number
---@field attackCurve number
---@field decay number
---@field decayCurve number
---@field release number
---@field releaseCurve number
---@field sustain number
---@type RuntimeFilterEnvParams
filterEnv = filterEnv or {}

---@class RuntimeFxChorusParams
---@field depth number
---@field enabled boolean
---@field feedback number
---@field mix number
---@field rate number
---@type RuntimeFxChorusParams
fx.chorus = fx.chorus or {}

---@class RuntimeFxDelayParams
---@field damping number
---@field enabled boolean
---@field feedback number
---@field hpDamping number
---@field mix number
---@field pingPong boolean
---@field subdivision number
---@field tempoSync boolean
---@field time number
---@type RuntimeFxDelayParams
fx.delay = fx.delay or {}

---@class RuntimeFxDistortionParams
---@field drive number
---@field enabled boolean
---@field mix number
---@field type number
---@type RuntimeFxDistortionParams
fx.distortion = fx.distortion or {}

---@class RuntimeFxPhaserParams
---@field depth number
---@field enabled boolean
---@field feedback number
---@field mix number
---@field rate number
---@field stages integer
---@type RuntimeFxPhaserParams
fx.phaser = fx.phaser or {}

---@class RuntimeFxReverbParams
---@field bandwidth number
---@field damping number
---@field decay number
---@field diffusion number
---@field enabled boolean
---@field lowDamping number
---@field mix number
---@field modDepth number
---@field modRate number
---@field preDelay number
---@type RuntimeFxReverbParams
fx.reverb = fx.reverb or {}

---@class RuntimeLadderParams
---@field cutoff number
---@field drive number
---@field enabled boolean
---@field resonance number
---@type RuntimeLadderParams
ladder = ladder or {}

---@class RuntimeLfo1Params
---@field amplitude number
---@field attack number
---@field bank number
---@field delay number
---@field rate number
---@field retrigger boolean
---@field subdivision number
---@field tempoSync boolean
---@type RuntimeLfo1Params
lfo1 = lfo1 or {}

---@class RuntimeLfo2Params
---@field amplitude number
---@field attack number
---@field bank number
---@field delay number
---@field rate number
---@field retrigger boolean
---@field subdivision number
---@field tempoSync boolean
---@type RuntimeLfo2Params
lfo2 = lfo2 or {}

---@class RuntimeLfo3Params
---@field amplitude number
---@field attack number
---@field bank number
---@field delay number
---@field rate number
---@field retrigger boolean
---@field subdivision number
---@field tempoSync boolean
---@type RuntimeLfo3Params
lfo3 = lfo3 or {}

---@class RuntimeMasterParams
---@field gain number
---@type RuntimeMasterParams
master = master or {}

---@class RuntimeModEnvParams
---@field attack number
---@field attackCurve number
---@field decay number
---@field decayCurve number
---@field release number
---@field releaseCurve number
---@field sustain number
---@type RuntimeModEnvParams
modEnv = modEnv or {}

---@class RuntimeMonoParams
---@field enabled boolean
---@field legato boolean
---@type RuntimeMonoParams
mono = mono or {}

---@class RuntimeNoiseParams
---@field enabled boolean
---@field mixLevel number
---@field type number
---@type RuntimeNoiseParams
noise = noise or {}

---@class RuntimeOsc1Params
---@field bank number
---@field detuneAmount number
---@field enabled boolean
---@field fixed boolean
---@field fixedFreq number
---@field fmDepth number
---@field mixLevel number
---@field octaveOffset integer
---@field phaseMode number
---@field randomRange number
---@field ratio number
---@field resetPhase number
---@field scanPos number
---@type RuntimeOsc1Params
osc1 = osc1 or {}

---@class RuntimeOsc2Params
---@field bank number
---@field detuneAmount number
---@field enabled boolean
---@field fixed boolean
---@field fixedFreq number
---@field fmDepth number
---@field mixLevel number
---@field octaveOffset integer
---@field phaseMode number
---@field randomRange number
---@field ratio number
---@field resetPhase number
---@field scanPos number
---@type RuntimeOsc2Params
osc2 = osc2 or {}

---@class RuntimeOsc3Params
---@field bank number
---@field detuneAmount number
---@field enabled boolean
---@field fixed boolean
---@field fixedFreq number
---@field fmDepth number
---@field mixLevel number
---@field octaveOffset integer
---@field phaseMode number
---@field randomRange number
---@field ratio number
---@field resetPhase number
---@field scanPos number
---@type RuntimeOsc3Params
osc3 = osc3 or {}

---@class RuntimeOsc4Params
---@field bank number
---@field detuneAmount number
---@field enabled boolean
---@field fixed boolean
---@field fixedFreq number
---@field fmDepth number
---@field mixLevel number
---@field octaveOffset integer
---@field phaseMode number
---@field randomRange number
---@field ratio number
---@field resetPhase number
---@field scanPos number
---@type RuntimeOsc4Params
osc4 = osc4 or {}

---@class RuntimePitchBendParams
---@field range number
---@type RuntimePitchBendParams
pitchBend = pitchBend or {}

---@class RuntimePortaParams
---@field enabled boolean
---@field legato boolean
---@field time number
---@type RuntimePortaParams
porta = porta or {}

---@class RuntimeSaturatorParams
---@field drive number
---@field enabled boolean
---@field mix number
---@type RuntimeSaturatorParams
saturator = saturator or {}

---@class RuntimeSvfParams
---@field cutoff number
---@field enabled boolean
---@field mode number
---@field resonance number
---@type RuntimeSvfParams
svf = svf or {}

---@class RuntimeUnisonParams
---@field detune number
---@field enabled boolean
---@field spread number
---@field voices integer
---@type RuntimeUnisonParams
unison = unison or {}

---@class RuntimeMixerParams
---@field limiterThreshold number
---@field masterGain number
---@type RuntimeMixerParams
mixer = mixer or {}

---@class RuntimeTrackParams
---@field gain number
---@field mute boolean
---@field pan number
---@type RuntimeTrackParams
track = track or {}

