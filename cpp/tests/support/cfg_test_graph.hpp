#pragma once

#include "mycpptools/cfg/cfg_graph.hpp"
#include "mycpptools/source/function_info.hpp"

#include <optional>
#include <string>
#include <utility>

namespace mycpptools::test {

inline source::FunctionInfo makeTestFunction(std::string Id = "function-id") {
  return source::FunctionInfo{source::FunctionId(std::move(Id)), "run",
                              "sample::run", "void ()", std::nullopt};
}

class CfgTestGraph {
public:
  CfgTestGraph() {
    Entry = addNode(cfg::CfgNodeKind::Entry, "Entry");
    Exit = addNode(cfg::CfgNodeKind::Exit, "Exit");
    Graph.setRoot(Entry);
  }

  [[nodiscard]] cfg::CfgNodeId
  addNode(cfg::CfgNodeKind Kind, std::string Label,
          std::optional<cfg::CfgTransferKind> Transfer = std::nullopt) {
    cfg::CfgNodeMetadata Metadata;
    Metadata.Transfer = Transfer;
    return Graph.addNode(cfg::CfgNode{Kind, std::move(Metadata),
                                      cfg::CfgNodeDisplay{std::move(Label)}});
  }

  [[nodiscard]] cfg::CfgEdgeId
  connect(cfg::CfgNodeId Source, cfg::CfgNodeId Target, cfg::CfgEdgeKind Kind,
          std::optional<std::string> Label = std::nullopt) {
    return Graph.addEdge(Source, Target,
                         cfg::CfgEdge{Kind, std::nullopt, std::move(Label)});
  }

  [[nodiscard]] cfg::FunctionCfgGraph
  finish(std::string FunctionId = "function-id") {
    return cfg::FunctionCfgGraph(makeTestFunction(std::move(FunctionId)),
                                 std::move(Graph), Exit);
  }

  cfg::CfgGraph Graph;
  cfg::CfgNodeId Entry{0};
  cfg::CfgNodeId Exit{0};
};

} // namespace mycpptools::test
