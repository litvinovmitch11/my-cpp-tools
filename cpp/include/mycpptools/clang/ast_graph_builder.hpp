#pragma once

#include "mycpptools/graph/graph.hpp"

#include "clang/Tooling/Tooling.h"

#include <vector>

namespace mycpptools::clang_frontend {

using FunctionAstGraphs = std::vector<mycpptools::FunctionAstGraph>;

int buildAstGraphs(::clang::tooling::ClangTool &Tool,
                   FunctionAstGraphs &Output);

} // namespace mycpptools::clang_frontend
