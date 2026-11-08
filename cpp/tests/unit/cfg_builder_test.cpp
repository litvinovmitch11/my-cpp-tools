#include "mycpptools/cfg/cfg_builder.hpp"

#include "support/test_support.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace ast = mycpptools::ast;
namespace cfg = mycpptools::cfg;
namespace source = mycpptools::source;

namespace {

class AstFixture {
public:
  explicit AstFixture(
      ast::FunctionKind Function = ast::FunctionKind::Ordinary) {
    ast::AstNodeMetadata Metadata;
    Metadata.ConcreteKind = "FunctionDecl";
    Metadata.FunctionCategory = Function;
    Root = Graph.addNode(ast::AstNode{ast::AstNodeKind::FunctionDeclaration,
                                      std::move(Metadata),
                                      ast::AstNodeDisplay{"Function test"}});
    Graph.setRoot(Root);
  }

  ast::AstNodeId add(ast::AstNodeKind Kind, std::string Label,
                     ast::AstNodeMetadata Metadata = {}) {
    if (Metadata.ConcreteKind.empty())
      Metadata.ConcreteKind = Label;
    return Graph.addNode(ast::AstNode{Kind, std::move(Metadata),
                                      ast::AstNodeDisplay{std::move(Label)}});
  }

  void attach(ast::AstNodeId Parent, ast::AstNodeId Child,
              ast::AstEdgeRole Role = ast::AstEdgeRole::Child) {
    const std::size_t Ordinal = Graph.outgoingEdges(Parent).size();
    (void)Graph.addEdge(Parent, Child, ast::AstEdge{Role, Ordinal});
  }

  ast::FunctionAstGraph finish(std::string Id = "test-id") {
    source::FunctionInfo Function{source::FunctionId(std::move(Id)), "test",
                                  "test", "int ()", std::nullopt};
    return ast::FunctionAstGraph(std::move(Function), std::move(Graph));
  }

  ast::AstNodeId root() const { return Root; }

private:
  ast::AstGraph Graph;
  ast::AstNodeId Root{0};
};

} // namespace

static const cfg::FunctionCfgGraph &
expectGraph(cfg::FunctionCfgBuildResult &Result) {
  MYCPPTOOLS_EXPECT_TRUE(std::holds_alternative<cfg::FunctionCfgGraph>(Result));
  return std::get<cfg::FunctionCfgGraph>(Result);
}

static void buildsShortCircuitReturn() {
  AstFixture Fixture;
  const ast::AstNodeId Body =
      Fixture.add(ast::AstNodeKind::CompoundStatement, "CompoundStmt");
  const ast::AstNodeId Return =
      Fixture.add(ast::AstNodeKind::ReturnStatement, "return");
  ast::AstNodeMetadata AndMetadata;
  AndMetadata.Operator = "&&";
  const ast::AstNodeId And = Fixture.add(ast::AstNodeKind::BinaryExpression,
                                         "BinaryOperator &&", AndMetadata);
  const ast::AstNodeId Left =
      Fixture.add(ast::AstNodeKind::DeclarationReferenceExpression, "left");
  const ast::AstNodeId Right =
      Fixture.add(ast::AstNodeKind::DeclarationReferenceExpression, "right");
  Fixture.attach(Fixture.root(), Body, ast::AstEdgeRole::Body);
  Fixture.attach(Body, Return);
  Fixture.attach(Return, And, ast::AstEdgeRole::ReturnValue);
  Fixture.attach(And, Left, ast::AstEdgeRole::LeftOperand);
  Fixture.attach(And, Right, ast::AstEdgeRole::RightOperand);

  ast::FunctionAstGraph AstGraph = Fixture.finish();
  cfg::FunctionCfgBuildResult Result = cfg::buildFunctionCfgGraph(AstGraph);
  const cfg::FunctionCfgGraph &Cfg = expectGraph(Result);

  std::size_t Conditions = 0;
  std::size_t TrueEdges = 0;
  std::size_t FalseEdges = 0;
  for (const cfg::CfgGraph::Node &Node : Cfg.graph().nodes()) {
    if (Node.Payload.Kind == cfg::CfgNodeKind::Condition)
      ++Conditions;
  }
  for (const cfg::CfgGraph::Edge &Edge : Cfg.graph().edges()) {
    TrueEdges += Edge.Payload.Kind == cfg::CfgEdgeKind::TrueBranch;
    FalseEdges += Edge.Payload.Kind == cfg::CfgEdgeKind::FalseBranch;
  }
  MYCPPTOOLS_EXPECT_EQ(Conditions, std::size_t(2));
  MYCPPTOOLS_EXPECT_EQ(TrueEdges, std::size_t(2));
  MYCPPTOOLS_EXPECT_EQ(FalseEdges, std::size_t(2));
}

static void resolvesGotoAndKeepsUnreachableCode() {
  AstFixture Fixture;
  const ast::AstNodeId Body =
      Fixture.add(ast::AstNodeKind::CompoundStatement, "CompoundStmt");
  ast::AstNodeMetadata GotoMetadata;
  GotoMetadata.TargetLabel = "done";
  const ast::AstNodeId Goto =
      Fixture.add(ast::AstNodeKind::GotoStatement, "goto done", GotoMetadata);
  const ast::AstNodeId Dead =
      Fixture.add(ast::AstNodeKind::OtherStatement, "dead");
  ast::AstNodeMetadata LabelMetadata;
  LabelMetadata.Name = "done";
  const ast::AstNodeId Label =
      Fixture.add(ast::AstNodeKind::LabelStatement, "done", LabelMetadata);
  const ast::AstNodeId Return =
      Fixture.add(ast::AstNodeKind::ReturnStatement, "return");
  Fixture.attach(Fixture.root(), Body, ast::AstEdgeRole::Body);
  Fixture.attach(Body, Goto);
  Fixture.attach(Body, Dead);
  Fixture.attach(Body, Label);
  Fixture.attach(Label, Return, ast::AstEdgeRole::LabelBody);

  ast::FunctionAstGraph AstGraph = Fixture.finish();
  cfg::FunctionCfgBuildResult Result = cfg::buildFunctionCfgGraph(AstGraph);
  const cfg::FunctionCfgGraph &Cfg = expectGraph(Result);
  const std::vector<bool> Reachable = cfg::computeReachableNodes(Cfg);

  bool FoundUnreachableDeadNode = false;
  for (const cfg::CfgGraph::Node &Node : Cfg.graph().nodes()) {
    if (Node.Payload.Display.Label == "dead")
      FoundUnreachableDeadNode = !Reachable[Node.Id.value()];
  }
  MYCPPTOOLS_EXPECT_TRUE(FoundUnreachableDeadNode);
}

static void reportsInvalidAstTransactionally() {
  AstFixture ValidFixture;
  const ast::AstNodeId ValidBody =
      ValidFixture.add(ast::AstNodeKind::CompoundStatement, "CompoundStmt");
  ValidFixture.attach(ValidFixture.root(), ValidBody, ast::AstEdgeRole::Body);

  AstFixture InvalidFixture;
  const ast::AstNodeId InvalidBody =
      InvalidFixture.add(ast::AstNodeKind::CompoundStatement, "CompoundStmt");
  const ast::AstNodeId Try =
      InvalidFixture.add(ast::AstNodeKind::TryStatement, "try");
  InvalidFixture.attach(InvalidFixture.root(), InvalidBody,
                        ast::AstEdgeRole::Body);
  InvalidFixture.attach(InvalidBody, Try);

  ast::FunctionAstGraphs AstGraphs;
  AstGraphs.push_back(ValidFixture.finish("valid-id"));
  AstGraphs.push_back(InvalidFixture.finish("invalid-id"));
  cfg::FunctionCfgGraphsBuildResult Result =
      cfg::buildFunctionCfgGraphs(AstGraphs);

  MYCPPTOOLS_EXPECT_TRUE(std::holds_alternative<cfg::CfgBuildError>(Result));
  const cfg::CfgBuildError &Error = std::get<cfg::CfgBuildError>(Result);
  MYCPPTOOLS_EXPECT_EQ(Error.Code, cfg::CfgBuildErrorCode::InvalidAst);
  MYCPPTOOLS_EXPECT_EQ(Error.Function, source::FunctionId("invalid-id"));
}

static void buildsNestedThrowAsExceptionalTransfer() {
  AstFixture Fixture;
  const ast::AstNodeId Body =
      Fixture.add(ast::AstNodeKind::CompoundStatement, "CompoundStmt");
  const ast::AstNodeId Call =
      Fixture.add(ast::AstNodeKind::CallExpression, "CallExpr");
  const ast::AstNodeId Throw =
      Fixture.add(ast::AstNodeKind::ThrowExpression, "ThrowExpr");
  Fixture.attach(Fixture.root(), Body, ast::AstEdgeRole::Body);
  Fixture.attach(Body, Call);
  Fixture.attach(Call, Throw, ast::AstEdgeRole::Argument);

  ast::FunctionAstGraph AstGraph = Fixture.finish();
  cfg::FunctionCfgBuildResult Result = cfg::buildFunctionCfgGraph(AstGraph);
  const cfg::FunctionCfgGraph &Cfg = expectGraph(Result);

  bool FoundThrow = false;
  bool FoundExceptionalEdge = false;
  for (const cfg::CfgGraph::Node &Node : Cfg.graph().nodes()) {
    FoundThrow = FoundThrow ||
                 Node.Payload.Metadata.Transfer == cfg::CfgTransferKind::Throw;
  }
  for (const cfg::CfgGraph::Edge &Edge : Cfg.graph().edges()) {
    FoundExceptionalEdge =
        FoundExceptionalEdge ||
        Edge.Payload.Kind == cfg::CfgEdgeKind::ExceptionBranch;
  }
  MYCPPTOOLS_EXPECT_TRUE(FoundThrow);
  MYCPPTOOLS_EXPECT_TRUE(FoundExceptionalEdge);
}

static void buildsTryCatchFromSemanticRoles() {
  AstFixture Fixture;
  const ast::AstNodeId Body =
      Fixture.add(ast::AstNodeKind::CompoundStatement, "CompoundStmt");
  const ast::AstNodeId Try =
      Fixture.add(ast::AstNodeKind::TryStatement, "CXXTryStmt");
  const ast::AstNodeId TryBody =
      Fixture.add(ast::AstNodeKind::CompoundStatement, "CompoundStmt");
  const ast::AstNodeId Throw =
      Fixture.add(ast::AstNodeKind::ThrowExpression, "CXXThrowExpr");
  ast::AstNodeMetadata CatchMetadata;
  CatchMetadata.ExceptionType = "int";
  const ast::AstNodeId Catch = Fixture.add(ast::AstNodeKind::CatchStatement,
                                           "CXXCatchStmt", CatchMetadata);
  const ast::AstNodeId CatchBody =
      Fixture.add(ast::AstNodeKind::CompoundStatement, "CompoundStmt");
  const ast::AstNodeId Return =
      Fixture.add(ast::AstNodeKind::ReturnStatement, "ReturnStmt");
  Fixture.attach(Fixture.root(), Body, ast::AstEdgeRole::Body);
  Fixture.attach(Body, Try);
  Fixture.attach(Try, TryBody, ast::AstEdgeRole::TryBody);
  Fixture.attach(Try, Catch, ast::AstEdgeRole::Handler);
  Fixture.attach(TryBody, Throw);
  Fixture.attach(Catch, CatchBody, ast::AstEdgeRole::CatchBody);
  Fixture.attach(CatchBody, Return);

  ast::FunctionAstGraph AstGraph = Fixture.finish();
  cfg::FunctionCfgBuildResult Result = cfg::buildFunctionCfgGraph(AstGraph);
  const cfg::FunctionCfgGraph &Cfg = expectGraph(Result);

  std::size_t ExceptionalEdges = 0;
  for (const cfg::CfgGraph::Edge &Edge : Cfg.graph().edges()) {
    ExceptionalEdges += Edge.Payload.Kind == cfg::CfgEdgeKind::ExceptionBranch;
  }
  MYCPPTOOLS_EXPECT_EQ(ExceptionalEdges, std::size_t(3));
}

static void routesPotentiallyThrowingCallsToHandlerDispatch() {
  AstFixture Fixture;
  const ast::AstNodeId Body =
      Fixture.add(ast::AstNodeKind::CompoundStatement, "CompoundStmt");
  const ast::AstNodeId Try =
      Fixture.add(ast::AstNodeKind::TryStatement, "CXXTryStmt");
  const ast::AstNodeId TryBody =
      Fixture.add(ast::AstNodeKind::CompoundStatement, "CompoundStmt");
  ast::AstNodeMetadata CallMetadata;
  CallMetadata.MayThrow = true;
  const ast::AstNodeId Call =
      Fixture.add(ast::AstNodeKind::CallExpression, "CallExpr", CallMetadata);
  ast::AstNodeMetadata CatchMetadata;
  CatchMetadata.ExceptionType = "...";
  CatchMetadata.IsCatchAll = true;
  const ast::AstNodeId Catch = Fixture.add(ast::AstNodeKind::CatchStatement,
                                           "CXXCatchStmt", CatchMetadata);
  const ast::AstNodeId CatchBody =
      Fixture.add(ast::AstNodeKind::CompoundStatement, "CompoundStmt");
  Fixture.attach(Fixture.root(), Body, ast::AstEdgeRole::Body);
  Fixture.attach(Body, Try);
  Fixture.attach(Try, TryBody, ast::AstEdgeRole::TryBody);
  Fixture.attach(Try, Catch, ast::AstEdgeRole::Handler);
  Fixture.attach(TryBody, Call);
  Fixture.attach(Catch, CatchBody, ast::AstEdgeRole::CatchBody);

  ast::FunctionAstGraph AstGraph = Fixture.finish();
  cfg::FunctionCfgBuildResult Result = cfg::buildFunctionCfgGraph(AstGraph);
  const cfg::FunctionCfgGraph &Cfg = expectGraph(Result);

  bool CallTargetsDispatch = false;
  for (const cfg::CfgGraph::Edge &Edge : Cfg.graph().edges()) {
    const cfg::CfgGraph::Node &SourceNode = Cfg.graph().node(Edge.Source);
    const cfg::CfgGraph::Node &TargetNode = Cfg.graph().node(Edge.Target);
    CallTargetsDispatch =
        CallTargetsDispatch ||
        (SourceNode.Payload.Display.Label == "CallExpr" &&
         TargetNode.Payload.Display.Label == "exception dispatch" &&
         Edge.Payload.Kind == cfg::CfgEdgeKind::ExceptionBranch);
  }
  MYCPPTOOLS_EXPECT_TRUE(CallTargetsDispatch);
}

static void evaluatesAssignmentConditionRightBeforeLeft() {
  AstFixture Fixture;
  const ast::AstNodeId Body =
      Fixture.add(ast::AstNodeKind::CompoundStatement, "CompoundStmt");
  const ast::AstNodeId If =
      Fixture.add(ast::AstNodeKind::IfStatement, "IfStmt");
  ast::AstNodeMetadata AssignmentMetadata;
  AssignmentMetadata.Operator = "=";
  const ast::AstNodeId Assignment =
      Fixture.add(ast::AstNodeKind::BinaryExpression,
                  "BinaryOperator =", AssignmentMetadata);
  ast::AstNodeMetadata CallMetadata;
  CallMetadata.MayThrow = true;
  const ast::AstNodeId Left =
      Fixture.add(ast::AstNodeKind::CallExpression, "left", CallMetadata);
  const ast::AstNodeId Right =
      Fixture.add(ast::AstNodeKind::CallExpression, "right", CallMetadata);
  const ast::AstNodeId Then =
      Fixture.add(ast::AstNodeKind::CompoundStatement, "then");
  Fixture.attach(Fixture.root(), Body, ast::AstEdgeRole::Body);
  Fixture.attach(Body, If);
  Fixture.attach(If, Assignment, ast::AstEdgeRole::Condition);
  Fixture.attach(If, Then, ast::AstEdgeRole::ThenBranch);
  Fixture.attach(Assignment, Left, ast::AstEdgeRole::LeftOperand);
  Fixture.attach(Assignment, Right, ast::AstEdgeRole::RightOperand);

  ast::FunctionAstGraph AstGraph = Fixture.finish();
  cfg::FunctionCfgBuildResult Result = cfg::buildFunctionCfgGraph(AstGraph);
  const cfg::FunctionCfgGraph &Cfg = expectGraph(Result);

  bool RightBeforeLeft = false;
  bool LeftBeforeCondition = false;
  for (const cfg::CfgGraph::Edge &Edge : Cfg.graph().edges()) {
    if (Edge.Payload.Kind != cfg::CfgEdgeKind::Flow)
      continue;
    const std::string &Source =
        Cfg.graph().node(Edge.Source).Payload.Display.Label;
    const std::string &Target =
        Cfg.graph().node(Edge.Target).Payload.Display.Label;
    RightBeforeLeft =
        RightBeforeLeft || (Source == "right" && Target == "left");
    LeftBeforeCondition = LeftBeforeCondition ||
                          (Source == "left" && Target == "BinaryOperator =");
  }
  MYCPPTOOLS_EXPECT_TRUE(RightBeforeLeft);
  MYCPPTOOLS_EXPECT_TRUE(LeftBeforeCondition);
}

static void rethrowsAfterConstructorAndDestructorFunctionTryHandlers() {
  const auto ExpectImplicitRethrow = [](ast::FunctionKind FunctionKind) {
    AstFixture Fixture(FunctionKind);
    const ast::AstNodeId Try =
        Fixture.add(ast::AstNodeKind::TryStatement, "CXXTryStmt");
    const ast::AstNodeId TryBody =
        Fixture.add(ast::AstNodeKind::CompoundStatement, "try body");
    ast::AstNodeMetadata CatchMetadata;
    CatchMetadata.ExceptionType = "...";
    CatchMetadata.IsCatchAll = true;
    const ast::AstNodeId Catch = Fixture.add(ast::AstNodeKind::CatchStatement,
                                             "CXXCatchStmt", CatchMetadata);
    const ast::AstNodeId CatchBody =
        Fixture.add(ast::AstNodeKind::CompoundStatement, "catch body");
    Fixture.attach(Fixture.root(), Try, ast::AstEdgeRole::Body);
    Fixture.attach(Try, TryBody, ast::AstEdgeRole::TryBody);
    Fixture.attach(Try, Catch, ast::AstEdgeRole::Handler);
    Fixture.attach(Catch, CatchBody, ast::AstEdgeRole::CatchBody);

    ast::FunctionAstGraph AstGraph = Fixture.finish();
    cfg::FunctionCfgBuildResult Result = cfg::buildFunctionCfgGraph(AstGraph);
    const cfg::FunctionCfgGraph &Cfg = expectGraph(Result);

    bool FoundImplicitRethrow = false;
    bool RethrowTargetsExit = false;
    for (const cfg::CfgGraph::Node &Node : Cfg.graph().nodes()) {
      FoundImplicitRethrow =
          FoundImplicitRethrow ||
          (Node.Payload.Display.Label == "implicit rethrow" &&
           Node.Payload.Metadata.Transfer == cfg::CfgTransferKind::Throw);
    }
    for (const cfg::CfgGraph::Edge &Edge : Cfg.graph().edges()) {
      RethrowTargetsExit =
          RethrowTargetsExit ||
          (Cfg.graph().node(Edge.Source).Payload.Display.Label ==
               "implicit rethrow" &&
           Edge.Target == Cfg.exit() &&
           Edge.Payload.Kind == cfg::CfgEdgeKind::ExceptionBranch);
    }
    MYCPPTOOLS_EXPECT_TRUE(FoundImplicitRethrow);
    MYCPPTOOLS_EXPECT_TRUE(RethrowTargetsExit);
  };

  ExpectImplicitRethrow(ast::FunctionKind::Constructor);
  ExpectImplicitRethrow(ast::FunctionKind::Destructor);
}

static void reportsUnresolvedGotoTarget() {
  AstFixture Fixture;
  const ast::AstNodeId Body =
      Fixture.add(ast::AstNodeKind::CompoundStatement, "CompoundStmt");
  ast::AstNodeMetadata GotoMetadata;
  GotoMetadata.TargetLabel = "missing";
  const ast::AstNodeId Goto = Fixture.add(ast::AstNodeKind::GotoStatement,
                                          "goto missing", GotoMetadata);
  Fixture.attach(Fixture.root(), Body, ast::AstEdgeRole::Body);
  Fixture.attach(Body, Goto);

  ast::FunctionAstGraph AstGraph = Fixture.finish();
  cfg::FunctionCfgBuildResult Result = cfg::buildFunctionCfgGraph(AstGraph);
  MYCPPTOOLS_EXPECT_TRUE(std::holds_alternative<cfg::CfgBuildError>(Result));
  const cfg::CfgBuildError &Error = std::get<cfg::CfgBuildError>(Result);
  MYCPPTOOLS_EXPECT_EQ(Error.Code, cfg::CfgBuildErrorCode::UnresolvedLabel);
  MYCPPTOOLS_EXPECT_EQ(Error.AstNode, std::optional<ast::AstNodeId>(Goto));
}

static void makesCodeAfterNoreturnCallUnreachable() {
  AstFixture Fixture;
  const ast::AstNodeId Body =
      Fixture.add(ast::AstNodeKind::CompoundStatement, "CompoundStmt");
  ast::AstNodeMetadata CallMetadata;
  CallMetadata.DoesNotReturn = true;
  const ast::AstNodeId Call =
      Fixture.add(ast::AstNodeKind::CallExpression, "terminate", CallMetadata);
  const ast::AstNodeId Dead =
      Fixture.add(ast::AstNodeKind::ReturnStatement, "dead return");
  Fixture.attach(Fixture.root(), Body, ast::AstEdgeRole::Body);
  Fixture.attach(Body, Call);
  Fixture.attach(Body, Dead);

  ast::FunctionAstGraph AstGraph = Fixture.finish();
  cfg::FunctionCfgBuildResult Result = cfg::buildFunctionCfgGraph(AstGraph);
  const cfg::FunctionCfgGraph &Cfg = expectGraph(Result);
  const std::vector<bool> Reachable = cfg::computeReachableNodes(Cfg);

  bool DeadReturnIsUnreachable = false;
  for (const cfg::CfgGraph::Node &Node : Cfg.graph().nodes()) {
    if (Node.Payload.Metadata.AstOrigin == Dead)
      DeadReturnIsUnreachable = !Reachable[Node.Id.value()];
  }
  MYCPPTOOLS_EXPECT_TRUE(DeadReturnIsUnreachable);
}

static void routesRangeProtocolExceptionsToHandler() {
  AstFixture Fixture;
  const ast::AstNodeId Body =
      Fixture.add(ast::AstNodeKind::CompoundStatement, "CompoundStmt");
  const ast::AstNodeId Try =
      Fixture.add(ast::AstNodeKind::TryStatement, "CXXTryStmt");
  const ast::AstNodeId TryBody =
      Fixture.add(ast::AstNodeKind::CompoundStatement, "try body");
  ast::AstNodeMetadata RangeForMetadata;
  RangeForMetadata.RangeFor = ast::RangeForSemantics{true, true, true, true};
  const ast::AstNodeId RangeFor = Fixture.add(
      ast::AstNodeKind::RangeForStatement, "CXXForRangeStmt", RangeForMetadata);
  const ast::AstNodeId Range =
      Fixture.add(ast::AstNodeKind::DeclarationReferenceExpression, "range");
  const ast::AstNodeId LoopVariable =
      Fixture.add(ast::AstNodeKind::VariableDeclaration, "loop variable");
  const ast::AstNodeId LoopBody =
      Fixture.add(ast::AstNodeKind::CompoundStatement, "loop body");
  ast::AstNodeMetadata CatchMetadata;
  CatchMetadata.ExceptionType = "...";
  CatchMetadata.IsCatchAll = true;
  const ast::AstNodeId Catch = Fixture.add(ast::AstNodeKind::CatchStatement,
                                           "CXXCatchStmt", CatchMetadata);
  const ast::AstNodeId CatchBody =
      Fixture.add(ast::AstNodeKind::CompoundStatement, "catch body");
  Fixture.attach(Fixture.root(), Body, ast::AstEdgeRole::Body);
  Fixture.attach(Body, Try);
  Fixture.attach(Try, TryBody, ast::AstEdgeRole::TryBody);
  Fixture.attach(Try, Catch, ast::AstEdgeRole::Handler);
  Fixture.attach(TryBody, RangeFor);
  Fixture.attach(RangeFor, Range, ast::AstEdgeRole::Range);
  Fixture.attach(RangeFor, LoopVariable, ast::AstEdgeRole::LoopVariable);
  Fixture.attach(RangeFor, LoopBody, ast::AstEdgeRole::Body);
  Fixture.attach(Catch, CatchBody, ast::AstEdgeRole::CatchBody);

  ast::FunctionAstGraph AstGraph = Fixture.finish();
  cfg::FunctionCfgBuildResult Result = cfg::buildFunctionCfgGraph(AstGraph);
  const cfg::FunctionCfgGraph &Cfg = expectGraph(Result);

  std::size_t RangeExceptionEdges = 0;
  for (const cfg::CfgGraph::Edge &Edge : Cfg.graph().edges()) {
    const std::string &Source =
        Cfg.graph().node(Edge.Source).Payload.Display.Label;
    const std::string &Target =
        Cfg.graph().node(Edge.Target).Payload.Display.Label;
    if ((Source == "initialize range" || Source == "range has next" ||
         Source == "read range element" || Source == "advance range") &&
        Target == "exception dispatch" &&
        Edge.Payload.Kind == cfg::CfgEdgeKind::ExceptionBranch)
      ++RangeExceptionEdges;
  }
  MYCPPTOOLS_EXPECT_EQ(RangeExceptionEdges, std::size_t(4));
}

int main() {
  return mycpptools::test::runTests({
      {"builds a short-circuit return", buildsShortCircuitReturn},
      {"resolves goto and keeps unreachable code",
       resolvesGotoAndKeepsUnreachableCode},
      {"reports an invalid AST transactionally",
       reportsInvalidAstTransactionally},
      {"builds a nested throw as an exceptional transfer",
       buildsNestedThrowAsExceptionalTransfer},
      {"builds try/catch from semantic AST roles",
       buildsTryCatchFromSemanticRoles},
      {"routes potentially throwing calls to handler dispatch",
       routesPotentiallyThrowingCallsToHandlerDispatch},
      {"evaluates an assignment condition right before left",
       evaluatesAssignmentConditionRightBeforeLeft},
      {"rethrows after constructor and destructor function-try-block handlers",
       rethrowsAfterConstructorAndDestructorFunctionTryHandlers},
      {"reports an unresolved goto target", reportsUnresolvedGotoTarget},
      {"makes code after a noreturn call unreachable",
       makesCodeAfterNoreturnCallUnreachable},
      {"routes range protocol exceptions to the nearest handler",
       routesRangeProtocolExceptionsToHandler},
  });
}
