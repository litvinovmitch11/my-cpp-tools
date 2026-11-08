#include "mycpptools/cfg/cfg_graph.hpp"

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mycpptools::cfg {

[[nodiscard]] static bool isConditionEdge(CfgEdgeKind Kind) {
  switch (Kind) {
  case CfgEdgeKind::TrueBranch:
  case CfgEdgeKind::FalseBranch:
  case CfgEdgeKind::CaseBranch:
  case CfgEdgeKind::DefaultBranch:
  case CfgEdgeKind::ExceptionBranch:
  case CfgEdgeKind::AsmBranch:
  case CfgEdgeKind::Fallthrough:
  case CfgEdgeKind::Suspend:
    return true;
  case CfgEdgeKind::Flow:
  case CfgEdgeKind::IndirectBranch:
  case CfgEdgeKind::Resume:
    return false;
  }
  return false;
}

static void validateCfgGraph(const CfgGraph &Graph, CfgNodeId Exit) {
  if (Graph.empty())
    throw std::invalid_argument("function CFG must not be empty");

  const std::optional<CfgNodeId> Entry = Graph.root();
  if (!Entry)
    throw std::invalid_argument("function CFG must have an entry node");
  if (Graph.node(*Entry).Payload.Kind != CfgNodeKind::Entry)
    throw std::invalid_argument("function CFG root must be the entry node");
  if (Exit.value() >= Graph.nodeCount())
    throw std::invalid_argument("function CFG exit ID is out of range");
  if (Graph.node(Exit).Payload.Kind != CfgNodeKind::Exit)
    throw std::invalid_argument("function CFG exit ID must name the exit node");

  std::size_t EntryCount = 0;
  std::size_t ExitCount = 0;
  for (const CfgGraph::Node &Node : Graph.nodes()) {
    if (Node.Payload.Kind == CfgNodeKind::Entry)
      ++EntryCount;
    if (Node.Payload.Kind == CfgNodeKind::Exit)
      ++ExitCount;

    const bool IsTransfer = Node.Payload.Kind == CfgNodeKind::Transfer;
    if (IsTransfer != Node.Payload.Metadata.Transfer.has_value())
      throw std::invalid_argument(
          "function CFG transfer metadata must match the node kind");

    const auto &Outgoing = Graph.outgoingEdges(Node.Id);
    if (Node.Payload.Kind == CfgNodeKind::Entry && Outgoing.size() != 1)
      throw std::invalid_argument(
          "function CFG entry must have exactly one successor");
    if (Node.Payload.Kind == CfgNodeKind::Exit && !Outgoing.empty())
      throw std::invalid_argument("function CFG exit must have no successors");
    if (Node.Payload.Kind == CfgNodeKind::Condition && Outgoing.empty())
      throw std::invalid_argument(
          "function CFG condition must have at least one successor");
    if (Node.Payload.Kind == CfgNodeKind::Anchor && Outgoing.size() > 1)
      throw std::invalid_argument(
          "function CFG non-condition node has multiple successors");

    bool HasTrue = false;
    bool HasFalse = false;
    bool HasDefault = false;
    bool HasLinearSuccessor = false;
    bool HasExceptionalSuccessor = false;
    for (const CfgEdgeId EdgeId : Outgoing) {
      const CfgEdgeKind Kind = Graph.edge(EdgeId).Payload.Kind;
      if (Node.Payload.Kind == CfgNodeKind::Condition && !isConditionEdge(Kind))
        throw std::invalid_argument(
            "function CFG condition edges must name their branch");
      if (Node.Payload.Kind == CfgNodeKind::Anchor &&
          Kind != CfgEdgeKind::Flow && Kind != CfgEdgeKind::Resume)
        throw std::invalid_argument(
            "function CFG linear nodes must use flow or resume edges");
      if (Node.Payload.Kind == CfgNodeKind::Operation) {
        if (Kind != CfgEdgeKind::Flow && Kind != CfgEdgeKind::ExceptionBranch)
          throw std::invalid_argument(
              "function CFG operations must use flow or exception edges");
        if (Kind == CfgEdgeKind::Flow && HasLinearSuccessor)
          throw std::invalid_argument(
              "function CFG operation has multiple flow successors");
        if (Kind == CfgEdgeKind::ExceptionBranch && HasExceptionalSuccessor)
          throw std::invalid_argument(
              "function CFG operation has multiple exception successors");
        HasLinearSuccessor = HasLinearSuccessor || Kind == CfgEdgeKind::Flow;
        HasExceptionalSuccessor =
            HasExceptionalSuccessor || Kind == CfgEdgeKind::ExceptionBranch;
      }
      if (Kind == CfgEdgeKind::TrueBranch) {
        if (HasTrue)
          throw std::invalid_argument("duplicate true CFG branch");
        HasTrue = true;
      } else if (Kind == CfgEdgeKind::FalseBranch) {
        if (HasFalse)
          throw std::invalid_argument("duplicate false CFG branch");
        HasFalse = true;
      } else if (Kind == CfgEdgeKind::DefaultBranch) {
        if (HasDefault)
          throw std::invalid_argument("duplicate default CFG branch");
        HasDefault = true;
      }
    }

    if (Node.Payload.Kind == CfgNodeKind::Transfer) {
      if (Outgoing.empty())
        throw std::invalid_argument(
            "function CFG transfer must have a successor");
      const CfgTransferKind Transfer = *Node.Payload.Metadata.Transfer;
      for (const CfgEdgeId EdgeId : Outgoing) {
        const CfgEdgeKind Kind = Graph.edge(EdgeId).Payload.Kind;
        if (Transfer == CfgTransferKind::IndirectGoto) {
          if (Kind != CfgEdgeKind::IndirectBranch)
            throw std::invalid_argument(
                "indirect goto must use indirect CFG branches");
        } else if (Transfer == CfgTransferKind::Throw) {
          if (Outgoing.size() != 1 || Kind != CfgEdgeKind::ExceptionBranch)
            throw std::invalid_argument(
                "throw must have one exceptional CFG successor");
        } else if (Outgoing.size() != 1 || Kind != CfgEdgeKind::Flow) {
          throw std::invalid_argument(
              "function CFG transfer must have one flow successor");
        }
      }
    }
  }

  if (EntryCount != 1 || ExitCount != 1)
    throw std::invalid_argument(
        "function CFG must have exactly one entry and one exit");
  if (!Graph.incomingEdges(*Entry).empty())
    throw std::invalid_argument("function CFG entry must have no predecessors");
  if (Graph.edge(Graph.outgoingEdges(*Entry).front()).Payload.Kind !=
      CfgEdgeKind::Flow)
    throw std::invalid_argument("function CFG entry edge must be flow");
}

std::string_view toString(CfgNodeKind Kind) noexcept {
  switch (Kind) {
  case CfgNodeKind::Entry:
    return "entry";
  case CfgNodeKind::Exit:
    return "exit";
  case CfgNodeKind::Operation:
    return "operation";
  case CfgNodeKind::Condition:
    return "condition";
  case CfgNodeKind::Transfer:
    return "transfer";
  case CfgNodeKind::Anchor:
    return "anchor";
  }
  return "unknown CFG node";
}

std::string_view toString(CfgTransferKind Kind) noexcept {
  switch (Kind) {
  case CfgTransferKind::Return:
    return "return";
  case CfgTransferKind::Break:
    return "break";
  case CfgTransferKind::Continue:
    return "continue";
  case CfgTransferKind::Goto:
    return "goto";
  case CfgTransferKind::IndirectGoto:
    return "indirect goto";
  case CfgTransferKind::Throw:
    return "throw";
  case CfgTransferKind::CoReturn:
    return "co-return";
  }
  return "unknown CFG transfer";
}

std::string_view toString(CfgEdgeKind Kind) noexcept {
  switch (Kind) {
  case CfgEdgeKind::Flow:
    return "flow";
  case CfgEdgeKind::TrueBranch:
    return "true";
  case CfgEdgeKind::FalseBranch:
    return "false";
  case CfgEdgeKind::CaseBranch:
    return "case";
  case CfgEdgeKind::DefaultBranch:
    return "default";
  case CfgEdgeKind::ExceptionBranch:
    return "exception";
  case CfgEdgeKind::IndirectBranch:
    return "indirect";
  case CfgEdgeKind::AsmBranch:
    return "asm";
  case CfgEdgeKind::Fallthrough:
    return "fallthrough";
  case CfgEdgeKind::Suspend:
    return "suspend";
  case CfgEdgeKind::Resume:
    return "resume";
  }
  return "unknown CFG edge";
}

FunctionCfgGraph::FunctionCfgGraph(source::FunctionInfo Function,
                                   CfgGraph Graph, CfgNodeId Exit)
    : Function(std::move(Function)), Graph(std::move(Graph)), Exit(Exit) {
  validateCfgGraph(this->Graph, this->Exit);
}

const source::FunctionInfo &FunctionCfgGraph::function() const noexcept {
  return Function;
}

const CfgGraph &FunctionCfgGraph::graph() const noexcept { return Graph; }

CfgNodeId FunctionCfgGraph::entry() const noexcept { return *Graph.root(); }

CfgNodeId FunctionCfgGraph::exit() const noexcept { return Exit; }

std::vector<bool> computeReachableNodes(const FunctionCfgGraph &FunctionGraph) {
  const CfgGraph &Graph = FunctionGraph.graph();
  std::vector<bool> Reachable(Graph.nodeCount(), false);
  std::vector<CfgNodeId> Pending{FunctionGraph.entry()};
  while (!Pending.empty()) {
    const CfgNodeId Node = Pending.back();
    Pending.pop_back();
    if (Reachable[Node.value()])
      continue;
    Reachable[Node.value()] = true;
    for (const CfgEdgeId Edge : Graph.outgoingEdges(Node))
      Pending.push_back(Graph.edge(Edge).Target);
  }
  return Reachable;
}

} // namespace mycpptools::cfg
