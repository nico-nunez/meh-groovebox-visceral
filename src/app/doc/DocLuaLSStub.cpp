#include "app/doc/DocLuaLSStub.h"

#include "app/Constants.h"
#include "app/doc/DocMetadata.h"

#include <sstream>
#include <stdexcept>
#include <string>

namespace app::doc {
namespace {

void appendLine(std::string& out, const std::string& line = "") {
  out += line;
  out += '\n';
}

std::string luaType(DocLuaValueKind kind, const char* typeName) {
  if (typeName && typeName[0] != '\0')
    return typeName;

  switch (kind) {
  case DocLuaValueKind::Boolean:
    return "boolean";
  case DocLuaValueKind::Integer:
    return "integer";
  case DocLuaValueKind::Number:
    return "number";
  case DocLuaValueKind::String:
    return "string";
  case DocLuaValueKind::Table:
    return "table";
  case DocLuaValueKind::Function:
    return "function";
  case DocLuaValueKind::Any:
    return "any";
  }
  return "any";
}

std::string boundsComment(const DocIntegerBounds& bounds) {
  if (!bounds.hasMin && !bounds.hasMax)
    return "";

  std::ostringstream out;
  out << " -- ";
  if (bounds.hasMin)
    out << bounds.min;
  else
    out << "-inf";
  out << "..";
  if (bounds.hasMax)
    out << bounds.max;
  else
    out << "inf";
  return out.str();
}

void emitHeader(std::string& out, const char* metaName) {
  appendLine(out, "--- @generated from C++ metadata. Do not edit.");
  appendLine(out, "--- Source of truth: C++ metadata descriptors.");
  appendLine(out, std::string("---@meta ") + metaName);
  appendLine(out);
}

void emitAlias(std::string& out, const DocTypeMetadata& type) {
  appendLine(out, std::string("---@alias ") + type.name + std::string(" ") + type.aliasTarget);
  appendLine(out);
}

void emitDocumentClass(std::string& out, const DocTypeMetadata& type) {
  appendLine(out, std::string("---@class ") + type.name);
  for (const auto& field : type.fields) {
    std::string line = "---@field ";
    line += field.name;
    if (!field.required)
      line += "?";
    line += " ";
    line += luaType(field.kind, field.elementType);
    line += boundsComment(field.integerBounds);
    appendLine(out, line);
  }
  appendLine(out);
}

void emitDocumentFunction(std::string& out, const DocFunctionMetadata& fn) {
  for (const auto& arg : fn.args) {
    std::string line = "---@param ";
    line += arg.name;
    line += " ";
    line += luaType(arg.kind, arg.typeName);
    line += boundsComment(arg.integerBounds);
    appendLine(out, line);
  }
  if (fn.returnsTypeName && fn.returnsTypeName[0] != '\0')
    appendLine(out, std::string("---@return ") + fn.returnsTypeName);

  std::string signature = "function ";
  signature += fn.name;
  signature += "(";
  for (std::size_t i = 0; i < fn.args.size; ++i) {
    if (i != 0)
      signature += ", ";
    signature += fn.args.data[i].name;
  }
  signature += ") end";
  appendLine(out, signature);
  appendLine(out);
}

bool contains(const std::string& haystack, const std::string& needle) {
  return haystack.find(needle) != std::string::npos;
}

void requireContains(const std::string& file, const std::string& needle, const char* label) {
  if (!contains(file, needle))
    throw std::runtime_error(std::string("generated stub missing ") + label);
}

void requireNotContains(const std::string& file, const std::string& needle, const char* label) {
  if (contains(file, needle))
    throw std::runtime_error(std::string("generated stub unexpectedly contains ") + label);
}

} // namespace

std::string renderAuthoredDocumentLuaLSStub() {
  std::string out;
  emitHeader(out, "meh_groovebox_authored");
  appendLine(out, "--- Bounds:");
  appendLine(out, "--- trackNumber: 1.." + std::to_string(app::MAX_TRACKS));
  appendLine(out, "--- activeSlot: 1.." + std::to_string(app::sequencer::PATTERNS_PER_LANE));
  appendLine(out, "--- numSteps: 1.." + std::to_string(app::sequencer::MAX_PATTERN_STEPS));
  appendLine(out, "--- stepsPerBeat: 1.." + std::to_string(app::sequencer::MAX_STEPS_PER_BEAT));
  appendLine(out, "--- locks: at most " + std::to_string(app::sequencer::MAX_LOCKS_PER_STEP));
  appendLine(out);

  for (const auto& type : authoredDocumentTypes()) {
    switch (type.kind) {
    case DocMetadataKind::Struct:
      emitDocumentClass(out, type);
      break;
    case DocMetadataKind::Alias:
      emitAlias(out, type);
      break;
    }
  }

  for (const auto& ctor : authoredDocumentConstructors()) {
    appendLine(out, std::string("---@param settings ") + ctor + "?");
    appendLine(out, std::string("---@return ") + ctor);
    appendLine(out, std::string("function ") + ctor + "(settings) end");
    appendLine(out);
  }

  for (const auto& fn : authoredDocumentFunctions())
    emitDocumentFunction(out, fn);

  requireContains(out, "function track", "document track");
  requireContains(out, "function TrackSettings", "TrackSettings");
  requireContains(out, "function SynthSettings", "SynthSettings");
  requireContains(out, "function MixerSettings", "MixerSettings");
  requireNotContains(out, "applyFile", "applyFile");
  requireNotContains(out, "apply_file", "apply_file");
  requireNotContains(out, "transport", "transport");

  return out;
}

} // namespace app::doc
