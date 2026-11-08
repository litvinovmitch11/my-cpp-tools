#include "mycpptools/cfg/cfg_graph.hpp"

#include "support/test_support.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace cfg = mycpptools::cfg;
namespace source = mycpptools::source;

static source::FunctionInfo makeFunction() {
  return source::FunctionInfo{source::FunctionId("function-id"), "run",
                              "sample::run", "void ()", std::nullopt};
}

static cfg::CfgNode
makeNode(cfg::CfgNodeKind Kind, std::string Label,
         std::optional<cfg::CfgTransferKind> Transfer = std::nullopt) {
  cfg::CfgNodeMetadata Metadata;
  Metadata.Transfer = Transfer;
  return cfg::CfgNode{Kind, std::move(Metadata),
                      cfg::CfgNodeDisplay{std::move(Label)}};
}

static cfg::CfgEdge makeEdge(cfg::CfgEdgeKind Kind) {
  return cfg::CfgEdge{Kind, std::nullopt, std::nullopt};
}

static void expectInvalidGraph(cfg::CfgGraph Graph, cfg::CfgNodeId Exit,
                               std::string_view ExpectedMessage) {
  try {
    (void)cfg::FunctionCfgGraph(makeFunction(), std::move(Graph), Exit);
  } catch (const std::invalid_argument &Error) {
    MYCPPTOOLS_EXPECT_EQ(std::string_view(Error.what()), ExpectedMessage);
    return;
  }
  MYCPPTOOLS_EXPECT_TRUE(false);
}

namespace {

class CfgFixture {
public:
  explicit CfgFixture(
      cfg::CfgNodeKind SubjectKind,
      std::optional<cfg::CfgTransferKind> Transfer = std::nullopt) {
    Entry = Graph.addNode(makeNode(cfg::CfgNodeKind::Entry, "Entry"));
    Subject = Graph.addNode(makeNode(SubjectKind, "subject", Transfer));
    Exit = Graph.addNode(makeNode(cfg::CfgNodeKind::Exit, "Exit"));
    Graph.setRoot(Entry);
    connect(Entry, Subject, cfg::CfgEdgeKind::Flow);
  }

  void connect(cfg::CfgNodeId Source, cfg::CfgNodeId Target,
               cfg::CfgEdgeKind Kind) {
    (void)Graph.addEdge(Source, Target, makeEdge(Kind));
  }

  void expectInvalid(std::string_view Message) {
    expectInvalidGraph(std::move(Graph), Exit, Message);
  }

  cfg::CfgGraph Graph;
  cfg::CfgNodeId Entry{0};
  cfg::CfgNodeId Subject{0};
  cfg::CfgNodeId Exit{0};
};

} // namespace

static cfg::FunctionCfgGraph makeGraphWithUnreachableNode() {
  cfg::CfgGraph Graph;
  const cfg::CfgNodeId Entry = Graph.addNode(
      cfg::CfgNode{cfg::CfgNodeKind::Entry, {}, cfg::CfgNodeDisplay{"Entry"}});
  const cfg::CfgNodeId Operation = Graph.addNode(cfg::CfgNode{
      cfg::CfgNodeKind::Operation, {}, cfg::CfgNodeDisplay{"work"}});
  const cfg::CfgNodeId Exit = Graph.addNode(
      cfg::CfgNode{cfg::CfgNodeKind::Exit, {}, cfg::CfgNodeDisplay{"Exit"}});
  (void)Graph.addNode(cfg::CfgNode{
      cfg::CfgNodeKind::Operation, {}, cfg::CfgNodeDisplay{"unreachable"}});
  Graph.setRoot(Entry);
  (void)Graph.addEdge(
      Entry, Operation,
      cfg::CfgEdge{cfg::CfgEdgeKind::Flow, std::nullopt, std::nullopt});
  (void)Graph.addEdge(
      Operation, Exit,
      cfg::CfgEdge{cfg::CfgEdgeKind::Flow, std::nullopt, std::nullopt});
  return cfg::FunctionCfgGraph(makeFunction(), std::move(Graph), Exit);
}

static void exposesEntryExitAndReachability() {
  const cfg::FunctionCfgGraph FunctionGraph = makeGraphWithUnreachableNode();

  MYCPPTOOLS_EXPECT_EQ(FunctionGraph.entry(), cfg::CfgNodeId(0));
  MYCPPTOOLS_EXPECT_EQ(FunctionGraph.exit(), cfg::CfgNodeId(2));
  const std::vector<bool> Reachable = cfg::computeReachableNodes(FunctionGraph);
  MYCPPTOOLS_EXPECT_EQ(Reachable.size(), std::size_t(4));
  MYCPPTOOLS_EXPECT_TRUE(Reachable[0]);
  MYCPPTOOLS_EXPECT_TRUE(Reachable[1]);
  MYCPPTOOLS_EXPECT_TRUE(Reachable[2]);
  MYCPPTOOLS_EXPECT_TRUE(!Reachable[3]);
}

static void rejectsMultipleEntrySuccessors() {
  cfg::CfgGraph Graph;
  const cfg::CfgNodeId Entry = Graph.addNode(
      cfg::CfgNode{cfg::CfgNodeKind::Entry, {}, cfg::CfgNodeDisplay{"Entry"}});
  const cfg::CfgNodeId First = Graph.addNode(cfg::CfgNode{
      cfg::CfgNodeKind::Operation, {}, cfg::CfgNodeDisplay{"first"}});
  const cfg::CfgNodeId Exit = Graph.addNode(
      cfg::CfgNode{cfg::CfgNodeKind::Exit, {}, cfg::CfgNodeDisplay{"Exit"}});
  Graph.setRoot(Entry);
  (void)Graph.addEdge(
      Entry, First,
      cfg::CfgEdge{cfg::CfgEdgeKind::Flow, std::nullopt, std::nullopt});
  (void)Graph.addEdge(
      Entry, Exit,
      cfg::CfgEdge{cfg::CfgEdgeKind::Flow, std::nullopt, std::nullopt});
  (void)Graph.addEdge(
      First, Exit,
      cfg::CfgEdge{cfg::CfgEdgeKind::Flow, std::nullopt, std::nullopt});

  MYCPPTOOLS_EXPECT_THROWS(
      cfg::FunctionCfgGraph(makeFunction(), std::move(Graph), Exit),
      std::invalid_argument);
}

static void rejectsBranchFromOperation() {
  cfg::CfgGraph Graph;
  const cfg::CfgNodeId Entry = Graph.addNode(
      cfg::CfgNode{cfg::CfgNodeKind::Entry, {}, cfg::CfgNodeDisplay{"Entry"}});
  const cfg::CfgNodeId Operation = Graph.addNode(cfg::CfgNode{
      cfg::CfgNodeKind::Operation, {}, cfg::CfgNodeDisplay{"work"}});
  const cfg::CfgNodeId Exit = Graph.addNode(
      cfg::CfgNode{cfg::CfgNodeKind::Exit, {}, cfg::CfgNodeDisplay{"Exit"}});
  Graph.setRoot(Entry);
  (void)Graph.addEdge(
      Entry, Operation,
      cfg::CfgEdge{cfg::CfgEdgeKind::Flow, std::nullopt, std::nullopt});
  (void)Graph.addEdge(
      Operation, Exit,
      cfg::CfgEdge{cfg::CfgEdgeKind::TrueBranch, std::nullopt, std::nullopt});

  MYCPPTOOLS_EXPECT_THROWS(
      cfg::FunctionCfgGraph(makeFunction(), std::move(Graph), Exit),
      std::invalid_argument);
}

static void acceptsExceptionalOperationSuccessor() {
  cfg::CfgGraph Graph;
  const cfg::CfgNodeId Entry = Graph.addNode(
      cfg::CfgNode{cfg::CfgNodeKind::Entry, {}, cfg::CfgNodeDisplay{"Entry"}});
  const cfg::CfgNodeId Operation = Graph.addNode(cfg::CfgNode{
      cfg::CfgNodeKind::Operation, {}, cfg::CfgNodeDisplay{"may throw"}});
  const cfg::CfgNodeId Exit = Graph.addNode(
      cfg::CfgNode{cfg::CfgNodeKind::Exit, {}, cfg::CfgNodeDisplay{"Exit"}});
  Graph.setRoot(Entry);
  (void)Graph.addEdge(
      Entry, Operation,
      cfg::CfgEdge{cfg::CfgEdgeKind::Flow, std::nullopt, std::nullopt});
  (void)Graph.addEdge(
      Operation, Exit,
      cfg::CfgEdge{cfg::CfgEdgeKind::Flow, std::nullopt, std::nullopt});
  (void)Graph.addEdge(
      Operation, Exit,
      cfg::CfgEdge{cfg::CfgEdgeKind::ExceptionBranch, std::nullopt, "throws"});

  const cfg::FunctionCfgGraph FunctionGraph(makeFunction(), std::move(Graph),
                                            Exit);
  MYCPPTOOLS_EXPECT_EQ(FunctionGraph.graph().edgeCount(), std::size_t(3));
}

static void acceptsOperationWithoutNormalSuccessor() {
  CfgFixture Fixture(cfg::CfgNodeKind::Operation);

  const cfg::FunctionCfgGraph FunctionGraph(
      makeFunction(), std::move(Fixture.Graph), Fixture.Exit);
  const std::vector<bool> Reachable = cfg::computeReachableNodes(FunctionGraph);
  MYCPPTOOLS_EXPECT_TRUE(Reachable[Fixture.Entry.value()]);
  MYCPPTOOLS_EXPECT_TRUE(Reachable[Fixture.Subject.value()]);
  MYCPPTOOLS_EXPECT_TRUE(!Reachable[Fixture.Exit.value()]);
}

static void rejectsDuplicateUniqueConditionBranches() {
  const auto ExpectDuplicate = [](cfg::CfgEdgeKind Kind,
                                  std::string_view Message) {
    CfgFixture Fixture(cfg::CfgNodeKind::Condition);
    Fixture.connect(Fixture.Subject, Fixture.Exit, Kind);
    Fixture.connect(Fixture.Subject, Fixture.Exit, Kind);
    Fixture.expectInvalid(Message);
  };

  ExpectDuplicate(cfg::CfgEdgeKind::TrueBranch, "duplicate true CFG branch");
  ExpectDuplicate(cfg::CfgEdgeKind::FalseBranch, "duplicate false CFG branch");
  ExpectDuplicate(cfg::CfgEdgeKind::DefaultBranch,
                  "duplicate default CFG branch");
}

static void rejectsMalformedOperationSuccessors() {
  {
    CfgFixture Fixture(cfg::CfgNodeKind::Operation);
    Fixture.connect(Fixture.Subject, Fixture.Exit, cfg::CfgEdgeKind::Flow);
    Fixture.connect(Fixture.Subject, Fixture.Exit, cfg::CfgEdgeKind::Flow);
    Fixture.expectInvalid(
        "function CFG operation has multiple flow successors");
  }
  {
    CfgFixture Fixture(cfg::CfgNodeKind::Operation);
    Fixture.connect(Fixture.Subject, Fixture.Exit,
                    cfg::CfgEdgeKind::ExceptionBranch);
    Fixture.connect(Fixture.Subject, Fixture.Exit,
                    cfg::CfgEdgeKind::ExceptionBranch);
    Fixture.expectInvalid(
        "function CFG operation has multiple exception successors");
  }
}

static void rejectsMismatchedTransferMetadata() {
  {
    CfgFixture Fixture(cfg::CfgNodeKind::Transfer);
    Fixture.connect(Fixture.Subject, Fixture.Exit, cfg::CfgEdgeKind::Flow);
    Fixture.expectInvalid(
        "function CFG transfer metadata must match the node kind");
  }
  {
    CfgFixture Fixture(cfg::CfgNodeKind::Operation,
                       cfg::CfgTransferKind::Return);
    Fixture.connect(Fixture.Subject, Fixture.Exit, cfg::CfgEdgeKind::Flow);
    Fixture.expectInvalid(
        "function CFG transfer metadata must match the node kind");
  }
}

static void rejectsMalformedTransfers() {
  {
    CfgFixture Fixture(cfg::CfgNodeKind::Transfer,
                       cfg::CfgTransferKind::Return);
    Fixture.expectInvalid("function CFG transfer must have a successor");
  }
  {
    CfgFixture Fixture(cfg::CfgNodeKind::Transfer,
                       cfg::CfgTransferKind::Return);
    Fixture.connect(Fixture.Subject, Fixture.Exit,
                    cfg::CfgEdgeKind::ExceptionBranch);
    Fixture.expectInvalid("function CFG transfer must have one flow successor");
  }
  {
    CfgFixture Fixture(cfg::CfgNodeKind::Transfer, cfg::CfgTransferKind::Throw);
    Fixture.connect(Fixture.Subject, Fixture.Exit, cfg::CfgEdgeKind::Flow);
    Fixture.expectInvalid("throw must have one exceptional CFG successor");
  }
  {
    CfgFixture Fixture(cfg::CfgNodeKind::Transfer,
                       cfg::CfgTransferKind::IndirectGoto);
    Fixture.connect(Fixture.Subject, Fixture.Exit, cfg::CfgEdgeKind::Flow);
    Fixture.expectInvalid("indirect goto must use indirect CFG branches");
  }
}

static void rejectsMalformedConditionAndAnchorEdges() {
  {
    CfgFixture Fixture(cfg::CfgNodeKind::Condition);
    Fixture.expectInvalid(
        "function CFG condition must have at least one successor");
  }
  {
    CfgFixture Fixture(cfg::CfgNodeKind::Condition);
    Fixture.connect(Fixture.Subject, Fixture.Exit, cfg::CfgEdgeKind::Flow);
    Fixture.expectInvalid(
        "function CFG condition edges must name their branch");
  }
  {
    CfgFixture Fixture(cfg::CfgNodeKind::Anchor);
    Fixture.connect(Fixture.Subject, Fixture.Exit,
                    cfg::CfgEdgeKind::TrueBranch);
    Fixture.expectInvalid(
        "function CFG linear nodes must use flow or resume edges");
  }
  {
    CfgFixture Fixture(cfg::CfgNodeKind::Anchor);
    Fixture.connect(Fixture.Subject, Fixture.Exit, cfg::CfgEdgeKind::Flow);
    Fixture.connect(Fixture.Subject, Fixture.Exit, cfg::CfgEdgeKind::Flow);
    Fixture.expectInvalid(
        "function CFG non-condition node has multiple successors");
  }
}

static void rejectsMalformedEntryAndExitShape() {
  {
    cfg::CfgGraph Graph;
    const cfg::CfgNodeId Entry =
        Graph.addNode(makeNode(cfg::CfgNodeKind::Entry, "Entry"));
    const cfg::CfgNodeId Exit =
        Graph.addNode(makeNode(cfg::CfgNodeKind::Exit, "Exit"));
    Graph.setRoot(Entry);
    (void)Graph.addEdge(Entry, Exit, makeEdge(cfg::CfgEdgeKind::TrueBranch));
    expectInvalidGraph(std::move(Graph), Exit,
                       "function CFG entry edge must be flow");
  }
  {
    CfgFixture Fixture(cfg::CfgNodeKind::Operation);
    Fixture.connect(Fixture.Subject, Fixture.Exit, cfg::CfgEdgeKind::Flow);
    Fixture.connect(Fixture.Exit, Fixture.Subject, cfg::CfgEdgeKind::Flow);
    Fixture.expectInvalid("function CFG exit must have no successors");
  }
  {
    CfgFixture Fixture(cfg::CfgNodeKind::Condition);
    Fixture.connect(Fixture.Subject, Fixture.Entry,
                    cfg::CfgEdgeKind::TrueBranch);
    Fixture.connect(Fixture.Subject, Fixture.Exit,
                    cfg::CfgEdgeKind::FalseBranch);
    Fixture.expectInvalid("function CFG entry must have no predecessors");
  }
}

int main() {
  return mycpptools::test::runTests({
      {"exposes entry, exit, and reachability",
       exposesEntryExitAndReachability},
      {"rejects multiple entry successors", rejectsMultipleEntrySuccessors},
      {"rejects a branch from an operation", rejectsBranchFromOperation},
      {"accepts an exceptional operation successor",
       acceptsExceptionalOperationSuccessor},
      {"accepts an operation without a normal successor",
       acceptsOperationWithoutNormalSuccessor},
      {"rejects duplicate unique condition branches",
       rejectsDuplicateUniqueConditionBranches},
      {"rejects malformed operation successors",
       rejectsMalformedOperationSuccessors},
      {"rejects mismatched transfer metadata",
       rejectsMismatchedTransferMetadata},
      {"rejects malformed transfers", rejectsMalformedTransfers},
      {"rejects malformed condition and anchor edges",
       rejectsMalformedConditionAndAnchorEdges},
      {"rejects malformed entry and exit shape",
       rejectsMalformedEntryAndExitShape},
  });
}
