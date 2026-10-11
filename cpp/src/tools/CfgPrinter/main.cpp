#include "mycpptools/clang/ast_graph_builder.hpp"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include <iostream>

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

  mycpptools::FunctionAstGraphs Graphs;
  if (buildFunctionAstGraphs(Tool, Graphs)) {
    return 1;
  }

  for (const auto &Graph : Graphs) {
    // print func graph
    std::cout << Graph.getName() << std::endl;
  }
}
