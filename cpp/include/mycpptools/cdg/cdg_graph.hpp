#pragma once

#include "mycpptools/cfg/cfg_graph.hpp"
#include "mycpptools/graph/directed_graph.hpp"
#include "mycpptools/source/function_info.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mycpptools::cdg {

enum class CdgNodeKind {
  Entry,
  Exit,
  Operation,
  Predicate,
  Transfer,
  Anchor,
};

enum class CdgEdgeKind {
  Control,
  ExceptionalControl,
};

[[nodiscard]] std::string_view toString(CdgNodeKind Kind) noexcept;
[[nodiscard]] std::string_view toString(CdgEdgeKind Kind) noexcept;
[[nodiscard]] CdgNodeKind toCdgNodeKind(cfg::CfgNodeKind Kind) noexcept;
[[nodiscard]] CdgEdgeKind toCdgEdgeKind(cfg::CfgEdgeKind Kind) noexcept;

struct CdgNodeMetadata {
  cfg::CfgNodeId CfgOrigin;
  std::optional<source::SourceRange> Source;
  bool ReachableFromEntry = false;
};

struct CdgNodeDisplay {
  std::string Label;
};

struct CdgNode {
  CdgNodeKind Kind;
  CdgNodeMetadata Metadata;
  CdgNodeDisplay Display;
};

struct CdgEdge {
  CdgEdgeKind Kind;
  cfg::CfgEdgeId CfgOrigin;
  cfg::CfgEdgeKind ControllingBranch;
  std::optional<std::string> Label;
};

using CdgGraph = graph::DirectedGraph<CdgNode, CdgEdge>;
using CdgNodeId = CdgGraph::NodeId;
using CdgEdgeId = CdgGraph::EdgeId;

class FunctionCdgGraph {
public:
  // Throws std::invalid_argument unless Graph contains exactly one node for
  // every CFG node named by NodesByCfg, with the entry node as its root.
  FunctionCdgGraph(const cfg::FunctionCfgGraph &CfgFunction, CdgGraph Graph,
                   std::vector<CdgNodeId> NodesByCfg);

  [[nodiscard]] const source::FunctionInfo &function() const noexcept;
  [[nodiscard]] const CdgGraph &graph() const noexcept;
  [[nodiscard]] CdgNodeId entry() const noexcept;
  [[nodiscard]] CdgNodeId nodeForCfg(cfg::CfgNodeId Node) const;

private:
  source::FunctionInfo Function;
  CdgGraph Graph;
  std::vector<CdgNodeId> NodesByCfg;
};

using FunctionCdgGraphs = std::vector<FunctionCdgGraph>;

} // namespace mycpptools::cdg
