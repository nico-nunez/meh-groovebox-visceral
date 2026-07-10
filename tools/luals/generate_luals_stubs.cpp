#include "app/doc/DocLuaLSStub.h"
#include "lua/metadata/LuaRuntimeMetadata.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct RenderedFile {
  std::string relativePath{};
  std::string contents{};
};

struct Args {
  std::filesystem::path outDir{};
  bool check = false;
};

void appendLine(std::string& out, const std::string& line = "") {
  out += line;
  out += '\n';
}

std::string usage() {
  return "usage: generate_luals_stubs --out generated/luals [--check]\n";
}

bool parseArgs(int argc, char** argv, Args& out) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--out") {
      if (i + 1 >= argc)
        return false;
      out.outDir = argv[++i];
      continue;
    }
    if (arg == "--check") {
      out.check = true;
      continue;
    }
    if (arg == "--help" || arg == "-h") {
      std::cout << usage();
      std::exit(0);
    }
    return false;
  }
  return !out.outDir.empty();
}

std::string runtimeLuaType(lua::RuntimeLuaValueKind kind, const char* typeName) {
  if (typeName && typeName[0] != '\0')
    return typeName;

  switch (kind) {
  case lua::RuntimeLuaValueKind::Boolean:
    return "boolean";
  case lua::RuntimeLuaValueKind::Integer:
    return "integer";
  case lua::RuntimeLuaValueKind::Number:
    return "number";
  case lua::RuntimeLuaValueKind::String:
    return "string";
  case lua::RuntimeLuaValueKind::Table:
    return "table";
  case lua::RuntimeLuaValueKind::Function:
    return "function";
  case lua::RuntimeLuaValueKind::Userdata:
    return "userdata";
  case lua::RuntimeLuaValueKind::Any:
    return "any";
  }
  return "any";
}

void emitHeader(std::string& out, const char* metaName) {
  appendLine(out, "--- @generated from C++ metadata. Do not edit.");
  appendLine(out, "--- Source of truth: C++ metadata descriptors.");
  appendLine(out, std::string("---@meta ") + metaName);
  appendLine(out);
}

void emitRuntimeFunction(std::string& out,
                         const lua::RuntimeLuaFunctionMetadata& fn,
                         const std::string& receiver) {
  for (const auto& arg : fn.args) {
    std::string line = "---@param ";
    line += arg.name;
    if (arg.optional)
      line += "?";
    line += " ";
    line += runtimeLuaType(arg.kind, arg.typeName);
    appendLine(out, line);
  }
  if (fn.returnsTypeName && fn.returnsTypeName[0] != '\0')
    appendLine(out, std::string("---@return ") + fn.returnsTypeName);

  std::string signature = "function ";
  if (!receiver.empty()) {
    signature += receiver;
    signature += ".";
  }
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

RenderedFile renderAuthoredDocumentStub() {
  return {"authored_document/meh_groovebox_authored.lua",
          app::doc::renderAuthoredDocumentLuaLSStub()};
}

void emitRuntimeTable(std::string& out, const lua::RuntimeLuaTableMetadata& table) {
  appendLine(out, std::string("---@class ") + table.name);
  appendLine(out, std::string(table.name) + " = " + table.name + " or {}");
  appendLine(out);
  for (const auto& method : table.methods)
    emitRuntimeFunction(out, method, table.name);
}

struct ProxyTable {
  std::string table{};
  std::vector<lua::RuntimeLuaProxyFieldMetadata> fields{};
};

std::vector<ProxyTable> groupProxyFields(std::vector<lua::RuntimeLuaProxyFieldMetadata> fields) {
  std::sort(fields.begin(), fields.end(), [](const auto& a, const auto& b) {
    if (a.table != b.table)
      return a.table < b.table;
    return a.field < b.field;
  });

  std::vector<ProxyTable> grouped{};
  for (const auto& field : fields) {
    if (grouped.empty() || grouped.back().table != field.table)
      grouped.push_back({field.table, {}});
    grouped.back().fields.push_back(field);
  }
  return grouped;
}

std::string classNameForProxyTable(const std::string& table) {
  std::string name = "Runtime";
  bool capitalize = true;
  for (const char ch : table) {
    if (ch == '.') {
      capitalize = true;
      continue;
    }
    const char rendered =
        capitalize ? static_cast<char>(std::toupper(static_cast<unsigned char>(ch))) : ch;
    name += rendered;
    capitalize = false;
  }
  name += "Params";
  return name;
}

void emitProxyTables(std::string& out, std::vector<lua::RuntimeLuaProxyFieldMetadata> fields) {
  for (const auto& table : groupProxyFields(std::move(fields))) {
    const std::string className = classNameForProxyTable(table.table);
    appendLine(out, "---@class " + className);
    for (const auto& field : table.fields) {
      appendLine(out, "---@field " + field.field + " " + runtimeLuaType(field.valueKind, ""));
    }
    appendLine(out, "---@type " + className);
    appendLine(out, table.table + " = " + table.table + " or {}");
    appendLine(out);
  }
}

RenderedFile renderRuntimeLuaStub() {
  std::string out;
  emitHeader(out, "meh_groovebox_runtime");

  for (const auto& global : lua::runtimeLuaGlobals()) {
    if (global.function) {
      emitRuntimeFunction(out, *global.function, "");
    } else if (global.table) {
      appendLine(out, std::string(global.name) + " = " + global.name + " or {}");
      appendLine(out);
    } else {
      appendLine(out, std::string(global.name) + " = " + global.name);
      appendLine(out);
    }
  }

  for (const auto& table : lua::runtimeLuaTables())
    emitRuntimeTable(out, table);

  for (const auto& type : lua::runtimeLuaUserdataTypes())
    emitRuntimeTable(out, type);

  std::vector<lua::RuntimeLuaProxyFieldMetadata> engineFields{};
  lua::collectRuntimeLuaEngineParamProxyFields(engineFields);
  emitProxyTables(out, std::move(engineFields));

  std::vector<lua::RuntimeLuaProxyFieldMetadata> appFields{};
  lua::collectRuntimeLuaAppParamProxyFields(appFields);
  emitProxyTables(out, std::move(appFields));

  requireContains(out, "function applyFile", "applyFile");
  requireContains(out, "transport", "transport");
  requireContains(out, "seq", "seq");
  requireContains(out, "mixer", "mixer");
  requireNotContains(out, "apply_file", "apply_file");
  requireNotContains(out, "function TrackSettings", "TrackSettings");
  requireNotContains(out, "function SynthSettings", "SynthSettings");
  requireNotContains(out, "function MixerSettings", "MixerSettings");
  requireNotContains(out, "function track(", "document track");

  return {"runtime_lua/meh_groovebox_runtime.lua", out};
}

RenderedFile renderReadme() {
  std::string out;
  appendLine(out, "# Generated LuaLS Stubs");
  appendLine(out);
  appendLine(out, "Files in this directory are generated from C++ metadata.");
  appendLine(out);
  appendLine(out, "Do not edit them by hand.");
  appendLine(out);
  appendLine(out, "Update generated stubs:");
  appendLine(out);
  appendLine(out, "```sh");
  appendLine(out, "make luals-stubs");
  appendLine(out, "```");
  appendLine(out);
  appendLine(out, "Check generated stubs:");
  appendLine(out);
  appendLine(out, "```sh");
  appendLine(out, "make check-luals-stubs");
  appendLine(out, "```");
  appendLine(out);
  appendLine(out, "Run the full project gate:");
  appendLine(out);
  appendLine(out, "```sh");
  appendLine(out, "scripts/run_tests.sh");
  appendLine(out, "```");
  appendLine(out);
  appendLine(out, "The authored-document and runtime Lua stubs are separate LuaLS library roots.");
  return {"README.md", out};
}

std::vector<RenderedFile> renderAllFiles() {
  return {
      renderReadme(),
      renderAuthoredDocumentStub(),
      renderRuntimeLuaStub(),
  };
}

void writeRenderedFiles(const std::filesystem::path& outDir,
                        const std::vector<RenderedFile>& files) {
  for (const auto& file : files) {
    const auto path = outDir / file.relativePath;
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    if (!out)
      throw std::runtime_error("failed to open output file: " + path.string());
    out << file.contents;
  }
}

std::string readFile(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in)
    return {};
  std::ostringstream buffer;
  buffer << in.rdbuf();
  return buffer.str();
}

int checkRenderedFiles(const std::filesystem::path& outDir,
                       const std::vector<RenderedFile>& files) {
  int failures = 0;
  for (const auto& file : files) {
    const auto path = outDir / file.relativePath;
    if (!std::filesystem::exists(path)) {
      std::cerr << "missing generated file: " << path.string() << '\n';
      ++failures;
      continue;
    }
    if (readFile(path) != file.contents) {
      std::cerr << "stale generated file: " << path.string() << '\n';
      ++failures;
    }
  }
  return failures == 0 ? 0 : 1;
}

} // namespace

int main(int argc, char** argv) {
  Args args{};
  if (!parseArgs(argc, argv, args)) {
    std::cerr << usage();
    return 2;
  }

  try {
    const auto files = renderAllFiles();
    if (args.check)
      return checkRenderedFiles(args.outDir, files);
    writeRenderedFiles(args.outDir, files);
    return 0;
  } catch (const std::exception& err) {
    std::cerr << "generate_luals_stubs: " << err.what() << '\n';
    return 1;
  }
}
