#pragma once

#include "mycpptools/ast/ast_graph.hpp"

namespace clang::tooling {

class ClangTool;

} // namespace clang::tooling

namespace mycpptools::clang_frontend {

// Builds graphs transactionally. On success, Output is replaced with all
// graphs collected by Tool. On failure, Output remains unchanged.
int buildFunctionAstGraphs(clang::tooling::ClangTool &Tool,
                           ast::FunctionAstGraphs &Output);

} // namespace mycpptools::clang_frontend
