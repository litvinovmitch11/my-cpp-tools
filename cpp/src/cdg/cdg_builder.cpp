#include "mycpptools/cdg/cdg_builder.hpp"

#include "mycpptools/cdg/post_dominators.hpp"

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace mycpptools::cdg {

FunctionCdgGraph
buildFunctionCdgGraph(const cfg::FunctionCfgGraph &CfgFunction) {
  const cfg::CfgGraph &Cfg = CfgFunction.graph();
  const std::vector<bool> Reachable = cfg::computeReachableNodes(CfgFunction);
  CdgGraph Cdg;
  std::vector<CdgNodeId> NodesByCfg;
  NodesByCfg.reserve(Cfg.nodeCount());
  for (const cfg::CfgGraph::Node &Node : Cfg.nodes()) {
    NodesByCfg.push_back(Cdg.addNode(CdgNode{
        toCdgNodeKind(Node.Payload.Kind),
        CdgNodeMetadata{Node.Id, Node.Payload.Metadata.Source,
                        Reachable[Node.Id.value()]},
        CdgNodeDisplay{Node.Payload.Display.Label},
    }));
  }
  Cdg.setRoot(NodesByCfg[CfgFunction.entry().value()]);

  const PostDominatorRelation PostDominators =
      computePostDominators(CfgFunction);
  for (const cfg::CfgGraph::Edge &Branch : Cfg.edges()) {
    if (PostDominators.postDominates(Branch.Target, Branch.Source))
      continue;

    for (std::size_t DependentIndex = 0; DependentIndex < Cfg.nodeCount();
         ++DependentIndex) {
      const cfg::CfgNodeId Dependent(DependentIndex);
      if (!PostDominators.postDominates(Dependent, Branch.Target))
        continue;
      if (Dependent != Branch.Source &&
          PostDominators.postDominates(Dependent, Branch.Source))
        continue;

      (void)Cdg.addEdge(NodesByCfg[Branch.Source.value()],
                        NodesByCfg[DependentIndex],
                        CdgEdge{toCdgEdgeKind(Branch.Payload.Kind), Branch.Id,
                                Branch.Payload.Kind, Branch.Payload.Label});
    }
  }

  return FunctionCdgGraph(CfgFunction, std::move(Cdg), std::move(NodesByCfg));
}

FunctionCdgGraphs
buildFunctionCdgGraphs(const cfg::FunctionCfgGraphs &CfgGraphs) {
  FunctionCdgGraphs Result;
  Result.reserve(CfgGraphs.size());
  for (const cfg::FunctionCfgGraph &CfgGraph : CfgGraphs)
    Result.push_back(buildFunctionCdgGraph(CfgGraph));
  return Result;
}

} // namespace mycpptools::cdg
