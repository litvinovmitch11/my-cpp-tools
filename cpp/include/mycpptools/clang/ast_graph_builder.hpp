#pragma once

#include "mycpptools/graph/graph.hpp"

#include "clang/Tooling/Tooling.h"

#include <memory>
#include <vector>

using namespace ::clang::tooling;

namespace mycpptools::clang_frontend {

using FunctionAstGraphs = std::vector<mycpptools::FunctionAstGraph>;

std::unique_ptr<FrontendActionFactory>
createAstGraphActionFactory(FunctionAstGraphs &Output);

} // namespace mycpptools::clang_frontend
