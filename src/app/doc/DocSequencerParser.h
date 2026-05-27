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

struct AuthoredDocNormalizeResult {
  bool ok = false;
  DocDiagnostics diagnostics{};
};

AuthoredDocNormalizeResult parseAndNormalizeAuthoredDoc(DocID documentID,
                                                        DocRevision revision,
                                                        const char* bufferText,
                                                        AuthoredDocModel* outModel);
} // namespace app::doc
