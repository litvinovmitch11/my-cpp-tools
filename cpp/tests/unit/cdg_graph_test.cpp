#include "mycpptools/cdg/cdg_graph.hpp"

#include "support/cfg_test_graph.hpp"
#include "support/test_support.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace cdg = mycpptools::cdg;
namespace cfg = mycpptools::cfg;
namespace test = mycpptools::test;

static cdg::CdgNode makeNode(cdg::CdgNodeKind Kind, cfg::CfgNodeId Origin,
                             std::string Label) {
  return cdg::CdgNode{Kind, cdg::CdgNodeMetadata{Origin, std::nullopt, true},
                      cdg::CdgNodeDisplay{std::move(Label)}};
}

static cdg::FunctionCdgGraph makeValidGraph() {
  test::CfgTestGraph CfgBuilder;
  (void)CfgBuilder.connect(CfgBuilder.Entry, CfgBuilder.Exit,
                           cfg::CfgEdgeKind::Flow);
  const cfg::FunctionCfgGraph Cfg = CfgBuilder.finish();
  cdg::CdgGraph Graph;
  const cdg::CdgNodeId Entry = Graph.addNode(
      makeNode(cdg::CdgNodeKind::Entry, cfg::CfgNodeId(0), "Entry"));
  const cdg::CdgNodeId Exit = Graph.addNode(
      makeNode(cdg::CdgNodeKind::Exit, cfg::CfgNodeId(1), "Exit"));
  Graph.setRoot(Entry);
  return cdg::FunctionCdgGraph(Cfg, std::move(Graph), {Entry, Exit});
}

static void exposesMappingAndKinds() {
  const cdg::FunctionCdgGraph FunctionGraph = makeValidGraph();

  MYCPPTOOLS_EXPECT_EQ(FunctionGraph.entry(), cdg::CdgNodeId(0));
  MYCPPTOOLS_EXPECT_EQ(FunctionGraph.nodeForCfg(cfg::CfgNodeId(1)),
                       cdg::CdgNodeId(1));
  MYCPPTOOLS_EXPECT_EQ(cdg::toString(cdg::CdgNodeKind::Predicate), "predicate");
  MYCPPTOOLS_EXPECT_EQ(cdg::toString(cdg::CdgEdgeKind::ExceptionalControl),
                       "exceptional control");
  MYCPPTOOLS_EXPECT_EQ(cdg::toCdgNodeKind(cfg::CfgNodeKind::Condition),
                       cdg::CdgNodeKind::Predicate);
  MYCPPTOOLS_EXPECT_EQ(cdg::toCdgEdgeKind(cfg::CfgEdgeKind::ExceptionBranch),
                       cdg::CdgEdgeKind::ExceptionalControl);
  MYCPPTOOLS_EXPECT_THROWS(FunctionGraph.nodeForCfg(cfg::CfgNodeId(2)),
                           std::out_of_range);
}

static void rejectsMissingAndDuplicateOrigins() {
  test::CfgTestGraph CfgBuilder;
  (void)CfgBuilder.connect(CfgBuilder.Entry, CfgBuilder.Exit,
                           cfg::CfgEdgeKind::Flow);
  const cfg::FunctionCfgGraph Cfg = CfgBuilder.finish();
  {
    cdg::CdgGraph Graph;
    MYCPPTOOLS_EXPECT_THROWS(cdg::FunctionCdgGraph(Cfg, std::move(Graph), {}),
                             std::invalid_argument);
  }
  {
    cdg::CdgGraph Graph;
    const cdg::CdgNodeId Entry = Graph.addNode(
        makeNode(cdg::CdgNodeKind::Entry, cfg::CfgNodeId(0), "Entry"));
    const cdg::CdgNodeId Exit = Graph.addNode(
        makeNode(cdg::CdgNodeKind::Exit, cfg::CfgNodeId(0), "Exit"));
    Graph.setRoot(Entry);
    MYCPPTOOLS_EXPECT_THROWS(
        cdg::FunctionCdgGraph(Cfg, std::move(Graph), {Entry, Exit}),
        std::invalid_argument);
  }
}

static void rejectsInconsistentMapping() {
  test::CfgTestGraph CfgBuilder;
  (void)CfgBuilder.connect(CfgBuilder.Entry, CfgBuilder.Exit,
                           cfg::CfgEdgeKind::Flow);
  const cfg::FunctionCfgGraph Cfg = CfgBuilder.finish();
  cdg::CdgGraph Graph;
  const cdg::CdgNodeId Entry = Graph.addNode(
      makeNode(cdg::CdgNodeKind::Entry, cfg::CfgNodeId(0), "Entry"));
  const cdg::CdgNodeId Exit = Graph.addNode(
      makeNode(cdg::CdgNodeKind::Exit, cfg::CfgNodeId(1), "Exit"));
  Graph.setRoot(Entry);

  MYCPPTOOLS_EXPECT_THROWS(
      cdg::FunctionCdgGraph(Cfg, std::move(Graph), {Exit, Entry}),
      std::invalid_argument);
}

static void rejectsMismatchedExceptionalKind() {
  test::CfgTestGraph CfgBuilder;
  const cfg::CfgNodeId Call =
      CfgBuilder.addNode(cfg::CfgNodeKind::Operation, "call");
  (void)CfgBuilder.connect(CfgBuilder.Entry, Call, cfg::CfgEdgeKind::Flow);
  (void)CfgBuilder.connect(Call, CfgBuilder.Exit, cfg::CfgEdgeKind::Flow);
  const cfg::CfgEdgeId Exception = CfgBuilder.connect(
      Call, CfgBuilder.Exit, cfg::CfgEdgeKind::ExceptionBranch, "throws");
  const cfg::FunctionCfgGraph Cfg = CfgBuilder.finish();
  cdg::CdgGraph Graph;
  const cdg::CdgNodeId Entry = Graph.addNode(
      makeNode(cdg::CdgNodeKind::Entry, cfg::CfgNodeId(0), "Entry"));
  const cdg::CdgNodeId Exit = Graph.addNode(
      makeNode(cdg::CdgNodeKind::Exit, cfg::CfgNodeId(1), "Exit"));
  const cdg::CdgNodeId CallNode =
      Graph.addNode(makeNode(cdg::CdgNodeKind::Operation, Call, "call"));
  Graph.setRoot(Entry);
  (void)Graph.addEdge(CallNode, Exit,
                      cdg::CdgEdge{cdg::CdgEdgeKind::Control, Exception,
                                   cfg::CfgEdgeKind::ExceptionBranch,
                                   "throws"});

  MYCPPTOOLS_EXPECT_THROWS(
      cdg::FunctionCdgGraph(Cfg, std::move(Graph), {Entry, Exit, CallNode}),
      std::invalid_argument);
}

int main() {
  return mycpptools::test::runTests({
      {"exposes mapping and kinds", exposesMappingAndKinds},
      {"rejects missing and duplicate origins",
       rejectsMissingAndDuplicateOrigins},
      {"rejects inconsistent mapping", rejectsInconsistentMapping},
      {"rejects mismatched exceptional kind", rejectsMismatchedExceptionalKind},
  });
}
