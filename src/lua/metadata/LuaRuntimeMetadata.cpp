#include "LuaRuntimeMetadata.h"

#include "app/AppParams.h"
#include "app/Constants.h"
#include "app/Sequencer.h"
#include "synth/params/ParamDefs.h"

#include <cstring>
#include <string>

namespace lua {
namespace {

template <typename T, std::size_t N>
constexpr app::doc::DocMetadataSpan<T> spanOf(const T (&items)[N]) {
  return {items, N};
}

bool equals(const char* a, const char* b) {
  return a && b && std::strcmp(a, b) == 0;
}

constexpr RuntimeLuaIntegerBounds intBounds(int min, int max) {
  return {min, max, true, true};
}

constexpr RuntimeLuaArgMetadata
arg(const char* name, RuntimeLuaValueKind kind, const char* typeName = "", bool optional = false) {
  return {name, kind, typeName, optional};
}

constexpr RuntimeLuaArgMetadata intArg(const char* name, int min, int max) {
  return {name, RuntimeLuaValueKind::Integer, "", false, intBounds(min, max)};
}

constexpr RuntimeLuaFunctionMetadata fn(const char* name,
                                        app::doc::DocMetadataSpan<RuntimeLuaArgMetadata> args = {},
                                        const char* returnsTypeName = "",
                                        RuntimeLuaStatus status = RuntimeLuaStatus::Implemented,
                                        const char* doc = "") {
  return {name, status, args, returnsTypeName, doc};
}

constexpr RuntimeLuaArgMetadata kPathArg[] = {
    arg("path", RuntimeLuaValueKind::String),
};
constexpr RuntimeLuaArgMetadata kOptionalFilterArg[] = {
    arg("filter", RuntimeLuaValueKind::String, "", true),
};
constexpr RuntimeLuaArgMetadata kFullNameArg[] = {
    arg("fullName", RuntimeLuaValueKind::String),
};
constexpr RuntimeLuaArgMetadata kOptionalTopicArg[] = {
    arg("topic", RuntimeLuaValueKind::String, "", true),
};

constexpr RuntimeLuaFunctionMetadata kPanicFunction = fn(rtglobal::Panic);
constexpr RuntimeLuaFunctionMetadata kParamsFunction =
    fn(rtglobal::Params, spanOf(kOptionalFilterArg));
constexpr RuntimeLuaFunctionMetadata kGetFunction = fn(rtglobal::Get, spanOf(kFullNameArg));
constexpr RuntimeLuaFunctionMetadata kHelpFunction = fn(rtglobal::Help, spanOf(kOptionalTopicArg));
constexpr RuntimeLuaFunctionMetadata kApplyFileFunction =
    fn(rtglobal::ApplyFile,
       spanOf(kPathArg),
       "",
       RuntimeLuaStatus::Transitional,
       "Apply an authored document file through app::doc::applySequencerFile.");
constexpr RuntimeLuaFunctionMetadata kClearFunction = fn(rtglobal::Clear);
constexpr RuntimeLuaFunctionMetadata kQuitFunction = fn(rtglobal::Quit);

constexpr RuntimeLuaArgMetadata kBpmArg[] = {
    arg("bpm", RuntimeLuaValueKind::Number),
};
constexpr RuntimeLuaFunctionMetadata kTransportMethods[] = {
    fn(rtmethod::SetBPM, spanOf(kBpmArg)),
    fn(rtmethod::Play),
    fn(rtmethod::Pause),
    fn(rtmethod::Stop),
};

constexpr RuntimeLuaArgMetadata kMidiStickyArgs[] = {
    intArg("track", 1, app::MAX_TRACKS),
};
constexpr RuntimeLuaArgMetadata kMidiChannelArgs[] = {
    intArg("channel", 1, app::MAX_MIDI_CHANNELS),
    intArg("track", 1, app::MAX_TRACKS),
};
constexpr RuntimeLuaArgMetadata kMidiUnchannelArgs[] = {
    intArg("channel", 1, app::MAX_MIDI_CHANNELS),
};
constexpr RuntimeLuaFunctionMetadata kMidiMethods[] = {
    fn(rtmethod::Sticky, spanOf(kMidiStickyArgs)),
    fn(rtmethod::Unsticky),
    fn(rtmethod::Channel, spanOf(kMidiChannelArgs)),
    fn(rtmethod::Unchannel, spanOf(kMidiUnchannelArgs)),
    fn(rtmethod::Routes),
};

constexpr RuntimeLuaArgMetadata kModAddArgs[] = {
    arg("src", RuntimeLuaValueKind::String),
    arg("dest", RuntimeLuaValueKind::String),
    arg("amount", RuntimeLuaValueKind::Number),
};
constexpr RuntimeLuaArgMetadata kModRemoveArgs[] = {
    arg("index", RuntimeLuaValueKind::Integer),
};
constexpr RuntimeLuaFunctionMetadata kModMethods[] = {
    fn(rtmethod::Add, spanOf(kModAddArgs)),
    fn(rtmethod::Remove, spanOf(kModRemoveArgs)),
    fn(rtmethod::Clear),
    fn(rtmethod::List),
};

constexpr RuntimeLuaArgMetadata kFmAddArgs[] = {
    arg("carrier", RuntimeLuaValueKind::String),
    arg("source", RuntimeLuaValueKind::String),
    arg("depth", RuntimeLuaValueKind::Number, "", true),
};
constexpr RuntimeLuaArgMetadata kFmRouteArgs[] = {
    arg("carrier", RuntimeLuaValueKind::String),
    arg("source", RuntimeLuaValueKind::String),
};
constexpr RuntimeLuaArgMetadata kFmCarrierArg[] = {
    arg("carrier", RuntimeLuaValueKind::String),
};
constexpr RuntimeLuaFunctionMetadata kFmMethods[] = {
    fn(rtmethod::Add, spanOf(kFmAddArgs)),
    fn(rtmethod::Remove, spanOf(kFmRouteArgs)),
    fn(rtmethod::Clear, spanOf(kFmCarrierArg)),
    fn(rtmethod::List, spanOf(kFmCarrierArg)),
};

constexpr RuntimeLuaArgMetadata kPresetNameArg[] = {
    arg("name", RuntimeLuaValueKind::String),
};
constexpr RuntimeLuaFunctionMetadata kPresetMethods[] = {
    fn(rtmethod::Load, spanOf(kPresetNameArg)),
    fn(rtmethod::Save, spanOf(kPresetNameArg)),
    fn(rtmethod::Init),
    fn(rtmethod::List),
    fn(rtmethod::Dump),
};

constexpr RuntimeLuaArgMetadata kVariadicNamesArg[] = {
    arg("...", RuntimeLuaValueKind::String),
};
constexpr RuntimeLuaFunctionMetadata kFxMethods[] = {
    fn(rtmethod::Set,
       spanOf(kVariadicNamesArg),
       "",
       RuntimeLuaStatus::Implemented,
       "Variadic effect processor names."),
    fn(rtmethod::List),
    fn(rtmethod::Clear),
};

constexpr RuntimeLuaFunctionMetadata kSignalMethods[] = {
    fn(rtmethod::Set,
       spanOf(kVariadicNamesArg),
       "",
       RuntimeLuaStatus::Implemented,
       "Variadic signal processor names."),
    fn(rtmethod::List),
    fn(rtmethod::Clear),
};

constexpr RuntimeLuaFunctionMetadata kMixerMethods[] = {
    fn(rtmethod::List),
};

constexpr RuntimeLuaArgMetadata kOptionalTrackArg[] = {
    {
        "trackIndex",
        RuntimeLuaValueKind::Integer,
        "",
        true,
        intBounds(1, app::MAX_TRACKS),
    },
};
constexpr RuntimeLuaArgMetadata kSelectTrackArgs[] = {
    intArg("trackIndex", 1, app::MAX_TRACKS),
};
constexpr RuntimeLuaFunctionMetadata kSeqMethods[] = {
    fn(rtmethod::Track, spanOf(kOptionalTrackArg), rttype::SeqTrack),
    fn(rtmethod::ListTracks),
    fn(rtmethod::SelectTrack, spanOf(kSelectTrackArgs)),
    fn(rtmethod::Edit),
    fn(rtmethod::New),
    fn(rtmethod::Commit),
};

constexpr RuntimeLuaArgMetadata kStepIndexArg[] = {
    intArg("stepIndex", 1, app::sequencer::MAX_PATTERN_STEPS),
};
constexpr RuntimeLuaArgMetadata kNumStepsArg[] = {
    intArg("numSteps", 1, app::sequencer::MAX_PATTERN_STEPS),
};
constexpr RuntimeLuaArgMetadata kStepsPerBeatArg[] = {
    intArg("stepsPerBeat", 1, app::sequencer::MAX_STEPS_PER_BEAT),
};
constexpr RuntimeLuaArgMetadata kValuesTableArg[] = {
    arg("values", RuntimeLuaValueKind::Table),
};
constexpr RuntimeLuaArgMetadata kPatternBankArg[] = {
    arg("bank", RuntimeLuaValueKind::Table, rttype::RuntimePatternBank),
};
constexpr RuntimeLuaArgMetadata kReplacePatternArgs[] = {
    intArg("slot", 1, app::sequencer::PATTERNS_PER_LANE),
    arg("pattern", RuntimeLuaValueKind::Table, rttype::RuntimePattern),
};
constexpr RuntimeLuaArgMetadata kPatternSlotArg[] = {
    intArg("slot", 1, app::sequencer::PATTERNS_PER_LANE),
};
constexpr RuntimeLuaFunctionMetadata kSeqTrackMethods[] = {
    fn(rtmethod::Step, spanOf(kStepIndexArg), rttype::SeqStep),
    fn(rtmethod::SetNumSteps, spanOf(kNumStepsArg)),
    fn(rtmethod::SetStepsPerBeat, spanOf(kStepsPerBeatArg)),
    fn(rtmethod::GetPattern, {}, rttype::RuntimePattern),
    fn(rtmethod::SetPattern, spanOf(kValuesTableArg)),
    fn(rtmethod::SetNotes, spanOf(kValuesTableArg)),
    fn(rtmethod::SetVelocities, spanOf(kValuesTableArg)),
    fn(rtmethod::ResetPattern),
    fn(rtmethod::GetPatterns, {}, rttype::RuntimePatternBank),
    fn(rtmethod::ReplacePatterns, spanOf(kPatternBankArg)),
    fn(rtmethod::ReplacePattern, spanOf(kReplacePatternArgs)),
    fn(rtmethod::ClearPatternSlot, spanOf(kPatternSlotArg)),
    fn(rtmethod::Clear),
};

constexpr RuntimeLuaArgMetadata kStepEventArg[] = {
    arg("event", RuntimeLuaValueKind::Table, rttype::RuntimeStep),
};
constexpr RuntimeLuaArgMetadata kBoolActiveArg[] = {
    arg("active", RuntimeLuaValueKind::Boolean),
};
constexpr RuntimeLuaArgMetadata kBoolNoteOnArg[] = {
    arg("noteOn", RuntimeLuaValueKind::Boolean),
};
constexpr RuntimeLuaArgMetadata kNoteArg[] = {
    intArg("note", 0, 127),
};
constexpr RuntimeLuaArgMetadata kVelocityArg[] = {
    intArg("velocity", 0, 127),
};
constexpr RuntimeLuaArgMetadata kGateArg[] = {
    arg("gate", RuntimeLuaValueKind::Number),
};
constexpr RuntimeLuaArgMetadata kLegatoArg[] = {
    arg("legato", RuntimeLuaValueKind::Boolean),
};
constexpr RuntimeLuaArgMetadata kLockArgs[] = {
    arg("paramName", RuntimeLuaValueKind::String),
    arg("value", RuntimeLuaValueKind::Number),
};
constexpr RuntimeLuaArgMetadata kLockParamArg[] = {
    arg("paramName", RuntimeLuaValueKind::String),
};
constexpr RuntimeLuaFunctionMetadata kSeqStepMethods[] = {
    fn(rtmethod::Get, {}, rttype::RuntimeStep),
    fn(rtmethod::Set, spanOf(kStepEventArg)),
    fn(rtmethod::Clear),
    fn(rtmethod::SetActive, spanOf(kBoolActiveArg)),
    fn(rtmethod::SetNoteOn, spanOf(kBoolNoteOnArg)),
    fn(rtmethod::SetNote, spanOf(kNoteArg)),
    fn(rtmethod::SetVelocity, spanOf(kVelocityArg)),
    fn(rtmethod::SetGate, spanOf(kGateArg)),
    fn(rtmethod::SetLegato, spanOf(kLegatoArg)),
    fn(rtmethod::SetLock, spanOf(kLockArgs)),
    fn(rtmethod::ClearLock, spanOf(kLockParamArg)),
    fn(rtmethod::ClearLocks),
};

constexpr RuntimeLuaTableMetadata kRuntimeTables[] = {
    {rtglobal::Transport,
     RuntimeLuaSymbolKind::Table,
     RuntimeLuaStatus::Implemented,
     spanOf(kTransportMethods),
     "Transport commands."},
    {rtglobal::Midi,
     RuntimeLuaSymbolKind::Table,
     RuntimeLuaStatus::Implemented,
     spanOf(kMidiMethods),
     "MIDI routing commands."},
    {rtglobal::Seq,
     RuntimeLuaSymbolKind::Table,
     RuntimeLuaStatus::Implemented,
     spanOf(kSeqMethods),
     "Runtime sequencer commands."},
    {rtglobal::Preset,
     RuntimeLuaSymbolKind::Table,
     RuntimeLuaStatus::Implemented,
     spanOf(kPresetMethods),
     "Preset commands."},
    {rtglobal::Mod,
     RuntimeLuaSymbolKind::Table,
     RuntimeLuaStatus::Implemented,
     spanOf(kModMethods),
     "Mod matrix commands."},
    {rtglobal::Fm,
     RuntimeLuaSymbolKind::Table,
     RuntimeLuaStatus::Implemented,
     spanOf(kFmMethods),
     "FM routing commands."},
    {rtglobal::Fx,
     RuntimeLuaSymbolKind::ProxyTable,
     RuntimeLuaStatus::Implemented,
     spanOf(kFxMethods),
     "FX chain commands and FX parameter proxy parent."},
    {rtglobal::Signal,
     RuntimeLuaSymbolKind::Table,
     RuntimeLuaStatus::Implemented,
     spanOf(kSignalMethods),
     "Voice signal chain commands."},
    {rtglobal::Mixer,
     RuntimeLuaSymbolKind::ProxyTable,
     RuntimeLuaStatus::Implemented,
     spanOf(kMixerMethods),
     "Mixer commands and app parameter proxy."},
};

constexpr RuntimeLuaTableMetadata kRuntimeUserdataTypes[] = {
    {rttype::SeqTrack,
     RuntimeLuaSymbolKind::UserdataType,
     RuntimeLuaStatus::Implemented,
     spanOf(kSeqTrackMethods),
     "Runtime sequencer track userdata."},
    {rttype::SeqStep,
     RuntimeLuaSymbolKind::UserdataType,
     RuntimeLuaStatus::Implemented,
     spanOf(kSeqStepMethods),
     "Runtime sequencer step userdata."},
};

constexpr RuntimeLuaSymbolMetadata kRuntimeGlobals[] = {
    {rtglobal::Panic,
     RuntimeLuaSymbolKind::Function,
     RuntimeLuaStatus::Implemented,
     &kPanicFunction},
    {rtglobal::Params,
     RuntimeLuaSymbolKind::Function,
     RuntimeLuaStatus::Implemented,
     &kParamsFunction},
    {rtglobal::Get, RuntimeLuaSymbolKind::Function, RuntimeLuaStatus::Implemented, &kGetFunction},
    {rtglobal::Help, RuntimeLuaSymbolKind::Function, RuntimeLuaStatus::Implemented, &kHelpFunction},
    {rtglobal::ApplyFile,
     RuntimeLuaSymbolKind::Function,
     RuntimeLuaStatus::Transitional,
     &kApplyFileFunction},
    {rtglobal::Clear,
     RuntimeLuaSymbolKind::Function,
     RuntimeLuaStatus::Implemented,
     &kClearFunction},
    {rtglobal::Quit, RuntimeLuaSymbolKind::Function, RuntimeLuaStatus::Implemented, &kQuitFunction},
    {rtglobal::Transport,
     RuntimeLuaSymbolKind::Table,
     RuntimeLuaStatus::Implemented,
     nullptr,
     &kRuntimeTables[0]},
    {rtglobal::Midi,
     RuntimeLuaSymbolKind::Table,
     RuntimeLuaStatus::Implemented,
     nullptr,
     &kRuntimeTables[1]},
    {rtglobal::Seq,
     RuntimeLuaSymbolKind::Table,
     RuntimeLuaStatus::Implemented,
     nullptr,
     &kRuntimeTables[2]},
    {rtglobal::Preset,
     RuntimeLuaSymbolKind::Table,
     RuntimeLuaStatus::Implemented,
     nullptr,
     &kRuntimeTables[3]},
    {rtglobal::Mod,
     RuntimeLuaSymbolKind::Table,
     RuntimeLuaStatus::Implemented,
     nullptr,
     &kRuntimeTables[4]},
    {rtglobal::Fm,
     RuntimeLuaSymbolKind::Table,
     RuntimeLuaStatus::Implemented,
     nullptr,
     &kRuntimeTables[5]},
    {rtglobal::Fx,
     RuntimeLuaSymbolKind::ProxyTable,
     RuntimeLuaStatus::Implemented,
     nullptr,
     &kRuntimeTables[6]},
    {rtglobal::Signal,
     RuntimeLuaSymbolKind::Table,
     RuntimeLuaStatus::Implemented,
     nullptr,
     &kRuntimeTables[7]},
    {rtglobal::Mixer,
     RuntimeLuaSymbolKind::ProxyTable,
     RuntimeLuaStatus::Implemented,
     nullptr,
     &kRuntimeTables[8]},
    {rtglobal::Lp, RuntimeLuaSymbolKind::EnumValue},
    {rtglobal::Hp, RuntimeLuaSymbolKind::EnumValue},
    {rtglobal::Bp, RuntimeLuaSymbolKind::EnumValue},
    {rtglobal::Notch, RuntimeLuaSymbolKind::EnumValue},
    {rtglobal::Soft, RuntimeLuaSymbolKind::EnumValue},
    {rtglobal::Hard, RuntimeLuaSymbolKind::EnumValue},
    {rtglobal::PhaseReset, RuntimeLuaSymbolKind::EnumValue},
    {rtglobal::PhaseFree, RuntimeLuaSymbolKind::EnumValue},
    {rtglobal::PhaseRandom, RuntimeLuaSymbolKind::EnumValue},
    {rtglobal::PhaseSpread, RuntimeLuaSymbolKind::EnumValue},
    {rtglobal::Sine, RuntimeLuaSymbolKind::EnumValue},
    {rtglobal::Saw, RuntimeLuaSymbolKind::EnumValue},
    {rtglobal::Square, RuntimeLuaSymbolKind::EnumValue},
    {rtglobal::Triangle, RuntimeLuaSymbolKind::EnumValue},
    {rtglobal::SineToSaw, RuntimeLuaSymbolKind::EnumValue},
    {rtglobal::Sah, RuntimeLuaSymbolKind::EnumValue},
    {rtglobal::White, RuntimeLuaSymbolKind::EnumValue},
    {rtglobal::Pink, RuntimeLuaSymbolKind::EnumValue},
};

RuntimeLuaValueKind engineParamValueKind(synth::param::ParamType type) {
  using Type = synth::param::ParamType;
  switch (type) {
  case Type::Bool:
    return RuntimeLuaValueKind::Boolean;
  case Type::Int8:
    return RuntimeLuaValueKind::Integer;
  case Type::Float:
  case Type::OscBankID:
  case Type::PhaseMode:
  case Type::NoiseType:
  case Type::FilterMode:
  case Type::DistortionType:
  case Type::Subdivision:
    return RuntimeLuaValueKind::Number;
  }
  return RuntimeLuaValueKind::Number;
}

RuntimeLuaValueKind appParamValueKind(app::params::AppParamType type) {
  switch (type) {
  case app::params::AppParamType::Bool:
    return RuntimeLuaValueKind::Boolean;
  case app::params::AppParamType::Float:
    return RuntimeLuaValueKind::Number;
  }
  return RuntimeLuaValueKind::Number;
}

void splitEngineParamName(const char* fullName, std::string& table, std::string& field) {
  const char* dot = std::strchr(fullName, '.');
  if (!dot) {
    table.clear();
    field.clear();
    return;
  }

  const char* secondDot = std::strchr(dot + 1, '.');
  if (secondDot) {
    table.assign(fullName, static_cast<std::size_t>(secondDot - fullName));
    field.assign(secondDot + 1);
  } else {
    table.assign(fullName, static_cast<std::size_t>(dot - fullName));
    field.assign(dot + 1);
  }
}

} // namespace

app::doc::DocMetadataSpan<RuntimeLuaSymbolMetadata> runtimeLuaGlobals() {
  return spanOf(kRuntimeGlobals);
}

app::doc::DocMetadataSpan<RuntimeLuaTableMetadata> runtimeLuaTables() {
  return spanOf(kRuntimeTables);
}

app::doc::DocMetadataSpan<RuntimeLuaTableMetadata> runtimeLuaUserdataTypes() {
  return spanOf(kRuntimeUserdataTypes);
}

const RuntimeLuaSymbolMetadata* findRuntimeLuaGlobal(const char* name) {
  for (const auto& global : runtimeLuaGlobals()) {
    if (equals(global.name, name))
      return &global;
  }
  return nullptr;
}

const RuntimeLuaTableMetadata* findRuntimeLuaTable(const char* name) {
  for (const auto& table : runtimeLuaTables()) {
    if (equals(table.name, name))
      return &table;
  }
  return nullptr;
}

const RuntimeLuaTableMetadata* findRuntimeLuaUserdataType(const char* name) {
  for (const auto& type : runtimeLuaUserdataTypes()) {
    if (equals(type.name, name))
      return &type;
  }
  return nullptr;
}

void collectRuntimeLuaEngineParamProxyFields(std::vector<RuntimeLuaProxyFieldMetadata>& out) {
  for (const auto& def : synth::param::PARAM_DEFS) {
    std::string table{};
    std::string field{};
    splitEngineParamName(def.name, table, field);
    if (table.empty() || field.empty())
      continue;

    out.push_back({table, field, engineParamValueKind(def.type), def.name});
  }
}

void collectRuntimeLuaAppParamProxyFields(std::vector<RuntimeLuaProxyFieldMetadata>& out) {
  for (const auto& def : app::params::APP_PARAM_DEFS) {
    out.push_back({def.table, def.field, appParamValueKind(def.type), def.path});
  }
}

} // namespace lua
