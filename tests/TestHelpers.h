#pragma once

#include "app/AppContext.h"
#include "app/GrooveboxEditSession.h"
#include "app/doc/DocAuthoringService.h"
#include "app/doc/DocDiagnostics.h"
#include "app/doc/DocSequencerModel.h"
#include "app/doc/DocSequencerParser.h"
#include "synth/WavetableBanks.h"

namespace test {

inline bool hasDiagnostic(const app::doc::DocDiagnostics& diagnostics, const char* code) {
  for (const auto& diagnostic : diagnostics) {
    if (diagnostic.code == code)
      return true;
  }
  return false;
}

struct ParseTestWorkspace {
  app::doc::AuthoredDocModel model{};
};

inline ParseTestWorkspace* getParseTestWorkspace() {
  static ParseTestWorkspace workspace{};
  workspace = ParseTestWorkspace{};
  return &workspace;
}

inline app::doc::AuthoredDocNormalizeResult parseWorkspace(app::doc::DocID documentID,
                                                           app::doc::DocRevision revision,
                                                           const char* bufferText,
                                                           ParseTestWorkspace* ws) {
  synth::wavetable::banks::initFactoryBanks();

  return app::doc::parseAndNormalizeAuthoredDoc(documentID, revision, bufferText, &ws->model);
}

inline app::doc::AuthoredDocNormalizeResult parseWS(const char* text, ParseTestWorkspace* ws) {
  return parseWorkspace(1, 7, text, ws);
}

inline void publishPending(app::AppContext* app) {
  auto blockInfo = app::transport::advanceTransportBlock(app->transport, app->transport.mode, 512);
  app::publishPendingGrooveboxEditIfReady(app, blockInfo);
}

} // namespace test
