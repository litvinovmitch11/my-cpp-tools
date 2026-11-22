#include "mycpptools/cdg/post_dominators.hpp"

#include <cstddef>
#include <deque>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mycpptools::cdg {

static std::vector<bool>
computeCanReachExit(const cfg::FunctionCfgGraph &Function) {
  const cfg::CfgGraph &Graph = Function.graph();
  std::vector<bool> Result(Graph.nodeCount(), false);
  std::vector<cfg::CfgNodeId> Pending{Function.exit()};
  while (!Pending.empty()) {
    const cfg::CfgNodeId Node = Pending.back();
    Pending.pop_back();
    if (Result[Node.value()])
      continue;
    Result[Node.value()] = true;
    for (const cfg::CfgEdgeId EdgeId : Graph.incomingEdges(Node))
      Pending.push_back(Graph.edge(EdgeId).Source);
  }
  return Result;
}

static std::vector<bool> computeMayDiverge(const cfg::CfgGraph &Graph) {
  std::vector<bool> Result(Graph.nodeCount(), true);
  std::vector<std::size_t> RemainingSuccessors(Graph.nodeCount(), 0);
  std::deque<cfg::CfgNodeId> Pending;
  for (const cfg::CfgGraph::Node &Node : Graph.nodes()) {
    RemainingSuccessors[Node.Id.value()] = Graph.outgoingEdges(Node.Id).size();
    if (RemainingSuccessors[Node.Id.value()] == 0)
      Pending.push_back(Node.Id);
  }

  while (!Pending.empty()) {
    const cfg::CfgNodeId Node = Pending.front();
    Pending.pop_front();
    if (!Result[Node.value()])
      continue;
    Result[Node.value()] = false;
    for (const cfg::CfgEdgeId EdgeId : Graph.incomingEdges(Node)) {
      const cfg::CfgNodeId Predecessor = Graph.edge(EdgeId).Source;
      if (!Result[Predecessor.value()])
        continue;
      --RemainingSuccessors[Predecessor.value()];
      if (RemainingSuccessors[Predecessor.value()] == 0)
        Pending.push_back(Predecessor);
    }
  }
  return Result;
}

static std::vector<std::vector<bool>>
computeRelation(const cfg::CfgGraph &Graph) {
  const std::size_t NodeCount = Graph.nodeCount();
  std::vector<std::vector<bool>> Relation(NodeCount,
                                          std::vector<bool>(NodeCount, false));

  for (std::size_t DominatorIndex = 0; DominatorIndex < NodeCount;
       ++DominatorIndex) {
    const cfg::CfgNodeId Dominator(DominatorIndex);
    std::vector<bool> MustReach(NodeCount, false);
    std::vector<std::size_t> RemainingSuccessors(NodeCount, 0);
    for (const cfg::CfgGraph::Node &Node : Graph.nodes())
      RemainingSuccessors[Node.Id.value()] =
          Graph.outgoingEdges(Node.Id).size();

    std::deque<cfg::CfgNodeId> Pending;
    MustReach[DominatorIndex] = true;
    Pending.push_back(Dominator);
    while (!Pending.empty()) {
      const cfg::CfgNodeId Reached = Pending.front();
      Pending.pop_front();
      for (const cfg::CfgEdgeId EdgeId : Graph.incomingEdges(Reached)) {
        const cfg::CfgNodeId Predecessor = Graph.edge(EdgeId).Source;
        if (MustReach[Predecessor.value()])
          continue;
        --RemainingSuccessors[Predecessor.value()];
        if (RemainingSuccessors[Predecessor.value()] == 0 &&
            !Graph.outgoingEdges(Predecessor).empty()) {
          MustReach[Predecessor.value()] = true;
          Pending.push_back(Predecessor);
        }
      }
    }

    for (std::size_t NodeIndex = 0; NodeIndex < NodeCount; ++NodeIndex)
      Relation[NodeIndex][DominatorIndex] = MustReach[NodeIndex];
  }
  return Relation;
}

PostDominatorRelation::PostDominatorRelation(
    std::vector<std::vector<bool>> Relation, std::vector<bool> CanReachExit,
    std::vector<bool> MayDiverge)
    : Relation(std::move(Relation)), CanReachExit(std::move(CanReachExit)),
      MayDiverge(std::move(MayDiverge)) {}

std::size_t PostDominatorRelation::nodeCount() const noexcept {
  return Relation.size();
}

bool PostDominatorRelation::postDominates(cfg::CfgNodeId Dominator,
                                          cfg::CfgNodeId Node) const {
  validateNode(Dominator);
  validateNode(Node);
  return Relation[Node.value()][Dominator.value()];
}

std::optional<cfg::CfgNodeId>
PostDominatorRelation::immediatePostDominator(cfg::CfgNodeId Node) const {
  validateNode(Node);
  std::optional<cfg::CfgNodeId> Candidate;
  for (std::size_t CandidateIndex = 0; CandidateIndex < Relation.size();
       ++CandidateIndex) {
    if (CandidateIndex == Node.value() ||
        !Relation[Node.value()][CandidateIndex])
      continue;

    bool Closest = true;
    for (std::size_t OtherIndex = 0; OtherIndex < Relation.size();
         ++OtherIndex) {
      if (OtherIndex == Node.value() || OtherIndex == CandidateIndex ||
          !Relation[Node.value()][OtherIndex])
        continue;
      if (!Relation[CandidateIndex][OtherIndex]) {
        Closest = false;
        break;
      }
    }
    if (!Closest)
      continue;
    if (Candidate)
      return std::nullopt;
    Candidate = cfg::CfgNodeId(CandidateIndex);
  }
  return Candidate;
}

bool PostDominatorRelation::canReachFunctionExit(cfg::CfgNodeId Node) const {
  validateNode(Node);
  return CanReachExit[Node.value()];
}

bool PostDominatorRelation::mayDiverge(cfg::CfgNodeId Node) const {
  validateNode(Node);
  return MayDiverge[Node.value()];
}

void PostDominatorRelation::validateNode(cfg::CfgNodeId Node) const {
  if (Node.value() >= Relation.size())
    throw std::out_of_range("post-dominator CFG node ID is out of range");
}

PostDominatorRelation
computePostDominators(const cfg::FunctionCfgGraph &FunctionGraph) {
  const cfg::CfgGraph &Graph = FunctionGraph.graph();
  std::vector<std::vector<bool>> Relation = computeRelation(Graph);
  return PostDominatorRelation(std::move(Relation),
                               computeCanReachExit(FunctionGraph),
                               computeMayDiverge(Graph));
}

} // namespace mycpptools::cdg
