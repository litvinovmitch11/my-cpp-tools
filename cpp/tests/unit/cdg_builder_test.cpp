#include "mycpptools/cdg/cdg_builder.hpp"

#include "support/cfg_test_graph.hpp"
#include "support/test_support.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace cdg = mycpptools::cdg;
namespace cfg = mycpptools::cfg;
namespace test = mycpptools::test;

static bool hasDependency(const cdg::FunctionCdgGraph &Function,
                          cfg::CfgNodeId Controller, cfg::CfgNodeId Dependent,
                          cfg::CfgEdgeKind Branch) {
  const cdg::CdgGraph &Graph = Function.graph();
  const cdg::CdgNodeId Source = Function.nodeForCfg(Controller);
  const cdg::CdgNodeId Target = Function.nodeForCfg(Dependent);
  for (const cdg::CdgEdgeId EdgeId : Graph.outgoingEdges(Source)) {
    const cdg::CdgGraph::Edge &Edge = Graph.edge(EdgeId);
    if (Edge.Target == Target && Edge.Payload.ControllingBranch == Branch)
      return true;
  }
  return false;
}

static void buildsDiamondDependencies() {
  test::CfgTestGraph Builder;
  const cfg::CfgNodeId Predicate =
      Builder.addNode(cfg::CfgNodeKind::Condition, "predicate");
  const cfg::CfgNodeId Then =
      Builder.addNode(cfg::CfgNodeKind::Operation, "then");
  const cfg::CfgNodeId Else =
      Builder.addNode(cfg::CfgNodeKind::Operation, "else");
  const cfg::CfgNodeId Join = Builder.addNode(cfg::CfgNodeKind::Anchor, "join");
  (void)Builder.connect(Builder.Entry, Predicate, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Predicate, Then, cfg::CfgEdgeKind::TrueBranch, "true");
  (void)Builder.connect(Predicate, Else, cfg::CfgEdgeKind::FalseBranch,
                        "false");
  (void)Builder.connect(Then, Join, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Else, Join, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Join, Builder.Exit, cfg::CfgEdgeKind::Flow);
  const cfg::FunctionCfgGraph Cfg = Builder.finish();

  const cdg::FunctionCdgGraph Cdg = cdg::buildFunctionCdgGraph(Cfg);
  MYCPPTOOLS_EXPECT_EQ(Cdg.graph().nodeCount(), Cfg.graph().nodeCount());
  MYCPPTOOLS_EXPECT_EQ(Cdg.graph().edgeCount(), std::size_t(2));
  MYCPPTOOLS_EXPECT_TRUE(
      hasDependency(Cdg, Predicate, Then, cfg::CfgEdgeKind::TrueBranch));
  MYCPPTOOLS_EXPECT_TRUE(
      hasDependency(Cdg, Predicate, Else, cfg::CfgEdgeKind::FalseBranch));
  MYCPPTOOLS_EXPECT_TRUE(
      !hasDependency(Cdg, Predicate, Join, cfg::CfgEdgeKind::TrueBranch));
  MYCPPTOOLS_EXPECT_EQ(Cdg.graph().node(Cdg.nodeForCfg(Predicate)).Payload.Kind,
                       cdg::CdgNodeKind::Predicate);
}

static void buildsLoopDependencies() {
  test::CfgTestGraph Builder;
  const cfg::CfgNodeId Predicate =
      Builder.addNode(cfg::CfgNodeKind::Condition, "loop");
  const cfg::CfgNodeId Body =
      Builder.addNode(cfg::CfgNodeKind::Operation, "body");
  (void)Builder.connect(Builder.Entry, Predicate, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Predicate, Body, cfg::CfgEdgeKind::TrueBranch);
  (void)Builder.connect(Predicate, Builder.Exit, cfg::CfgEdgeKind::FalseBranch);
  (void)Builder.connect(Body, Predicate, cfg::CfgEdgeKind::Flow);
  const cfg::FunctionCfgGraph Cfg = Builder.finish();

  const cdg::FunctionCdgGraph Cdg = cdg::buildFunctionCdgGraph(Cfg);
  MYCPPTOOLS_EXPECT_TRUE(
      hasDependency(Cdg, Predicate, Body, cfg::CfgEdgeKind::TrueBranch));
  MYCPPTOOLS_EXPECT_TRUE(
      hasDependency(Cdg, Predicate, Predicate, cfg::CfgEdgeKind::TrueBranch));
  MYCPPTOOLS_EXPECT_TRUE(hasDependency(Cdg, Predicate, Builder.Exit,
                                       cfg::CfgEdgeKind::FalseBranch));
}

static void preservesSwitchBranchOrigins() {
  test::CfgTestGraph Builder;
  const cfg::CfgNodeId Switch =
      Builder.addNode(cfg::CfgNodeKind::Condition, "switch");
  const cfg::CfgNodeId First =
      Builder.addNode(cfg::CfgNodeKind::Operation, "case 1");
  const cfg::CfgNodeId Second =
      Builder.addNode(cfg::CfgNodeKind::Operation, "case 2");
  const cfg::CfgNodeId Default =
      Builder.addNode(cfg::CfgNodeKind::Operation, "default");
  const cfg::CfgNodeId Join = Builder.addNode(cfg::CfgNodeKind::Anchor, "join");
  (void)Builder.connect(Builder.Entry, Switch, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Switch, First, cfg::CfgEdgeKind::CaseBranch, "1");
  (void)Builder.connect(Switch, Second, cfg::CfgEdgeKind::CaseBranch, "2");
  (void)Builder.connect(Switch, Default, cfg::CfgEdgeKind::DefaultBranch,
                        "default");
  (void)Builder.connect(First, Join, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Second, Join, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Default, Join, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Join, Builder.Exit, cfg::CfgEdgeKind::Flow);

  const cdg::FunctionCdgGraph Cdg =
      cdg::buildFunctionCdgGraph(Builder.finish());
  MYCPPTOOLS_EXPECT_TRUE(
      hasDependency(Cdg, Switch, First, cfg::CfgEdgeKind::CaseBranch));
  MYCPPTOOLS_EXPECT_TRUE(
      hasDependency(Cdg, Switch, Second, cfg::CfgEdgeKind::CaseBranch));
  MYCPPTOOLS_EXPECT_TRUE(
      hasDependency(Cdg, Switch, Default, cfg::CfgEdgeKind::DefaultBranch));
  MYCPPTOOLS_EXPECT_TRUE(
      !hasDependency(Cdg, Switch, Join, cfg::CfgEdgeKind::CaseBranch));
}

static void distinguishesExceptionalControl() {
  test::CfgTestGraph Builder;
  const cfg::CfgNodeId Call =
      Builder.addNode(cfg::CfgNodeKind::Operation, "call");
  const cfg::CfgNodeId Normal =
      Builder.addNode(cfg::CfgNodeKind::Operation, "normal");
  const cfg::CfgNodeId Handler =
      Builder.addNode(cfg::CfgNodeKind::Operation, "handler");
  const cfg::CfgNodeId Join = Builder.addNode(cfg::CfgNodeKind::Anchor, "join");
  (void)Builder.connect(Builder.Entry, Call, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Call, Normal, cfg::CfgEdgeKind::Flow);
  const cfg::CfgEdgeId Exception = Builder.connect(
      Call, Handler, cfg::CfgEdgeKind::ExceptionBranch, "throws");
  (void)Builder.connect(Normal, Join, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Handler, Join, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Join, Builder.Exit, cfg::CfgEdgeKind::Flow);

  const cdg::FunctionCdgGraph Cdg =
      cdg::buildFunctionCdgGraph(Builder.finish());
  MYCPPTOOLS_EXPECT_TRUE(
      hasDependency(Cdg, Call, Normal, cfg::CfgEdgeKind::Flow));
  MYCPPTOOLS_EXPECT_TRUE(
      hasDependency(Cdg, Call, Handler, cfg::CfgEdgeKind::ExceptionBranch));
  bool FoundExceptionalEdge = false;
  for (const cdg::CdgGraph::Edge &Edge : Cdg.graph().edges()) {
    if (Edge.Payload.CfgOrigin == Exception) {
      MYCPPTOOLS_EXPECT_EQ(Edge.Payload.Kind,
                           cdg::CdgEdgeKind::ExceptionalControl);
      MYCPPTOOLS_EXPECT_EQ(Edge.Payload.Label,
                           std::optional<std::string>("throws"));
      FoundExceptionalEdge = true;
    }
  }
  MYCPPTOOLS_EXPECT_TRUE(FoundExceptionalEdge);
}

static void buildsIndirectAndAsmDependencies() {
  {
    test::CfgTestGraph Builder;
    const cfg::CfgNodeId Indirect =
        Builder.addNode(cfg::CfgNodeKind::Transfer, "indirect",
                        cfg::CfgTransferKind::IndirectGoto);
    const cfg::CfgNodeId First =
        Builder.addNode(cfg::CfgNodeKind::Anchor, "first");
    const cfg::CfgNodeId Second =
        Builder.addNode(cfg::CfgNodeKind::Anchor, "second");
    const cfg::CfgNodeId Join =
        Builder.addNode(cfg::CfgNodeKind::Anchor, "join");
    (void)Builder.connect(Builder.Entry, Indirect, cfg::CfgEdgeKind::Flow);
    (void)Builder.connect(Indirect, First, cfg::CfgEdgeKind::IndirectBranch);
    (void)Builder.connect(Indirect, Second, cfg::CfgEdgeKind::IndirectBranch);
    (void)Builder.connect(First, Join, cfg::CfgEdgeKind::Flow);
    (void)Builder.connect(Second, Join, cfg::CfgEdgeKind::Flow);
    (void)Builder.connect(Join, Builder.Exit, cfg::CfgEdgeKind::Flow);
    const cdg::FunctionCdgGraph Cdg =
        cdg::buildFunctionCdgGraph(Builder.finish());
    MYCPPTOOLS_EXPECT_TRUE(
        hasDependency(Cdg, Indirect, First, cfg::CfgEdgeKind::IndirectBranch));
    MYCPPTOOLS_EXPECT_TRUE(
        hasDependency(Cdg, Indirect, Second, cfg::CfgEdgeKind::IndirectBranch));
  }
  {
    test::CfgTestGraph Builder;
    const cfg::CfgNodeId Assembly =
        Builder.addNode(cfg::CfgNodeKind::Condition, "asm goto");
    const cfg::CfgNodeId Target =
        Builder.addNode(cfg::CfgNodeKind::Anchor, "target");
    const cfg::CfgNodeId Fallthrough =
        Builder.addNode(cfg::CfgNodeKind::Anchor, "fallthrough");
    const cfg::CfgNodeId Join =
        Builder.addNode(cfg::CfgNodeKind::Anchor, "join");
    (void)Builder.connect(Builder.Entry, Assembly, cfg::CfgEdgeKind::Flow);
    (void)Builder.connect(Assembly, Target, cfg::CfgEdgeKind::AsmBranch);
    (void)Builder.connect(Assembly, Fallthrough, cfg::CfgEdgeKind::Fallthrough);
    (void)Builder.connect(Target, Join, cfg::CfgEdgeKind::Flow);
    (void)Builder.connect(Fallthrough, Join, cfg::CfgEdgeKind::Flow);
    (void)Builder.connect(Join, Builder.Exit, cfg::CfgEdgeKind::Flow);
    const cdg::FunctionCdgGraph Cdg =
        cdg::buildFunctionCdgGraph(Builder.finish());
    MYCPPTOOLS_EXPECT_TRUE(
        hasDependency(Cdg, Assembly, Target, cfg::CfgEdgeKind::AsmBranch));
    MYCPPTOOLS_EXPECT_TRUE(hasDependency(Cdg, Assembly, Fallthrough,
                                         cfg::CfgEdgeKind::Fallthrough));
  }
}

static void retainsUnreachableDependencies() {
  test::CfgTestGraph Builder;
  const cfg::CfgNodeId Predicate =
      Builder.addNode(cfg::CfgNodeKind::Condition, "unreachable predicate");
  const cfg::CfgNodeId First =
      Builder.addNode(cfg::CfgNodeKind::Operation, "first");
  const cfg::CfgNodeId Second =
      Builder.addNode(cfg::CfgNodeKind::Operation, "second");
  const cfg::CfgNodeId Join = Builder.addNode(cfg::CfgNodeKind::Anchor, "join");
  (void)Builder.connect(Builder.Entry, Builder.Exit, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Predicate, First, cfg::CfgEdgeKind::TrueBranch);
  (void)Builder.connect(Predicate, Second, cfg::CfgEdgeKind::FalseBranch);
  (void)Builder.connect(First, Join, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Second, Join, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Join, Builder.Exit, cfg::CfgEdgeKind::Flow);
  const cfg::FunctionCfgGraph Cfg = Builder.finish();

  const cdg::FunctionCdgGraph Cdg = cdg::buildFunctionCdgGraph(Cfg);
  MYCPPTOOLS_EXPECT_TRUE(!Cdg.graph()
                              .node(Cdg.nodeForCfg(Predicate))
                              .Payload.Metadata.ReachableFromEntry);
  MYCPPTOOLS_EXPECT_TRUE(
      hasDependency(Cdg, Predicate, First, cfg::CfgEdgeKind::TrueBranch));
  MYCPPTOOLS_EXPECT_TRUE(
      hasDependency(Cdg, Predicate, Second, cfg::CfgEdgeKind::FalseBranch));
}

static void buildsNoReturnDependencies() {
  test::CfgTestGraph Builder;
  const cfg::CfgNodeId Predicate =
      Builder.addNode(cfg::CfgNodeKind::Condition, "predicate");
  const cfg::CfgNodeId NoReturn =
      Builder.addNode(cfg::CfgNodeKind::Operation, "no return");
  (void)Builder.connect(Builder.Entry, Predicate, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Predicate, NoReturn, cfg::CfgEdgeKind::TrueBranch);
  (void)Builder.connect(Predicate, Builder.Exit, cfg::CfgEdgeKind::FalseBranch);

  const cdg::FunctionCdgGraph Cdg =
      cdg::buildFunctionCdgGraph(Builder.finish());
  MYCPPTOOLS_EXPECT_TRUE(
      hasDependency(Cdg, Predicate, NoReturn, cfg::CfgEdgeKind::TrueBranch));
  MYCPPTOOLS_EXPECT_TRUE(hasDependency(Cdg, Predicate, Builder.Exit,
                                       cfg::CfgEdgeKind::FalseBranch));
}

static void buildsCollectionsInOrder() {
  test::CfgTestGraph First;
  (void)First.connect(First.Entry, First.Exit, cfg::CfgEdgeKind::Flow);
  test::CfgTestGraph Second;
  (void)Second.connect(Second.Entry, Second.Exit, cfg::CfgEdgeKind::Flow);
  cfg::FunctionCfgGraphs Inputs;
  Inputs.push_back(First.finish("first"));
  Inputs.push_back(Second.finish("second"));

  const cdg::FunctionCdgGraphs Results = cdg::buildFunctionCdgGraphs(Inputs);
  MYCPPTOOLS_EXPECT_EQ(Results.size(), std::size_t(2));
  MYCPPTOOLS_EXPECT_EQ(Results[0].function().Id.value(), "first");
  MYCPPTOOLS_EXPECT_EQ(Results[1].function().Id.value(), "second");
}

int main() {
  return mycpptools::test::runTests({
      {"builds diamond dependencies", buildsDiamondDependencies},
      {"builds loop dependencies", buildsLoopDependencies},
      {"preserves switch branch origins", preservesSwitchBranchOrigins},
      {"distinguishes exceptional control", distinguishesExceptionalControl},
      {"builds indirect and asm dependencies",
       buildsIndirectAndAsmDependencies},
      {"retains unreachable dependencies", retainsUnreachableDependencies},
      {"builds no-return dependencies", buildsNoReturnDependencies},
      {"builds collections in order", buildsCollectionsInOrder},
  });
}
