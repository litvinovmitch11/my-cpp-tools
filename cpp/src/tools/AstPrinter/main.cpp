#include "mycpptools/clang/ast_graph_builder.hpp"
#include "mycpptools/presentation/graph_dot_serializer.hpp"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

#include <iostream>

namespace {

llvm::cl::OptionCategory AstPrinterCategory("ast-printer options");

} // namespace

int main(int argc, const char **argv) {
  auto ExpectedParser = clang::tooling::CommonOptionsParser::create(
      argc, argv, AstPrinterCategory);
  if (!ExpectedParser) {
    llvm::errs() << llvm::toString(ExpectedParser.takeError());
    return 1;
  }

  clang::tooling::CommonOptionsParser &OptionsParser = ExpectedParser.get();
  clang::tooling::ClangTool Tool(OptionsParser.getCompilations(),
                                 OptionsParser.getSourcePathList());

  mycpptools::ast::FunctionAstGraphs FunctionGraphs;
  const int BuildResult =
      mycpptools::clang_frontend::buildFunctionAstGraphs(Tool, FunctionGraphs);
  if (BuildResult != 0)
    return BuildResult;

  const mycpptools::presentation::GraphDotSerializer Serializer;
  const bool Serialized = Serializer.writeAst(std::cout, FunctionGraphs);
  std::cout.flush();
  if (!Serialized || !std::cout) {
    llvm::errs() << "failed to write AST graph\n";
    return 1;
  }

  return 0;
}
