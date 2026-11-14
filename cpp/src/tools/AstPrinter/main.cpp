#include "mycpptools/clang/ast_graph_builder.hpp"
#include "mycpptools/presentation/graph_dot_serializer.hpp"
#include "mycpptools/tool/exit_code.hpp"

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
    return mycpptools::tool::toInt(mycpptools::tool::ExitCode::InternalError);
  }

  clang::tooling::CommonOptionsParser &OptionsParser = ExpectedParser.get();
  clang::tooling::ClangTool Tool(OptionsParser.getCompilations(),
                                 OptionsParser.getSourcePathList());

  mycpptools::ast::FunctionAstGraphs FunctionGraphs;
  const mycpptools::clang_frontend::AstGraphBuildStatus BuildStatus =
      mycpptools::clang_frontend::buildFunctionAstGraphs(Tool, FunctionGraphs);
  if (BuildStatus ==
      mycpptools::clang_frontend::AstGraphBuildStatus::InvalidSource)
    return mycpptools::tool::toInt(mycpptools::tool::ExitCode::InvalidSource);
  if (BuildStatus != mycpptools::clang_frontend::AstGraphBuildStatus::Success)
    return mycpptools::tool::toInt(mycpptools::tool::ExitCode::InternalError);

  const mycpptools::presentation::GraphDotSerializer Serializer;
  const bool Serialized = Serializer.writeAst(std::cout, FunctionGraphs);
  std::cout.flush();
  if (!Serialized || !std::cout) {
    llvm::errs() << "failed to write AST graph\n";
    return mycpptools::tool::toInt(mycpptools::tool::ExitCode::InternalError);
  }

  return mycpptools::tool::toInt(mycpptools::tool::ExitCode::Success);
}
