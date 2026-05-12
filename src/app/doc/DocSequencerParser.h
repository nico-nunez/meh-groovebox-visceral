#pragma once

#include "app/doc/DocAuthoredModel.h"
#include "app/doc/DocDiagnostics.h"
#include "app/doc/DocSequencerModel.h"

namespace app::doc {

struct SequencerNormalizeResult {
  bool ok = false;
  AuthoredSeqDocModel model{};
  DocDiagnostics diagnostics{};
};

struct AuthoredDocumentNormalizeResult {
  bool ok = false;
  AuthoredDocModel model{};
  DocDiagnostics diagnostics{};
};

AuthoredDocumentNormalizeResult parseAndNormalizeAuthoredDocument(DocID documentID,
                                                                  DocRevision revision,
                                                                  const char* bufferText,
                                                                  PatternArena* scratchArena);

} // namespace app::doc
