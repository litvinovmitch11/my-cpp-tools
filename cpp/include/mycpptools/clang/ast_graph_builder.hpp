#pragma once

#include "mycpptools/ast/ast_graph.hpp"

#include "clang/Tooling/Tooling.h"

namespace mycpptools::clang_frontend {

int buildFunctionAstGraphs(::clang::tooling::ClangTool &Tool,
                           mycpptools::FunctionAstGraphs &Output);

} // namespace mycpptools::clang_frontend
