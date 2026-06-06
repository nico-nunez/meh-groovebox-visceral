#pragma once

#include "app/doc/DocMetadata.h"

#include <cstdint>
#include <string>
#include <vector>

namespace lua {

enum class RuntimeLuaSymbolKind : uint8_t {
  Function,
  Table,
  UserdataType,
  ProxyTable,
  EnumValue,
};

enum class RuntimeLuaValueKind : uint8_t {
  Any,
  Boolean,
  Integer,
  Number,
  String,
  Table,
  Function,
  Userdata,
};

enum class RuntimeLuaStatus : uint8_t {
  Implemented,
  Transitional,
};

struct RuntimeLuaIntegerBounds {
  int min = 0;
  int max = 0;
  bool hasMin = false;
  bool hasMax = false;
};

struct RuntimeLuaNumberBounds {
  double min = 0.0;
  double max = 0.0;
  bool hasMin = false;
  bool hasMax = false;
  bool finiteOnly = false;
};

struct RuntimeLuaArgMetadata {
  const char* name = "";
  RuntimeLuaValueKind kind = RuntimeLuaValueKind::Any;
  const char* typeName = "";
  bool optional = false;
  RuntimeLuaIntegerBounds integerBounds{};
  RuntimeLuaNumberBounds numberBounds{};
  const char* doc = "";
};

struct RuntimeLuaFunctionMetadata {
  const char* name = "";
  RuntimeLuaStatus status = RuntimeLuaStatus::Implemented;
  app::doc::DocMetadataSpan<RuntimeLuaArgMetadata> args{};
  const char* returnsTypeName = "";
  const char* doc = "";
};

struct RuntimeLuaTableMetadata {
  const char* name = "";
  RuntimeLuaSymbolKind kind = RuntimeLuaSymbolKind::Table;
  RuntimeLuaStatus status = RuntimeLuaStatus::Implemented;
  app::doc::DocMetadataSpan<RuntimeLuaFunctionMetadata> methods{};
  const char* doc = "";
};

struct RuntimeLuaProxyFieldMetadata {
  std::string table{};
  std::string field{};
  RuntimeLuaValueKind valueKind = RuntimeLuaValueKind::Number;
  const char* sourceParamName = "";
  const char* doc = "";
};

struct RuntimeLuaSymbolMetadata {
  const char* name = "";
  RuntimeLuaSymbolKind kind = RuntimeLuaSymbolKind::Function;
  RuntimeLuaStatus status = RuntimeLuaStatus::Implemented;
  const RuntimeLuaFunctionMetadata* function = nullptr;
  const RuntimeLuaTableMetadata* table = nullptr;
  const char* doc = "";
};

app::doc::DocMetadataSpan<RuntimeLuaSymbolMetadata> runtimeLuaGlobals();
app::doc::DocMetadataSpan<RuntimeLuaTableMetadata> runtimeLuaTables();
app::doc::DocMetadataSpan<RuntimeLuaTableMetadata> runtimeLuaUserdataTypes();

const RuntimeLuaSymbolMetadata* findRuntimeLuaGlobal(const char* name);
const RuntimeLuaTableMetadata* findRuntimeLuaTable(const char* name);
const RuntimeLuaTableMetadata* findRuntimeLuaUserdataType(const char* name);

void collectRuntimeLuaEngineParamProxyFields(std::vector<RuntimeLuaProxyFieldMetadata>& out);
void collectRuntimeLuaAppParamProxyFields(std::vector<RuntimeLuaProxyFieldMetadata>& out);

namespace rtglobal {
inline constexpr const char* Panic = "panic";
inline constexpr const char* Params = "params";
inline constexpr const char* Get = "get";
inline constexpr const char* Help = "help";
inline constexpr const char* ApplyFile = "applyFile";
inline constexpr const char* Clear = "clear";
inline constexpr const char* Quit = "quit";

inline constexpr const char* Transport = "transport";
inline constexpr const char* Midi = "midi";
inline constexpr const char* Seq = "seq";
inline constexpr const char* Preset = "preset";
inline constexpr const char* Mod = "mod";
inline constexpr const char* Fm = "fm";
inline constexpr const char* Fx = "fx";
inline constexpr const char* Signal = "signal";
inline constexpr const char* Mixer = "mixer";

inline constexpr const char* Lp = "lp";
inline constexpr const char* Hp = "hp";
inline constexpr const char* Bp = "bp";
inline constexpr const char* Notch = "notch";
inline constexpr const char* Soft = "soft";
inline constexpr const char* Hard = "hard";
inline constexpr const char* PhaseReset = "phaseReset";
inline constexpr const char* PhaseFree = "phaseFree";
inline constexpr const char* PhaseRandom = "phaseRandom";
inline constexpr const char* PhaseSpread = "phaseSpread";
inline constexpr const char* Sine = "sine";
inline constexpr const char* Saw = "saw";
inline constexpr const char* Square = "square";
inline constexpr const char* Triangle = "triangle";
inline constexpr const char* SineToSaw = "sineToSaw";
inline constexpr const char* Sah = "sah";
inline constexpr const char* White = "white";
inline constexpr const char* Pink = "pink";
} // namespace rtglobal

namespace rtmethod {
inline constexpr const char* SetBPM = "setBPM";
inline constexpr const char* Play = "play";
inline constexpr const char* Pause = "pause";
inline constexpr const char* Stop = "stop";
inline constexpr const char* Sticky = "sticky";
inline constexpr const char* Unsticky = "unsticky";
inline constexpr const char* Channel = "channel";
inline constexpr const char* Unchannel = "unchannel";
inline constexpr const char* Routes = "routes";
inline constexpr const char* Add = "add";
inline constexpr const char* Remove = "remove";
inline constexpr const char* Clear = "clear";
inline constexpr const char* List = "list";
inline constexpr const char* Load = "load";
inline constexpr const char* Save = "save";
inline constexpr const char* Init = "init";
inline constexpr const char* Dump = "dump";
inline constexpr const char* Set = "set";
inline constexpr const char* ListTracks = "listTracks";
inline constexpr const char* SelectTrack = "selectTrack";
} // namespace rtmethod

} // namespace lua
