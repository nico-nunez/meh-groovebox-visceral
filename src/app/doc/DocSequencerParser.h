#pragma once

#include "app/doc/DocDiagnostics.h"
#include "app/doc/DocSequencerModel.h"

namespace app::doc {

struct SequencerNormalizeResult {
  bool ok = false;
  AuthoredSeqDocModel model{};
  DocDiagnostics diagnostics{};
};

SequencerNormalizeResult parseAndNormalizeSequencerDocument(DocID documentID,
                                                            DocRevision revision,
                                                            const char* bufferText);

} // namespace app::doc
