#pragma once

#include "mycpptools/ast/ast_graph.hpp"

namespace clang::tooling {

class ClangTool;

} // namespace clang::tooling

namespace mycpptools::clang_frontend {

enum class AstGraphBuildStatus {
  Success,
  InvalidSource,
  InternalError,
};

// Builds graphs transactionally. On success, Output is replaced with all
// graphs collected by Tool. On failure, Output remains unchanged.
AstGraphBuildStatus buildFunctionAstGraphs(clang::tooling::ClangTool &Tool,
                                           ast::FunctionAstGraphs &Output);

} // namespace mycpptools::clang_frontend
