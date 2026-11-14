#pragma once

namespace mycpptools::tool {

enum class ExitCode {
  Success = 0,
  InternalError = 1,
  InvalidSource = 2,
};

[[nodiscard]] constexpr int toInt(ExitCode Code) noexcept {
  return static_cast<int>(Code);
}

} // namespace mycpptools::tool
