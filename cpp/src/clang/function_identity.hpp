#pragma once

#include "mycpptools/source/function_info.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

namespace mycpptools::clang_frontend {

inline source::FunctionId
makeFunctionId(std::string Usr, std::string_view QualifiedName,
               std::string_view FileName,
               std::optional<std::size_t> ExpansionOffset) {
  if (!Usr.empty())
    return source::FunctionId(std::move(Usr));

  if (QualifiedName.empty())
    QualifiedName = "<anonymous>";
  if (FileName.empty())
    FileName = "<unknown-file>";

  std::string Fallback = "fallback:";
  Fallback += std::to_string(QualifiedName.size());
  Fallback += ':';
  Fallback += QualifiedName;
  Fallback += ':';
  Fallback += std::to_string(FileName.size());
  Fallback += ':';
  Fallback += FileName;
  Fallback += ':';
  Fallback += ExpansionOffset ? std::to_string(*ExpansionOffset) : "unknown";
  return source::FunctionId(std::move(Fallback));
}

} // namespace mycpptools::clang_frontend
