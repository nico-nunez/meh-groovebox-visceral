#pragma once

#include "app/doc/DocAuthoredModel.h"
#include "app/doc/DocDiagnostics.h"

namespace app::doc {

struct AuthoredDocNormalizeResult {
  bool ok = false;
  DocDiagnostics diagnostics{};
};

AuthoredDocNormalizeResult parseAndNormalizeAuthoredDoc(DocID documentID,
                                                        DocRevision revision,
                                                        const char* bufferText,
                                                        AuthoredDocModel* outModel);
} // namespace app::doc
