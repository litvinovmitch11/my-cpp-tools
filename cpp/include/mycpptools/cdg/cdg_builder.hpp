#pragma once

#include "mycpptools/cdg/cdg_graph.hpp"
#include "mycpptools/cfg/cfg_graph.hpp"

namespace mycpptools::cdg {

// Builds strong control dependence from the project-owned CFG. Every valid
// FunctionCfgGraph produces a FunctionCdgGraph.
[[nodiscard]] FunctionCdgGraph
buildFunctionCdgGraph(const cfg::FunctionCfgGraph &CfgGraph);

[[nodiscard]] FunctionCdgGraphs
buildFunctionCdgGraphs(const cfg::FunctionCfgGraphs &CfgGraphs);

} // namespace mycpptools::cdg
