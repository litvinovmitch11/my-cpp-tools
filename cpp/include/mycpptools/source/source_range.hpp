#pragma once

#include <cstddef>

namespace mycpptools::source {

struct SourceRange {
  // Half-open byte offsets into the main source file: [BeginOffset, EndOffset).
  std::size_t BeginOffset;
  std::size_t EndOffset;
};

} // namespace mycpptools::source
