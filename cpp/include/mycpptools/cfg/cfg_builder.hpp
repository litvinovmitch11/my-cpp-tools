#pragma once

#include "mycpptools/ast/ast_graph.hpp"
#include "mycpptools/cfg/cfg_graph.hpp"

#include <optional>
#include <string>
#include <variant>

namespace mycpptools::cfg {

enum class CfgBuildErrorCode {
  InvalidAst,
  UnresolvedLabel,
  InternalInvariant,
};

struct CfgBuildError {
  CfgBuildErrorCode Code;
  source::FunctionId Function;
  std::optional<ast::AstNodeId> AstNode;
  std::string Message;
};

using FunctionCfgBuildResult = std::variant<FunctionCfgGraph, CfgBuildError>;
using FunctionCfgGraphsBuildResult =
    std::variant<FunctionCfgGraphs, CfgBuildError>;

[[nodiscard]] FunctionCfgBuildResult
buildFunctionCfgGraph(const ast::FunctionAstGraph &AstGraph);

// Builds the collection transactionally: either all graphs or one error.
[[nodiscard]] FunctionCfgGraphsBuildResult
buildFunctionCfgGraphs(const ast::FunctionAstGraphs &AstGraphs);

} // namespace mycpptools::cfg
