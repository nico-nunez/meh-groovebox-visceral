#pragma once

#include "app/doc/DocTypes.h"

#include <string>
#include <vector>

namespace app::doc {

enum class DiagnosticSeverity : uint8_t { Error, Warning, Info };
enum class DiagnosticSource : uint8_t {
  Parser,
  Validator,
  Normalizer,
  Planner,
  GrooveboxAdmission,
};

struct DocDiagnostic {
  DiagnosticSeverity severity = DiagnosticSeverity::Error;
  DiagnosticSource source = DiagnosticSource::Parser;
  DocID documentID = 0;
  DocRevision revision = 0;
  std::string code{};
  std::string message{};
  SourceSpan span{};
  std::string relatedTarget{};
};

using DocDiagnostics = std::vector<DocDiagnostic>;

} // namespace app::doc
