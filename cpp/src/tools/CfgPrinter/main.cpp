#include "mycpptools/clang/ast_graph_builder.hpp"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include <iostream>
#include <memory>

using namespace mycpptools::clang_frontend;
using namespace clang;

namespace {

llvm::cl::OptionCategory CfgPrinterCategory("cfg-printer options");

} // namespace

int main(int argc, const char **argv) {
  auto ExpectedParser =
      tooling::CommonOptionsParser::create(argc, argv, CfgPrinterCategory);
  if (!ExpectedParser) {
    llvm::errs() << llvm::toString(ExpectedParser.takeError());
    return 1;
  }
  tooling::CommonOptionsParser &OptionsParser = ExpectedParser.get();
  tooling::ClangTool Tool(OptionsParser.getCompilations(),
                          OptionsParser.getSourcePathList());

  FunctionAstGraphs Functions;
  if (Tool.run(createAstGraphActionFactory(Functions).get())) {
    return 1;
  }

  for (const auto &Function : Functions) {
    // print func graph
    std::cout << Function.getName() << std::endl;
  }
}
