#pragma once

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

inline app::doc::AuthoredDocumentNormalizeResult parseDocument(app::doc::DocID documentID,
                                                               app::doc::DocRevision revision,
                                                               const char* bufferText) {
  synth::wavetable::banks::initFactoryBanks();
  static app::doc::PatternArena scratchArena{};
  return app::doc::parseAndNormalizeAuthoredDocument(documentID,
                                                     revision,
                                                     bufferText,
                                                     &scratchArena);
}

inline app::doc::AuthoredDocumentNormalizeResult parseDoc(const char* text) {
  return parseDocument(1, 7, text);
}

} // namespace test
