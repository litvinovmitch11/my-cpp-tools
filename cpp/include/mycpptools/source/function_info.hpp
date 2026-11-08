#pragma once

#include "mycpptools/source/source_range.hpp"

#include <optional>
#include <string>

namespace mycpptools::source {

class FunctionId {
public:
  // Throws std::invalid_argument when Value is empty.
  explicit FunctionId(std::string Value);

  [[nodiscard]] const std::string &value() const noexcept;

  friend bool operator==(const FunctionId &Left,
                         const FunctionId &Right) noexcept;
  friend bool operator!=(const FunctionId &Left,
                         const FunctionId &Right) noexcept;
  friend bool operator<(const FunctionId &Left,
                        const FunctionId &Right) noexcept;

private:
  std::string Value;
};

struct FunctionInfo {
  FunctionId Id;
  std::string Name;
  std::string QualifiedName;
  std::string Signature;
  std::optional<SourceRange> Definition;
};

} // namespace mycpptools::source
