#pragma once

#include <exception>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace mycpptools::test {

class Failure : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

[[noreturn]] inline void fail(std::string_view Expression,
                              std::string_view File, int Line) {
  throw Failure(std::string(File) + ":" + std::to_string(Line) +
                ": expectation failed: " + std::string(Expression));
}

template <typename ExpectedException, typename Callable>
void expectThrows(Callable &&Operation, std::string_view Expression,
                  std::string_view File, int Line) {
  try {
    std::forward<Callable>(Operation)();
  } catch (const ExpectedException &) {
    return;
  } catch (const std::exception &Error) {
    throw Failure(std::string(File) + ":" + std::to_string(Line) +
                  ": expected a different exception from " +
                  std::string(Expression) + ": " + Error.what());
  } catch (...) {
    throw Failure(std::string(File) + ":" + std::to_string(Line) +
                  ": expected a standard exception from " +
                  std::string(Expression));
  }

  fail(Expression, File, Line);
}

using TestFunction = void (*)();
using TestCase = std::pair<std::string_view, TestFunction>;

inline int runTests(std::initializer_list<TestCase> Tests) {
  for (const auto &[Name, Test] : Tests) {
    try {
      Test();
    } catch (const std::exception &Error) {
      std::cerr << Name << ": " << Error.what() << '\n';
      return 1;
    } catch (...) {
      std::cerr << Name << ": unknown exception\n";
      return 1;
    }
  }
  return 0;
}

} // namespace mycpptools::test

#define MYCPPTOOLS_EXPECT_TRUE(Expression)                                     \
  do {                                                                         \
    if (!(Expression))                                                         \
      ::mycpptools::test::fail(#Expression, __FILE__, __LINE__);               \
  } while (false)

#define MYCPPTOOLS_EXPECT_EQ(Left, Right)                                      \
  MYCPPTOOLS_EXPECT_TRUE((Left) == (Right))

#define MYCPPTOOLS_EXPECT_THROWS(Expression, Exception)                        \
  ::mycpptools::test::expectThrows<Exception>([&] { (void)(Expression); },     \
                                              #Expression, __FILE__, __LINE__)
