#include "mycpptools/cdg/cdg_builder.hpp"
#include "mycpptools/cfg/cfg_builder.hpp"
#include "mycpptools/clang/ast_graph_builder.hpp"
#include "mycpptools/presentation/graph_dot_serializer.hpp"
#include "mycpptools/tool/exit_code.hpp"

#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/raw_ostream.h"

#include <iostream>
#include <string>
#include <variant>

namespace {

llvm::cl::OptionCategory CdgPrinterCategory("cdg-printer options");

} // namespace

int main(int argc, const char **argv) {
  auto ExpectedParser = clang::tooling::CommonOptionsParser::create(
      argc, argv, CdgPrinterCategory);
  if (!ExpectedParser) {
    llvm::errs() << llvm::toString(ExpectedParser.takeError());
    return mycpptools::tool::toInt(mycpptools::tool::ExitCode::InternalError);
  }

  clang::tooling::CommonOptionsParser &OptionsParser = ExpectedParser.get();
  clang::tooling::ClangTool Tool(OptionsParser.getCompilations(),
                                 OptionsParser.getSourcePathList());

  mycpptools::ast::FunctionAstGraphs AstGraphs;
  const mycpptools::clang_frontend::AstGraphBuildStatus AstBuildStatus =
      mycpptools::clang_frontend::buildFunctionAstGraphs(Tool, AstGraphs);
  if (AstBuildStatus ==
      mycpptools::clang_frontend::AstGraphBuildStatus::InvalidSource)
    return mycpptools::tool::toInt(mycpptools::tool::ExitCode::InvalidSource);
  if (AstBuildStatus !=
      mycpptools::clang_frontend::AstGraphBuildStatus::Success)
    return mycpptools::tool::toInt(mycpptools::tool::ExitCode::InternalError);

  mycpptools::cfg::FunctionCfgGraphsBuildResult CfgBuildResult =
      mycpptools::cfg::buildFunctionCfgGraphs(AstGraphs);
  if (const auto *Error =
          std::get_if<mycpptools::cfg::CfgBuildError>(&CfgBuildResult)) {
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
    return mycpptools::tool::toInt(mycpptools::tool::ExitCode::InternalError);
  }

  const auto &CfgGraphs =
      std::get<mycpptools::cfg::FunctionCfgGraphs>(CfgBuildResult);
  const mycpptools::cdg::FunctionCdgGraphs CdgGraphs =
      mycpptools::cdg::buildFunctionCdgGraphs(CfgGraphs);
  const mycpptools::presentation::GraphDotSerializer Serializer;
  const bool Serialized = Serializer.writeCdg(std::cout, CdgGraphs);
  std::cout.flush();
  if (!Serialized || !std::cout) {
    llvm::errs() << "failed to write CDG graph\n";
    return mycpptools::tool::toInt(mycpptools::tool::ExitCode::InternalError);
  }

  return mycpptools::tool::toInt(mycpptools::tool::ExitCode::Success);
}
