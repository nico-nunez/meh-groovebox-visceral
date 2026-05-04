#pragma once

#include <cstdint>

namespace app::doc {

using DocID = uint32_t;
using DocRevision = uint32_t;
using ApplyOperationID = uint32_t;

enum class ApplyStatus : uint8_t {
  Idle,
  Validated,
  Planned,
  Admitted,
  Started,
  Completed,
  Failed,
  Superseded,
};

enum class ActivePatternSlotSource : uint8_t {
  Unset,
  Explicit,
  Inferred,
};

struct SourceSpan {
  uint32_t line = 0;
  uint32_t column = 0;
  uint32_t endLine = 0;
  uint32_t endColumn = 0;
};

} // namespace app::doc
