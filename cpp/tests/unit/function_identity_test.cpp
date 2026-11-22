#include "function_identity.hpp"

#include "support/test_support.hpp"

#include <optional>
#include <string>

using mycpptools::clang_frontend::makeFunctionId;

static void preservesClangUsr() {
  const auto Id = makeFunctionId("c:@F@run#I#", "run", "first.cpp", 17U);

  MYCPPTOOLS_EXPECT_EQ(Id.value(), "c:@F@run#I#");
}

static void fallbackIncludesFileAndExpansionOffset() {
  const auto First = makeFunctionId("", "run", "first.cpp", 17U);
  const auto Second = makeFunctionId("", "run", "second.cpp", 17U);

  MYCPPTOOLS_EXPECT_EQ(First.value(), "fallback:3:run:9:first.cpp:17");
  MYCPPTOOLS_EXPECT_TRUE(First != Second);
}

static void fallbackHandlesMissingLocation() {
  const auto Id = makeFunctionId("", "", "", std::nullopt);

  MYCPPTOOLS_EXPECT_EQ(Id.value(),
                       "fallback:11:<anonymous>:14:<unknown-file>:unknown");
}

int main() {
  return mycpptools::test::runTests({
      {"preserves a Clang USR", preservesClangUsr},
      {"fallback includes file and expansion offset",
       fallbackIncludesFileAndExpansionOffset},
      {"fallback handles a missing location", fallbackHandlesMissingLocation},
  });
}
