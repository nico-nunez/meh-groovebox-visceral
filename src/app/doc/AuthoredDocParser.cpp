#include "app/doc/AuthoredDocParser.h"

#include "app/doc/parser/ParserHelpers.h"

#include "app/doc/DocAuthoredModel.h"
#include "app/doc/metadata/DocMetadata.h"

#include <string>

namespace app::doc {
namespace {

int l_plainTableConstructor(lua_State* L) {
  if (lua_istable(L, 1)) {
    lua_pushvalue(L, 1);
    return 1;
  }

  lua_newtable(L);
  return 1;
}

void openParserLibraries(lua_State* L) {
  luaL_requiref(L, "_G", luaopen_base, 1);
  lua_pop(L, 1);

  luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
  lua_pop(L, 1);

  luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
  lua_pop(L, 1);

  luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
  lua_pop(L, 1);
}

void registerPlainConstructor(lua_State* L, const char* name) {
  lua_pushcfunction(L, l_plainTableConstructor);
  lua_setglobal(L, name);
}

bool isAuthoredConstructor(const char* name) {
  for (const char* constructor : authoredDocumentConstructors()) {
    if (std::strcmp(constructor, name) == 0)
      return true;
  }
  return false;
}

// ===================
// Register
// ===================
void registerParserEnvironment(lua_State* L, LuaSequencerParseContext& ctx) {
  openParserLibraries(L);

  const DocFunctionMetadata* mixerFunction = findAuthoredDocumentFunction(docglobal::Mixer);
  if (mixerFunction && mixerFunction->status == DocMetadataStatus::Implemented) {
    lua_pushlightuserdata(L, &ctx);
    lua_pushcclosure(L, l_captureMixer, 1);
    lua_setglobal(L, mixerFunction->name);
  }

  const DocFunctionMetadata* trackFunction = findAuthoredDocumentFunction(docglobal::Track);
  if (trackFunction && trackFunction->status == DocMetadataStatus::Implemented) {
    lua_pushlightuserdata(L, &ctx);
    lua_pushcclosure(L, l_captureTrack, 1);
    lua_setglobal(L, trackFunction->name);
  }

  const DocFunctionMetadata* synthFunction = findAuthoredDocumentFunction(docglobal::Synth);
  if (synthFunction && synthFunction->status == DocMetadataStatus::Implemented) {
    lua_pushlightuserdata(L, &ctx);
    lua_pushcclosure(L, l_captureSynth, 1);
    lua_setglobal(L, synthFunction->name);
  }

  if (isAuthoredConstructor(docctor::TrackSettings))
    registerPlainConstructor(L, docctor::TrackSettings);
  if (isAuthoredConstructor(docctor::SynthSettings))
    registerPlainConstructor(L, docctor::SynthSettings);
  if (isAuthoredConstructor(docctor::MixerSettings))
    registerPlainConstructor(L, docctor::MixerSettings);
}

} // namespace

AuthoredDocNormalizeResult parseAndNormalizeAuthoredDoc(DocID documentID,
                                                        DocRevision revision,
                                                        const char* bufferText,
                                                        AuthoredDocModel* outModel) {
  AuthoredDocNormalizeResult result{};
  if (!outModel) {
    DocDiagnostic d{};
    d.severity = DiagnosticSeverity::Error;
    d.source = DiagnosticSource::Parser;
    d.documentID = documentID;
    d.revision = revision;
    d.code = docdiag::InternalPlannerError;
    d.message = "null authored document output";
    result.diagnostics.push_back(d);
    return result;
  }

  *outModel = AuthoredDocModel{};

  outModel->documentID = documentID;
  outModel->revision = revision;
  outModel->sequencer.documentID = documentID;
  outModel->sequencer.revision = revision;

  LuaSequencerParseContext ctx{};
  ctx.documentID = documentID;
  ctx.revision = revision;
  ctx.model = outModel;

  lua_State* L = luaL_newstate();
  if (!L) {
    pushDiagnostic(ctx,
                   DiagnosticSource::Parser,
                   docdiag::DocumentLuaStateFailed,
                   "failed to create authoring Lua state",
                   SourceSpan{},
                   "");
    result.diagnostics = ctx.diagnostics;
    return result;
  }

  registerParserEnvironment(L, ctx);

  const char* text = bufferText ? bufferText : "";
  if (luaL_dostring(L, text) != LUA_OK) {
    const char* message = lua_tostring(L, -1);
    pushDiagnostic(ctx,
                   DiagnosticSource::Parser,
                   docdiag::DocumentLuaEvalFailed,
                   message ? message : "failed to evaluate authored document",
                   SourceSpan{},
                   "");
    lua_pop(L, 1);
    lua_close(L);

    resetAuthoredDocModel(ctx.model, ctx.documentID, ctx.revision);
    result.diagnostics = ctx.diagnostics;
    return result;
  }

  lua_close(L);

  result.ok = ctx.diagnostics.empty();
  result.diagnostics = ctx.diagnostics;
  return result;
}

} // namespace app::doc
