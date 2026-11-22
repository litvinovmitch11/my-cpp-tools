#pragma once

#include "mycpptools/cfg/cfg_graph.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace mycpptools::cdg {

class PostDominatorRelation {
public:
  [[nodiscard]] std::size_t nodeCount() const noexcept;

  // Strong post-dominance: Dominator occurs on every maximal path beginning
  // at Node. Infinite paths and finite paths that do not reach the function
  // exit are included.
  [[nodiscard]] bool postDominates(cfg::CfgNodeId Dominator,
                                   cfg::CfgNodeId Node) const;

  // Returns a real CFG node only when the closest strict post-dominator is
  // unique. Non-terminating regions may have no immediate post-dominator.
  [[nodiscard]] std::optional<cfg::CfgNodeId>
  immediatePostDominator(cfg::CfgNodeId Node) const;

  // These are existential properties and do not change post-dominance.
  [[nodiscard]] bool canReachFunctionExit(cfg::CfgNodeId Node) const;
  [[nodiscard]] bool mayDiverge(cfg::CfgNodeId Node) const;

private:
  PostDominatorRelation(std::vector<std::vector<bool>> Relation,
                        std::vector<bool> CanReachExit,
                        std::vector<bool> MayDiverge);

  void validateNode(cfg::CfgNodeId Node) const;

  std::vector<std::vector<bool>> Relation;
  std::vector<bool> CanReachExit;
  std::vector<bool> MayDiverge;

  friend PostDominatorRelation
  computePostDominators(const cfg::FunctionCfgGraph &FunctionGraph);
};

[[nodiscard]] PostDominatorRelation
computePostDominators(const cfg::FunctionCfgGraph &FunctionGraph);

} // namespace mycpptools::cdg
