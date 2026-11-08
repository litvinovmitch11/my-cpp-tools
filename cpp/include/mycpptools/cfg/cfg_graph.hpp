#pragma once

#include "mycpptools/ast/ast_graph.hpp"
#include "mycpptools/graph/directed_graph.hpp"
#include "mycpptools/source/function_info.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mycpptools::cfg {

enum class CfgNodeKind {
  Entry,
  Exit,
  Operation,
  Condition,
  Transfer,
  Anchor,
};

enum class CfgTransferKind {
  Return,
  Break,
  Continue,
  Goto,
  IndirectGoto,
  Throw,
  CoReturn,
};

enum class CfgEdgeKind {
  Flow,
  TrueBranch,
  FalseBranch,
  CaseBranch,
  DefaultBranch,
  ExceptionBranch,
  IndirectBranch,
  AsmBranch,
  Fallthrough,
  Suspend,
  Resume,
};

[[nodiscard]] std::string_view toString(CfgNodeKind Kind) noexcept;
[[nodiscard]] std::string_view toString(CfgTransferKind Kind) noexcept;
[[nodiscard]] std::string_view toString(CfgEdgeKind Kind) noexcept;

struct CfgNodeMetadata {
  std::optional<ast::AstNodeId> AstOrigin;
  std::optional<CfgTransferKind> Transfer;
  std::optional<source::SourceRange> Source;
};

struct CfgNodeDisplay {
  std::string Label;
};

struct CfgNode {
  CfgNodeKind Kind;
  CfgNodeMetadata Metadata;
  CfgNodeDisplay Display;
};

struct CfgEdge {
  CfgEdgeKind Kind;
  std::optional<ast::AstNodeId> AstOrigin;
  std::optional<std::string> Label;
};

using CfgGraph = graph::DirectedGraph<CfgNode, CfgEdge>;
using CfgNodeId = CfgGraph::NodeId;
using CfgEdgeId = CfgGraph::EdgeId;

class FunctionCfgGraph {
public:
  // Throws std::invalid_argument unless Graph satisfies the CFG invariants.
  FunctionCfgGraph(source::FunctionInfo Function, CfgGraph Graph,
                   CfgNodeId Exit);

  [[nodiscard]] const source::FunctionInfo &function() const noexcept;
  [[nodiscard]] const CfgGraph &graph() const noexcept;
  [[nodiscard]] CfgNodeId entry() const noexcept;
  [[nodiscard]] CfgNodeId exit() const noexcept;

private:
  source::FunctionInfo Function;
  CfgGraph Graph;
  CfgNodeId Exit;
};

using FunctionCfgGraphs = std::vector<FunctionCfgGraph>;

[[nodiscard]] std::vector<bool>
computeReachableNodes(const FunctionCfgGraph &FunctionGraph);

} // namespace mycpptools::cfg
