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

---@class MixerSettings

---@param settings table?
---@return TrackSettings
function TrackSettings(settings) end

---@param settings table?
---@return SynthSettings
function SynthSettings(settings) end

---@param settings table?
---@return MixerSettings
function MixerSettings(settings) end

---@param trackNumber integer -- 1..8
---@param settings TrackSettings
function track(trackNumber, settings) end

