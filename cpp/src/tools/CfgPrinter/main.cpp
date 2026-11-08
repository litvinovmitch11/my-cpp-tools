#include "mycpptools/cfg/cfg_builder.hpp"
#include "mycpptools/clang/ast_graph_builder.hpp"
#include "mycpptools/presentation/graph_dot_serializer.hpp"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

#include <iostream>
#include <string>
#include <variant>

namespace {

llvm::cl::OptionCategory CfgPrinterCategory("cfg-printer options");

} // namespace

int main(int argc, const char **argv) {
  auto ExpectedParser = clang::tooling::CommonOptionsParser::create(
      argc, argv, CfgPrinterCategory);
  if (!ExpectedParser) {
    llvm::errs() << llvm::toString(ExpectedParser.takeError());
    return 1;
  }

  clang::tooling::CommonOptionsParser &OptionsParser = ExpectedParser.get();
  clang::tooling::ClangTool Tool(OptionsParser.getCompilations(),
                                 OptionsParser.getSourcePathList());

  mycpptools::ast::FunctionAstGraphs AstGraphs;
  const int AstBuildResult =
      mycpptools::clang_frontend::buildFunctionAstGraphs(Tool, AstGraphs);
  if (AstBuildResult != 0)
    return AstBuildResult;

  mycpptools::cfg::FunctionCfgGraphsBuildResult BuildResult =
      mycpptools::cfg::buildFunctionCfgGraphs(AstGraphs);
  if (const auto *Error =
          std::get_if<mycpptools::cfg::CfgBuildError>(&BuildResult)) {
    std::string FunctionDescription = Error->Function.value();
    for (const mycpptools::ast::FunctionAstGraph &AstGraph : AstGraphs) {
      if (AstGraph.function().Id == Error->Function) {
        FunctionDescription = AstGraph.function().QualifiedName + " : " +
                              AstGraph.function().Signature;
        break;
      }
    }

    llvm::errs() << "failed to build CFG for " << FunctionDescription;
    if (Error->AstNode)
      llvm::errs() << " at AST node " << Error->AstNode->value();
    llvm::errs() << ": " << Error->Message << '\n';
    return 1;
  }

  const auto &CfgGraphs =
      std::get<mycpptools::cfg::FunctionCfgGraphs>(BuildResult);
  const mycpptools::presentation::GraphDotSerializer Serializer;
  const bool Serialized = Serializer.writeCfg(std::cout, CfgGraphs);
  std::cout.flush();
  if (!Serialized || !std::cout) {
    llvm::errs() << "failed to write CFG graph\n";
    return 1;
  }

  return 0;
}
