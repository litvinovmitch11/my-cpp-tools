#include "mycpptools/cdg/cdg_graph.hpp"

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mycpptools::cdg {

static bool
sameSourceRange(const std::optional<source::SourceRange> &Left,
                const std::optional<source::SourceRange> &Right) noexcept {
  if (Left.has_value() != Right.has_value())
    return false;
  return !Left || (Left->BeginOffset == Right->BeginOffset &&
                   Left->EndOffset == Right->EndOffset);
}

static void validateCdgGraph(const cfg::FunctionCfgGraph &CfgFunction,
                             const CdgGraph &Graph,
                             const std::vector<CdgNodeId> &NodesByCfg) {
  const cfg::CfgGraph &Cfg = CfgFunction.graph();
  if (Graph.empty())
    throw std::invalid_argument("function CDG must not be empty");
  if (Graph.nodeCount() != Cfg.nodeCount() ||
      NodesByCfg.size() != Cfg.nodeCount())
    throw std::invalid_argument(
        "function CDG must contain exactly one node per CFG node");

  const std::optional<CdgNodeId> Root = Graph.root();
  if (!Root)
    throw std::invalid_argument("function CDG must have an entry node");
  if (Graph.node(*Root).Payload.Kind != CdgNodeKind::Entry)
    throw std::invalid_argument("function CDG root must be the entry node");

  const std::vector<bool> Reachable = cfg::computeReachableNodes(CfgFunction);
  std::vector<bool> SeenCfgNodes(NodesByCfg.size(), false);
  std::size_t EntryCount = 0;
  std::size_t ExitCount = 0;
  for (const CdgGraph::Node &Node : Graph.nodes()) {
    const std::size_t Origin = Node.Payload.Metadata.CfgOrigin.value();
    if (Origin >= NodesByCfg.size())
      throw std::invalid_argument("function CDG CFG origin is out of range");
    if (SeenCfgNodes[Origin])
      throw std::invalid_argument("function CDG has duplicate CFG origins");
    if (NodesByCfg[Origin] != Node.Id)
      throw std::invalid_argument("function CDG CFG mapping is inconsistent");
    const cfg::CfgNode &CfgNode = Cfg.node(cfg::CfgNodeId(Origin)).Payload;
    if (Node.Payload.Kind != toCdgNodeKind(CfgNode.Kind))
      throw std::invalid_argument(
          "function CDG node kind must match its CFG origin");
    if (!sameSourceRange(Node.Payload.Metadata.Source,
                         CfgNode.Metadata.Source) ||
        Node.Payload.Metadata.ReachableFromEntry != Reachable[Origin] ||
        Node.Payload.Display.Label != CfgNode.Display.Label)
      throw std::invalid_argument(
          "function CDG node metadata must match its CFG origin");
    SeenCfgNodes[Origin] = true;

    if (Node.Payload.Kind == CdgNodeKind::Entry)
      ++EntryCount;
    if (Node.Payload.Kind == CdgNodeKind::Exit)
      ++ExitCount;
  }

  if (EntryCount != 1 || ExitCount != 1)
    throw std::invalid_argument(
        "function CDG must have exactly one entry and one exit node");
  if (*Root != NodesByCfg[CfgFunction.entry().value()])
    throw std::invalid_argument("function CDG entry mapping is inconsistent");

  for (const CdgGraph::Edge &Edge : Graph.edges()) {
    if (Edge.Payload.CfgOrigin.value() >= Cfg.edgeCount())
      throw std::invalid_argument(
          "function CDG CFG edge origin is out of range");
    const cfg::CfgGraph::Edge &CfgEdge = Cfg.edge(Edge.Payload.CfgOrigin);
    const cfg::CfgNodeId Controller =
        Graph.node(Edge.Source).Payload.Metadata.CfgOrigin;
    if (CfgEdge.Source != Controller)
      throw std::invalid_argument(
          "function CDG edge source must match its controlling CFG edge");
    if (Edge.Payload.ControllingBranch != CfgEdge.Payload.Kind ||
        Edge.Payload.Label != CfgEdge.Payload.Label)
      throw std::invalid_argument(
          "function CDG edge metadata must match its CFG origin");
    if (Edge.Payload.Kind != toCdgEdgeKind(CfgEdge.Payload.Kind))
      throw std::invalid_argument(
          "function CDG edge kind must match its controlling CFG branch");
  }
}

std::string_view toString(CdgNodeKind Kind) noexcept {
  switch (Kind) {
  case CdgNodeKind::Entry:
    return "entry";
  case CdgNodeKind::Exit:
    return "exit";
  case CdgNodeKind::Operation:
    return "operation";
  case CdgNodeKind::Predicate:
    return "predicate";
  case CdgNodeKind::Transfer:
    return "transfer";
  case CdgNodeKind::Anchor:
    return "anchor";
  }
  return "unknown CDG node";
}

std::string_view toString(CdgEdgeKind Kind) noexcept {
  switch (Kind) {
  case CdgEdgeKind::Control:
    return "control";
  case CdgEdgeKind::ExceptionalControl:
    return "exceptional control";
  }
  return "unknown CDG edge";
}

CdgNodeKind toCdgNodeKind(cfg::CfgNodeKind Kind) noexcept {
  switch (Kind) {
  case cfg::CfgNodeKind::Entry:
    return CdgNodeKind::Entry;
  case cfg::CfgNodeKind::Exit:
    return CdgNodeKind::Exit;
  case cfg::CfgNodeKind::Operation:
    return CdgNodeKind::Operation;
  case cfg::CfgNodeKind::Condition:
    return CdgNodeKind::Predicate;
  case cfg::CfgNodeKind::Transfer:
    return CdgNodeKind::Transfer;
  case cfg::CfgNodeKind::Anchor:
    return CdgNodeKind::Anchor;
  }
  return CdgNodeKind::Operation;
}

CdgEdgeKind toCdgEdgeKind(cfg::CfgEdgeKind Kind) noexcept {
  return Kind == cfg::CfgEdgeKind::ExceptionBranch
             ? CdgEdgeKind::ExceptionalControl
             : CdgEdgeKind::Control;
}

FunctionCdgGraph::FunctionCdgGraph(const cfg::FunctionCfgGraph &CfgFunction,
                                   CdgGraph Graph,
                                   std::vector<CdgNodeId> NodesByCfg)
    : Function(CfgFunction.function()), Graph(std::move(Graph)),
      NodesByCfg(std::move(NodesByCfg)) {
  validateCdgGraph(CfgFunction, this->Graph, this->NodesByCfg);
}

const source::FunctionInfo &FunctionCdgGraph::function() const noexcept {
  return Function;
}

const CdgGraph &FunctionCdgGraph::graph() const noexcept { return Graph; }

CdgNodeId FunctionCdgGraph::entry() const noexcept { return *Graph.root(); }

CdgNodeId FunctionCdgGraph::nodeForCfg(cfg::CfgNodeId Node) const {
  if (Node.value() >= NodesByCfg.size())
    throw std::out_of_range("CDG CFG node ID is out of range");
  return NodesByCfg[Node.value()];
}

} // namespace mycpptools::cdg
