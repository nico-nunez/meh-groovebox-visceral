#pragma once

#include "app/AppContext.h"
#include "app/GrooveboxEditSession.h"
#include "app/doc/AuthoredDocParser.h"
#include "app/doc/DocAuthoringService.h"
#include "app/doc/DocDiagnostics.h"
#include "app/sessions/AudioSession.h"
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

inline app::AppContext* makeAppContext(uint32_t sampleRate = 48000,
                                       uint32_t bufferFrameSize = 64,
                                       uint16_t numChannels = 2) {
  app::audio::DeviceInfo device{};
  device.sampleRate = sampleRate;
  device.bufferFrameSize = bufferFrameSize;
  device.numChannels = numChannels;
  return app::createAppContext(device);
}

inline void destroyAppContext(app::AppContext* app) {
  app::destroyAppContext(app);
}

} // namespace test
