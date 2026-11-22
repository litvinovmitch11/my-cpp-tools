#include "mycpptools/cdg/post_dominators.hpp"

#include "support/cfg_test_graph.hpp"
#include "support/test_support.hpp"

#include <optional>
#include <stdexcept>

namespace cdg = mycpptools::cdg;
namespace cfg = mycpptools::cfg;
namespace test = mycpptools::test;

static void computesDiamondPostDominators() {
  test::CfgTestGraph Builder;
  const cfg::CfgNodeId Predicate =
      Builder.addNode(cfg::CfgNodeKind::Condition, "predicate");
  const cfg::CfgNodeId Then =
      Builder.addNode(cfg::CfgNodeKind::Operation, "then");
  const cfg::CfgNodeId Else =
      Builder.addNode(cfg::CfgNodeKind::Operation, "else");
  const cfg::CfgNodeId Join = Builder.addNode(cfg::CfgNodeKind::Anchor, "join");
  (void)Builder.connect(Builder.Entry, Predicate, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Predicate, Then, cfg::CfgEdgeKind::TrueBranch);
  (void)Builder.connect(Predicate, Else, cfg::CfgEdgeKind::FalseBranch);
  (void)Builder.connect(Then, Join, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Else, Join, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Join, Builder.Exit, cfg::CfgEdgeKind::Flow);

  const cdg::PostDominatorRelation Relation =
      cdg::computePostDominators(Builder.finish());
  MYCPPTOOLS_EXPECT_TRUE(Relation.postDominates(Join, Predicate));
  MYCPPTOOLS_EXPECT_TRUE(Relation.postDominates(Builder.Exit, Predicate));
  MYCPPTOOLS_EXPECT_TRUE(!Relation.postDominates(Then, Predicate));
  MYCPPTOOLS_EXPECT_EQ(Relation.immediatePostDominator(Predicate),
                       std::optional<cfg::CfgNodeId>(Join));
  MYCPPTOOLS_EXPECT_EQ(Relation.immediatePostDominator(Then),
                       std::optional<cfg::CfgNodeId>(Join));
  MYCPPTOOLS_EXPECT_EQ(Relation.immediatePostDominator(Join),
                       std::optional<cfg::CfgNodeId>(Builder.Exit));
  MYCPPTOOLS_EXPECT_TRUE(Relation.canReachFunctionExit(Predicate));
  MYCPPTOOLS_EXPECT_TRUE(!Relation.mayDiverge(Predicate));
}

static void countsInfiniteLoopPaths() {
  test::CfgTestGraph Builder;
  const cfg::CfgNodeId Predicate =
      Builder.addNode(cfg::CfgNodeKind::Condition, "loop");
  const cfg::CfgNodeId Body =
      Builder.addNode(cfg::CfgNodeKind::Operation, "body");
  (void)Builder.connect(Builder.Entry, Predicate, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Predicate, Body, cfg::CfgEdgeKind::TrueBranch);
  (void)Builder.connect(Predicate, Builder.Exit, cfg::CfgEdgeKind::FalseBranch);
  (void)Builder.connect(Body, Predicate, cfg::CfgEdgeKind::Flow);

  const cdg::PostDominatorRelation Relation =
      cdg::computePostDominators(Builder.finish());
  MYCPPTOOLS_EXPECT_TRUE(Relation.canReachFunctionExit(Predicate));
  MYCPPTOOLS_EXPECT_TRUE(Relation.canReachFunctionExit(Body));
  MYCPPTOOLS_EXPECT_TRUE(Relation.mayDiverge(Predicate));
  MYCPPTOOLS_EXPECT_TRUE(Relation.mayDiverge(Body));
  MYCPPTOOLS_EXPECT_TRUE(!Relation.postDominates(Builder.Exit, Predicate));
  MYCPPTOOLS_EXPECT_TRUE(Relation.postDominates(Predicate, Body));
  MYCPPTOOLS_EXPECT_EQ(Relation.immediatePostDominator(Body),
                       std::optional<cfg::CfgNodeId>(Predicate));
  MYCPPTOOLS_EXPECT_TRUE(!Relation.immediatePostDominator(Predicate));
}

static void handlesSwitchBranches() {
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

  const cdg::PostDominatorRelation Relation =
      cdg::computePostDominators(Builder.finish());
  MYCPPTOOLS_EXPECT_EQ(Relation.immediatePostDominator(Switch),
                       std::optional<cfg::CfgNodeId>(Join));
  MYCPPTOOLS_EXPECT_TRUE(Relation.postDominates(Join, First));
  MYCPPTOOLS_EXPECT_TRUE(Relation.postDominates(Join, Second));
  MYCPPTOOLS_EXPECT_TRUE(Relation.postDominates(Join, Default));
}

static void handlesExceptionalBranches() {
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
  (void)Builder.connect(Call, Handler, cfg::CfgEdgeKind::ExceptionBranch,
                        "throws");
  (void)Builder.connect(Normal, Join, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Handler, Join, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Join, Builder.Exit, cfg::CfgEdgeKind::Flow);

  const cdg::PostDominatorRelation Relation =
      cdg::computePostDominators(Builder.finish());
  MYCPPTOOLS_EXPECT_EQ(Relation.immediatePostDominator(Call),
                       std::optional<cfg::CfgNodeId>(Join));
  MYCPPTOOLS_EXPECT_TRUE(!Relation.postDominates(Normal, Call));
  MYCPPTOOLS_EXPECT_TRUE(!Relation.postDominates(Handler, Call));
}

static void handlesIndirectAndAsmBranches() {
  {
    test::CfgTestGraph Builder;
    const cfg::CfgNodeId Indirect =
        Builder.addNode(cfg::CfgNodeKind::Transfer, "indirect goto",
                        cfg::CfgTransferKind::IndirectGoto);
    const cfg::CfgNodeId First =
        Builder.addNode(cfg::CfgNodeKind::Anchor, "first label");
    const cfg::CfgNodeId Second =
        Builder.addNode(cfg::CfgNodeKind::Anchor, "second label");
    const cfg::CfgNodeId Join =
        Builder.addNode(cfg::CfgNodeKind::Anchor, "join");
    (void)Builder.connect(Builder.Entry, Indirect, cfg::CfgEdgeKind::Flow);
    (void)Builder.connect(Indirect, First, cfg::CfgEdgeKind::IndirectBranch);
    (void)Builder.connect(Indirect, Second, cfg::CfgEdgeKind::IndirectBranch);
    (void)Builder.connect(First, Join, cfg::CfgEdgeKind::Flow);
    (void)Builder.connect(Second, Join, cfg::CfgEdgeKind::Flow);
    (void)Builder.connect(Join, Builder.Exit, cfg::CfgEdgeKind::Flow);

    const cdg::PostDominatorRelation Relation =
        cdg::computePostDominators(Builder.finish());
    MYCPPTOOLS_EXPECT_EQ(Relation.immediatePostDominator(Indirect),
                         std::optional<cfg::CfgNodeId>(Join));
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

    const cdg::PostDominatorRelation Relation =
        cdg::computePostDominators(Builder.finish());
    MYCPPTOOLS_EXPECT_EQ(Relation.immediatePostDominator(Assembly),
                         std::optional<cfg::CfgNodeId>(Join));
  }
}

static void analyzesUnreachableComponents() {
  test::CfgTestGraph Builder;
  const cfg::CfgNodeId Predicate =
      Builder.addNode(cfg::CfgNodeKind::Condition, "unreachable predicate");
  const cfg::CfgNodeId First =
      Builder.addNode(cfg::CfgNodeKind::Operation, "first");
  const cfg::CfgNodeId Second =
      Builder.addNode(cfg::CfgNodeKind::Operation, "second");
  const cfg::CfgNodeId Join =
      Builder.addNode(cfg::CfgNodeKind::Anchor, "unreachable join");
  (void)Builder.connect(Builder.Entry, Builder.Exit, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Predicate, First, cfg::CfgEdgeKind::TrueBranch);
  (void)Builder.connect(Predicate, Second, cfg::CfgEdgeKind::FalseBranch);
  (void)Builder.connect(First, Join, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Second, Join, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Join, Builder.Exit, cfg::CfgEdgeKind::Flow);

  const cdg::PostDominatorRelation Relation =
      cdg::computePostDominators(Builder.finish());
  MYCPPTOOLS_EXPECT_TRUE(Relation.canReachFunctionExit(Predicate));
  MYCPPTOOLS_EXPECT_TRUE(Relation.postDominates(Join, Predicate));
  MYCPPTOOLS_EXPECT_EQ(Relation.immediatePostDominator(Predicate),
                       std::optional<cfg::CfgNodeId>(Join));
}

static void separatesNoReturnPathsFromExit() {
  test::CfgTestGraph Builder;
  const cfg::CfgNodeId Predicate =
      Builder.addNode(cfg::CfgNodeKind::Condition, "predicate");
  const cfg::CfgNodeId NoReturn =
      Builder.addNode(cfg::CfgNodeKind::Operation, "no return");
  (void)Builder.connect(Builder.Entry, Predicate, cfg::CfgEdgeKind::Flow);
  (void)Builder.connect(Predicate, NoReturn, cfg::CfgEdgeKind::TrueBranch);
  (void)Builder.connect(Predicate, Builder.Exit, cfg::CfgEdgeKind::FalseBranch);

  const cdg::PostDominatorRelation Relation =
      cdg::computePostDominators(Builder.finish());
  MYCPPTOOLS_EXPECT_TRUE(Relation.canReachFunctionExit(Predicate));
  MYCPPTOOLS_EXPECT_TRUE(!Relation.canReachFunctionExit(NoReturn));
  MYCPPTOOLS_EXPECT_TRUE(!Relation.mayDiverge(NoReturn));
  MYCPPTOOLS_EXPECT_TRUE(!Relation.postDominates(Builder.Exit, Predicate));
  MYCPPTOOLS_EXPECT_TRUE(!Relation.immediatePostDominator(Predicate));
}

static void rejectsUnknownNodeIds() {
  test::CfgTestGraph Builder;
  (void)Builder.connect(Builder.Entry, Builder.Exit, cfg::CfgEdgeKind::Flow);
  const cdg::PostDominatorRelation Relation =
      cdg::computePostDominators(Builder.finish());
  MYCPPTOOLS_EXPECT_THROWS(Relation.mayDiverge(cfg::CfgNodeId(2)),
                           std::out_of_range);
}

int main() {
  return mycpptools::test::runTests({
      {"computes diamond post-dominators", computesDiamondPostDominators},
      {"counts infinite loop paths", countsInfiniteLoopPaths},
      {"handles switch branches", handlesSwitchBranches},
      {"handles exceptional branches", handlesExceptionalBranches},
      {"handles indirect and asm branches", handlesIndirectAndAsmBranches},
      {"analyzes unreachable components", analyzesUnreachableComponents},
      {"separates no-return paths from exit", separatesNoReturnPathsFromExit},
      {"rejects unknown node IDs", rejectsUnknownNodeIds},
  });
}
