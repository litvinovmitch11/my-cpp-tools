#include "mycpptools/source/function_info.hpp"

#include <stdexcept>
#include <utility>

namespace mycpptools::source {

FunctionId::FunctionId(std::string Value) : Value(std::move(Value)) {
  if (this->Value.empty())
    throw std::invalid_argument("function ID must not be empty");
}

const std::string &FunctionId::value() const noexcept { return Value; }

bool operator==(const FunctionId &Left, const FunctionId &Right) noexcept {
  return Left.Value == Right.Value;
}

bool operator!=(const FunctionId &Left, const FunctionId &Right) noexcept {
  return !(Left == Right);
}

bool operator<(const FunctionId &Left, const FunctionId &Right) noexcept {
  return Left.Value < Right.Value;
}

} // namespace mycpptools::source
