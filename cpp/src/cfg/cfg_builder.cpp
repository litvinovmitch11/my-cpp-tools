#include "mycpptools/cfg/cfg_builder.hpp"

#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace mycpptools::cfg {

namespace {

struct BuildFailure {
  CfgBuildError Error;
};

struct FragmentExit {
  FragmentExit(CfgNodeId Source) : Source(Source) {}

  FragmentExit(CfgNodeId Source, CfgEdgeKind Kind,
               std::optional<ast::AstNodeId> AstOrigin)
      : Source(Source), Kind(Kind), AstOrigin(AstOrigin) {}

  CfgNodeId Source;
  CfgEdgeKind Kind = CfgEdgeKind::Flow;
  std::optional<ast::AstNodeId> AstOrigin;
};

struct FlowFragment {
  CfgNodeId Entry;
  std::vector<FragmentExit> Exits;
};

struct ConditionFragment {
  CfgNodeId Entry;
  std::vector<CfgNodeId> TrueExits;
  std::vector<CfgNodeId> FalseExits;
};

struct SwitchTarget {
  CfgNodeId Node;
  CfgEdgeKind Kind;
  ast::AstNodeId Origin;
  std::string Label;
};

struct PendingLabelBranch {
  CfgNodeId Node;
  ast::AstNodeId Origin;
  std::string Label;
  CfgEdgeKind Kind;
};

class FunctionCfgBuilder {
public:
  explicit FunctionCfgBuilder(const ast::FunctionAstGraph &FunctionGraph)
      : FunctionGraph(FunctionGraph), Ast(FunctionGraph.graph()) {
    Entry = addSyntheticNode(CfgNodeKind::Entry, "Entry");
    Exit = addSyntheticNode(CfgNodeKind::Exit, "Exit");
    Graph.setRoot(Entry);

    collectFunctionLabels(*Ast.root());
  }

  FunctionCfgGraph build() {
    const ast::AstNodeId AstRoot = *Ast.root();
    const std::vector<ast::AstNodeId> Initializers =
        children(AstRoot, ast::AstEdgeRole::Initializer);
    const std::optional<ast::AstNodeId> Body =
        child(AstRoot, ast::AstEdgeRole::Body);

    if (!Body) {
      if (Initializers.empty()) {
        addEdge(Entry, Exit, CfgEdgeKind::Flow);
      } else {
        FlowFragment InitializerFlow = buildSequence(Initializers, AstRoot);
        addEdge(Entry, InitializerFlow.Entry, CfgEdgeKind::Flow);
        connect(InitializerFlow.Exits, Exit);
      }
    } else {
      const bool IsFunctionTryBlock =
          Ast.node(*Body).Payload.Kind == ast::AstNodeKind::TryStatement;
      const ast::FunctionKind FunctionKind =
          Ast.node(AstRoot).Payload.Metadata.FunctionCategory;
      const bool HandlerCompletionRethrows =
          FunctionKind == ast::FunctionKind::Constructor ||
          FunctionKind == ast::FunctionKind::Destructor;
      FlowFragment BodyFlow =
          IsFunctionTryBlock
              ? buildTry(*Body, Initializers, HandlerCompletionRethrows)
              : buildNode(*Body);
      if (!Initializers.empty() && !IsFunctionTryBlock) {
        FlowFragment InitializerFlow = buildSequence(Initializers, AstRoot);
        addEdge(Entry, InitializerFlow.Entry, CfgEdgeKind::Flow);
        connect(InitializerFlow.Exits, BodyFlow.Entry);
      } else {
        addEdge(Entry, BodyFlow.Entry, CfgEdgeKind::Flow);
      }
      connect(BodyFlow.Exits, Exit);
    }

    resolveLabelBranches();
    return FunctionCfgGraph(FunctionGraph.function(), std::move(Graph), Exit);
  }

private:
  void collectFunctionLabels(ast::AstNodeId Node) {
    const ast::AstNode &Payload = Ast.node(Node).Payload;
    if (Payload.Kind == ast::AstNodeKind::LabelStatement &&
        Payload.Metadata.Name)
      addUniqueLabel(AllLabels, *Payload.Metadata.Name);
    if (Payload.Kind == ast::AstNodeKind::AddressOfLabelExpression &&
        Payload.Metadata.TargetLabel)
      addUniqueLabel(AddressTakenLabels, *Payload.Metadata.TargetLabel);

    for (const ast::AstEdgeId EdgeId : Ast.outgoingEdges(Node)) {
      const ast::AstGraph::Edge &Edge = Ast.edge(EdgeId);
      if (Payload.Kind == ast::AstNodeKind::LambdaExpression &&
          Edge.Payload.Role == ast::AstEdgeRole::LambdaBody)
        continue;
      collectFunctionLabels(Edge.Target);
    }
  }

  static void addUniqueLabel(std::vector<std::string> &Labels,
                             const std::string &Label) {
    for (const std::string &Existing : Labels) {
      if (Existing == Label)
        return;
    }
    Labels.push_back(Label);
  }

  [[noreturn]] void fail(CfgBuildErrorCode Code, ast::AstNodeId Node,
                         std::string Message) const {
    throw BuildFailure{CfgBuildError{Code, FunctionGraph.function().Id, Node,
                                     std::move(Message)}};
  }

  [[nodiscard]] CfgNodeId addSyntheticNode(CfgNodeKind Kind,
                                           std::string Label) {
    return Graph.addNode(CfgNode{Kind, {}, CfgNodeDisplay{std::move(Label)}});
  }

  [[nodiscard]] CfgNodeId
  addAstNode(CfgNodeKind Kind, ast::AstNodeId Origin, std::string Label,
             std::optional<CfgTransferKind> Transfer = std::nullopt,
             bool ModelExceptionalFlow = true) {
    const ast::AstNode &AstNode = Ast.node(Origin).Payload;
    CfgNodeMetadata Metadata;
    Metadata.AstOrigin = Origin;
    Metadata.Transfer = Transfer;
    Metadata.Source = AstNode.Metadata.Source;
    const CfgNodeId Result = Graph.addNode(
        CfgNode{Kind, std::move(Metadata), CfgNodeDisplay{std::move(Label)}});
    if (ModelExceptionalFlow && AstNode.Metadata.MayThrow &&
        (Kind == CfgNodeKind::Operation || Kind == CfgNodeKind::Condition))
      addEdge(Result, exceptionTarget(), CfgEdgeKind::ExceptionBranch, Origin,
              "throws");
    return Result;
  }

  [[nodiscard]] CfgNodeId addSemanticNode(CfgNodeKind Kind,
                                          ast::AstNodeId Origin,
                                          std::string Label, bool MayThrow) {
    const CfgNodeId Result =
        addAstNode(Kind, Origin, std::move(Label), std::nullopt, false);
    if (MayThrow)
      addEdge(Result, exceptionTarget(), CfgEdgeKind::ExceptionBranch, Origin,
              "throws");
    return Result;
  }

  void addEdge(CfgNodeId Source, CfgNodeId Target, CfgEdgeKind Kind,
               std::optional<ast::AstNodeId> Origin = std::nullopt,
               std::optional<std::string> Label = std::nullopt) {
    (void)Graph.addEdge(Source, Target,
                        CfgEdge{Kind, Origin, std::move(Label)});
  }

  void connect(const std::vector<FragmentExit> &Exits, CfgNodeId Target) {
    for (const FragmentExit &ExitEdge : Exits)
      addEdge(ExitEdge.Source, Target, ExitEdge.Kind, ExitEdge.AstOrigin);
  }

  void connect(const std::vector<CfgNodeId> &Sources, CfgNodeId Target,
               CfgEdgeKind Kind) {
    for (const CfgNodeId Source : Sources)
      addEdge(Source, Target, Kind);
  }

  [[nodiscard]] std::vector<ast::AstNodeId>
  children(ast::AstNodeId Parent) const {
    std::vector<ast::AstNodeId> Result;
    for (const ast::AstEdgeId EdgeId : Ast.outgoingEdges(Parent))
      Result.push_back(Ast.edge(EdgeId).Target);
    return Result;
  }

  [[nodiscard]] std::vector<ast::AstNodeId>
  children(ast::AstNodeId Parent, ast::AstEdgeRole Role) const {
    std::vector<ast::AstNodeId> Result;
    for (const ast::AstEdgeId EdgeId : Ast.outgoingEdges(Parent)) {
      const ast::AstGraph::Edge &Edge = Ast.edge(EdgeId);
      if (Edge.Payload.Role == Role)
        Result.push_back(Edge.Target);
    }
    return Result;
  }

  [[nodiscard]] std::optional<ast::AstNodeId>
  child(ast::AstNodeId Parent, ast::AstEdgeRole Role) const {
    std::optional<ast::AstNodeId> Result;
    for (const ast::AstEdgeId EdgeId : Ast.outgoingEdges(Parent)) {
      const ast::AstGraph::Edge &Edge = Ast.edge(EdgeId);
      if (Edge.Payload.Role != Role)
        continue;
      if (Result)
        fail(CfgBuildErrorCode::InvalidAst, Parent,
             "multiple AST children have the singular role " +
                 std::string(ast::toString(Role)));
      Result = Edge.Target;
    }
    return Result;
  }

  [[nodiscard]] FlowFragment makeOperation(ast::AstNodeId Node) {
    const CfgNodeId Operation = addAstNode(
        CfgNodeKind::Operation, Node, Ast.node(Node).Payload.Display.Label);
    if (Ast.node(Node).Payload.Metadata.DoesNotReturn)
      return FlowFragment{Operation, {}};
    return FlowFragment{Operation, {{Operation}}};
  }

  [[nodiscard]] FlowFragment makeEmptyAnchor(ast::AstNodeId Origin,
                                             std::string Label = "empty") {
    const CfgNodeId Anchor =
        addAstNode(CfgNodeKind::Anchor, Origin, std::move(Label));
    return FlowFragment{Anchor, {{Anchor}}};
  }

  [[nodiscard]] FlowFragment buildLambda(ast::AstNodeId Node) {
    std::vector<ast::AstNodeId> CaptureInitializers;
    for (const ast::AstEdgeId EdgeId : Ast.outgoingEdges(Node)) {
      const ast::AstGraph::Edge &Edge = Ast.edge(EdgeId);
      if (Edge.Payload.Role != ast::AstEdgeRole::LambdaBody &&
          containsControlExpression(Edge.Target))
        CaptureInitializers.push_back(Edge.Target);
    }

    FlowFragment Lambda = makeOperation(Node);
    if (CaptureInitializers.empty())
      return Lambda;
    FlowFragment Captures =
        buildControlExpressionsInAstOrder(CaptureInitializers, Node);
    connect(Captures.Exits, Lambda.Entry);
    Lambda.Entry = Captures.Entry;
    return Lambda;
  }

  [[nodiscard]] FlowFragment
  buildSequence(const std::vector<ast::AstNodeId> &Nodes,
                ast::AstNodeId EmptyOrigin) {
    if (Nodes.empty())
      return makeEmptyAnchor(EmptyOrigin);

    FlowFragment Result = buildNode(Nodes.front());
    for (std::size_t Index = 1; Index < Nodes.size(); ++Index) {
      FlowFragment Next = buildNode(Nodes[Index]);
      connect(Result.Exits, Next.Entry);
      Result.Exits = std::move(Next.Exits);
    }
    return Result;
  }

  [[nodiscard]] FlowFragment
  buildControlExpressionsInAstOrder(const std::vector<ast::AstNodeId> &Nodes,
                                    ast::AstNodeId Origin) {
    if (Nodes.empty())
      return makeEmptyAnchor(Origin, "no control expression");
    FlowFragment Result = buildNode(Nodes.front());
    for (std::size_t Index = 1; Index < Nodes.size(); ++Index) {
      FlowFragment Next = buildNode(Nodes[Index]);
      connect(Result.Exits, Next.Entry);
      Result.Exits = std::move(Next.Exits);
    }
    return Result;
  }

  [[nodiscard]] FlowFragment buildNode(ast::AstNodeId Node) {
    const ast::AstNode &Payload = Ast.node(Node).Payload;

    switch (Payload.Kind) {
    case ast::AstNodeKind::CompoundStatement:
      return buildSequence(children(Node), Node);
    case ast::AstNodeKind::DeclarationStatement:
      return buildSequence(children(Node, ast::AstEdgeRole::Declaration), Node);
    case ast::AstNodeKind::VariableDeclaration:
      return buildVariable(Node);
    case ast::AstNodeKind::IfStatement:
    case ast::AstNodeKind::ConstexprIfStatement:
      return buildIf(Node);
    case ast::AstNodeKind::SwitchStatement:
      return buildSwitch(Node);
    case ast::AstNodeKind::CaseStatement:
      return buildCase(Node, false);
    case ast::AstNodeKind::DefaultStatement:
      return buildCase(Node, true);
    case ast::AstNodeKind::ForStatement:
      return buildFor(Node);
    case ast::AstNodeKind::RangeForStatement:
      return buildRangeFor(Node);
    case ast::AstNodeKind::WhileStatement:
      return buildWhile(Node);
    case ast::AstNodeKind::DoStatement:
      return buildDo(Node);
    case ast::AstNodeKind::ReturnStatement:
      return buildReturn(Node);
    case ast::AstNodeKind::BreakStatement:
      return buildBreak(Node);
    case ast::AstNodeKind::ContinueStatement:
      return buildContinue(Node);
    case ast::AstNodeKind::GotoStatement:
      return buildGoto(Node);
    case ast::AstNodeKind::IndirectGotoStatement:
      return buildIndirectGoto(Node);
    case ast::AstNodeKind::LabelStatement:
      return buildLabel(Node);
    case ast::AstNodeKind::TryStatement:
      return buildTry(Node);
    case ast::AstNodeKind::CatchStatement:
      return buildCatch(Node);
    case ast::AstNodeKind::ThrowExpression:
      return buildThrow(Node);
    case ast::AstNodeKind::CoroutineBodyStatement:
      return buildCoroutineBody(Node);
    case ast::AstNodeKind::CoReturnStatement:
      return buildCoReturn(Node);
    case ast::AstNodeKind::CoAwaitExpression:
      return buildCoroutineSuspend(Node, "co_await");
    case ast::AstNodeKind::CoYieldExpression:
      return buildCoroutineSuspend(Node, "co_yield");
    case ast::AstNodeKind::AsmStatement:
      return buildAsm(Node);
    case ast::AstNodeKind::LambdaExpression:
      return buildLambda(Node);
    case ast::AstNodeKind::OtherStatement:
      return buildOtherStatement(Node);
    default:
      break;
    }

    if (isExpression(Payload.Kind))
      return buildExpression(Node);
    return makeOperation(Node);
  }

  [[nodiscard]] static bool isExpression(ast::AstNodeKind Kind) {
    switch (Kind) {
    case ast::AstNodeKind::LambdaExpression:
    case ast::AstNodeKind::ThrowExpression:
    case ast::AstNodeKind::CoAwaitExpression:
    case ast::AstNodeKind::CoYieldExpression:
    case ast::AstNodeKind::CallExpression:
    case ast::AstNodeKind::BinaryExpression:
    case ast::AstNodeKind::UnaryExpression:
    case ast::AstNodeKind::ConditionalExpression:
    case ast::AstNodeKind::CompileTimeConditionalExpression:
    case ast::AstNodeKind::DeclarationReferenceExpression:
    case ast::AstNodeKind::AddressOfLabelExpression:
    case ast::AstNodeKind::MemberExpression:
    case ast::AstNodeKind::CastExpression:
    case ast::AstNodeKind::LiteralExpression:
    case ast::AstNodeKind::OtherExpression:
      return true;
    default:
      return false;
    }
  }

  [[nodiscard]] FlowFragment buildVariable(ast::AstNodeId Node) {
    const CfgNodeId Declaration = addAstNode(
        CfgNodeKind::Operation, Node, Ast.node(Node).Payload.Display.Label);
    const std::optional<ast::AstNodeId> Initializer =
        child(Node, ast::AstEdgeRole::Initializer);
    if (!Initializer)
      return FlowFragment{Declaration, {{Declaration}}};

    FlowFragment Value = buildExpression(*Initializer);
    connect(Value.Exits, Declaration);
    return FlowFragment{Value.Entry, {{Declaration}}};
  }

  [[nodiscard]] FlowFragment buildReturn(ast::AstNodeId Node) {
    const CfgNodeId Transfer = addAstNode(CfgNodeKind::Transfer, Node, "return",
                                          CfgTransferKind::Return);
    addEdge(Transfer, Exit, CfgEdgeKind::Flow);

    const std::optional<ast::AstNodeId> Value =
        child(Node, ast::AstEdgeRole::ReturnValue);
    if (!Value)
      return FlowFragment{Transfer, {}};

    FlowFragment ValueFlow = buildExpression(*Value);
    connect(ValueFlow.Exits, Transfer);
    return FlowFragment{ValueFlow.Entry, {}};
  }

  [[nodiscard]] FlowFragment buildBreak(ast::AstNodeId Node) {
    if (BreakTargets.empty())
      fail(CfgBuildErrorCode::InvalidAst, Node,
           "break statement has no enclosing loop or switch");
    const CfgNodeId Transfer = addAstNode(CfgNodeKind::Transfer, Node, "break",
                                          CfgTransferKind::Break);
    addEdge(Transfer, BreakTargets.back(), CfgEdgeKind::Flow);
    return FlowFragment{Transfer, {}};
  }

  [[nodiscard]] FlowFragment buildContinue(ast::AstNodeId Node) {
    if (ContinueTargets.empty())
      fail(CfgBuildErrorCode::InvalidAst, Node,
           "continue statement has no enclosing loop");
    const CfgNodeId Transfer = addAstNode(
        CfgNodeKind::Transfer, Node, "continue", CfgTransferKind::Continue);
    addEdge(Transfer, ContinueTargets.back(), CfgEdgeKind::Flow);
    return FlowFragment{Transfer, {}};
  }

  [[nodiscard]] FlowFragment buildGoto(ast::AstNodeId Node) {
    const std::optional<std::string> &Target =
        Ast.node(Node).Payload.Metadata.TargetLabel;
    if (!Target)
      fail(CfgBuildErrorCode::InvalidAst, Node,
           "goto statement has no target label metadata");

    const CfgNodeId Transfer = addAstNode(
        CfgNodeKind::Transfer, Node, "goto " + *Target, CfgTransferKind::Goto);
    const auto Known = Labels.find(*Target);
    if (Known != Labels.end())
      addEdge(Transfer, Known->second, CfgEdgeKind::Flow);
    else
      PendingLabelBranches.push_back(
          PendingLabelBranch{Transfer, Node, *Target, CfgEdgeKind::Flow});
    return FlowFragment{Transfer, {}};
  }

  [[nodiscard]] FlowFragment buildIndirectGoto(ast::AstNodeId Node) {
    const CfgNodeId Transfer =
        addAstNode(CfgNodeKind::Transfer, Node, "indirect goto",
                   CfgTransferKind::IndirectGoto);

    const std::vector<std::string> &Targets =
        AddressTakenLabels.empty() ? AllLabels : AddressTakenLabels;
    if (Targets.empty()) {
      addEdge(Transfer, Exit, CfgEdgeKind::IndirectBranch, Node, "unknown");
    } else {
      for (const std::string &Target : Targets) {
        const auto Known = Labels.find(Target);
        if (Known != Labels.end()) {
          addEdge(Transfer, Known->second, CfgEdgeKind::IndirectBranch, Node,
                  Target);
        } else {
          PendingLabelBranches.push_back(PendingLabelBranch{
              Transfer, Node, Target, CfgEdgeKind::IndirectBranch});
        }
      }
    }

    const auto Operand = child(Node, ast::AstEdgeRole::Operand);
    if (!Operand)
      return FlowFragment{Transfer, {}};
    FlowFragment Target = buildExpression(*Operand);
    connect(Target.Exits, Transfer);
    return FlowFragment{Target.Entry, {}};
  }

  [[nodiscard]] FlowFragment buildLabel(ast::AstNodeId Node) {
    const std::optional<std::string> &Name =
        Ast.node(Node).Payload.Metadata.Name;
    if (!Name)
      fail(CfgBuildErrorCode::InvalidAst, Node,
           "label statement has no name metadata");

    const CfgNodeId Anchor = addAstNode(CfgNodeKind::Anchor, Node, *Name + ":");
    if (!Labels.emplace(*Name, Anchor).second)
      fail(CfgBuildErrorCode::InvalidAst, Node, "duplicate label " + *Name);

    const std::optional<ast::AstNodeId> Body =
        child(Node, ast::AstEdgeRole::LabelBody);
    if (!Body)
      return FlowFragment{Anchor, {{Anchor}}};
    FlowFragment BodyFlow = buildNode(*Body);
    addEdge(Anchor, BodyFlow.Entry, CfgEdgeKind::Flow);
    return FlowFragment{Anchor, std::move(BodyFlow.Exits)};
  }

  [[nodiscard]] CfgNodeId exceptionTarget() const {
    return ExceptionTargets.empty() ? Exit : ExceptionTargets.back();
  }

  [[nodiscard]] FlowFragment buildThrow(ast::AstNodeId Node) {
    const CfgNodeId Transfer = addAstNode(CfgNodeKind::Transfer, Node, "throw",
                                          CfgTransferKind::Throw);
    addEdge(Transfer, exceptionTarget(), CfgEdgeKind::ExceptionBranch, Node);

    const auto Value = child(Node, ast::AstEdgeRole::ThrownValue);
    if (!Value)
      return FlowFragment{Transfer, {}};
    FlowFragment ValueFlow = buildExpression(*Value);
    connect(ValueFlow.Exits, Transfer);
    return FlowFragment{ValueFlow.Entry, {}};
  }

  [[nodiscard]] FlowFragment buildCatch(ast::AstNodeId Node) {
    const ast::AstNodeMetadata &Metadata = Ast.node(Node).Payload.Metadata;
    const std::string Type = Metadata.ExceptionType.value_or("?");
    const CfgNodeId Anchor =
        addAstNode(CfgNodeKind::Anchor, Node, "catch " + Type);

    std::vector<ast::AstNodeId> Contents =
        children(Node, ast::AstEdgeRole::ExceptionDeclaration);
    if (const auto Body = child(Node, ast::AstEdgeRole::CatchBody))
      Contents.push_back(*Body);
    if (Contents.empty())
      return FlowFragment{Anchor, {{Anchor}}};

    FlowFragment Content = buildSequence(Contents, Node);
    addEdge(Anchor, Content.Entry, CfgEdgeKind::Flow);
    return FlowFragment{Anchor, std::move(Content.Exits)};
  }

  [[nodiscard]] FlowFragment
  buildTry(ast::AstNodeId Node,
           const std::vector<ast::AstNodeId> &ProtectedPrefix = {},
           bool HandlerCompletionRethrows = false) {
    const auto BodyNode = child(Node, ast::AstEdgeRole::TryBody);
    const std::vector<ast::AstNodeId> Handlers =
        children(Node, ast::AstEdgeRole::Handler);
    if (!BodyNode || Handlers.empty())
      fail(CfgBuildErrorCode::InvalidAst, Node,
           "try statement is missing its body or handlers");

    const CfgNodeId OuterExceptionTarget = exceptionTarget();
    const CfgNodeId Dispatch =
        addAstNode(CfgNodeKind::Condition, Node, "exception dispatch");

    std::vector<FlowFragment> HandlerFlows;
    HandlerFlows.reserve(Handlers.size());
    bool HasCatchAll = false;
    for (const ast::AstNodeId Handler : Handlers) {
      HandlerFlows.push_back(buildCatch(Handler));
      const ast::AstNodeMetadata &Metadata = Ast.node(Handler).Payload.Metadata;
      if (HandlerCompletionRethrows && !HandlerFlows.back().Exits.empty()) {
        const CfgNodeId Rethrow =
            addAstNode(CfgNodeKind::Transfer, Handler, "implicit rethrow",
                       CfgTransferKind::Throw);
        connect(HandlerFlows.back().Exits, Rethrow);
        addEdge(Rethrow, OuterExceptionTarget, CfgEdgeKind::ExceptionBranch,
                Handler, "rethrow");
        HandlerFlows.back().Exits.clear();
      }
      HasCatchAll = HasCatchAll || Metadata.IsCatchAll;
      addEdge(Dispatch, HandlerFlows.back().Entry, CfgEdgeKind::ExceptionBranch,
              Handler, "catch " + Metadata.ExceptionType.value_or("?"));
    }
    if (!HasCatchAll)
      addEdge(Dispatch, OuterExceptionTarget, CfgEdgeKind::ExceptionBranch,
              Node, "uncaught");

    ExceptionTargets.push_back(Dispatch);
    std::optional<FlowFragment> Prefix;
    if (!ProtectedPrefix.empty())
      Prefix = buildSequence(ProtectedPrefix, Node);
    FlowFragment Body = buildNode(*BodyNode);
    ExceptionTargets.pop_back();
    if (Prefix) {
      connect(Prefix->Exits, Body.Entry);
      Body.Entry = Prefix->Entry;
    }
    for (FlowFragment &Handler : HandlerFlows) {
      Body.Exits.insert(Body.Exits.end(), Handler.Exits.begin(),
                        Handler.Exits.end());
    }
    return Body;
  }

  [[nodiscard]] FlowFragment buildCoroutineBody(ast::AstNodeId Node) {
    const auto Body = child(Node, ast::AstEdgeRole::Body);
    if (!Body)
      fail(CfgBuildErrorCode::InvalidAst, Node,
           "coroutine body is missing its user-written body");
    return buildNode(*Body);
  }

  [[nodiscard]] FlowFragment buildCoReturn(ast::AstNodeId Node) {
    const CfgNodeId Transfer = addAstNode(
        CfgNodeKind::Transfer, Node, "co_return", CfgTransferKind::CoReturn);
    addEdge(Transfer, Exit, CfgEdgeKind::Flow);

    const auto Value = child(Node, ast::AstEdgeRole::ReturnValue);
    if (!Value)
      return FlowFragment{Transfer, {}};
    FlowFragment ValueFlow = buildExpression(*Value);
    connect(ValueFlow.Exits, Transfer);
    return FlowFragment{ValueFlow.Entry, {}};
  }

  [[nodiscard]] FlowFragment buildCoroutineSuspend(ast::AstNodeId Node,
                                                   std::string Label) {
    const CfgNodeId Decision =
        addAstNode(CfgNodeKind::Condition, Node, std::move(Label));
    const CfgNodeId Suspended =
        addSyntheticNode(CfgNodeKind::Anchor, "suspended");
    const CfgNodeId After =
        addSyntheticNode(CfgNodeKind::Anchor, "after suspension");
    addEdge(Decision, After, CfgEdgeKind::TrueBranch, Node, "ready");
    addEdge(Decision, Suspended, CfgEdgeKind::Suspend, Node);
    addEdge(Suspended, After, CfgEdgeKind::Resume, Node);

    const auto Operand = child(Node, ast::AstEdgeRole::Operand);
    if (!Operand)
      return FlowFragment{Decision, {{After}}};
    FlowFragment OperandFlow = buildExpression(*Operand);
    connect(OperandFlow.Exits, Decision);
    return FlowFragment{OperandFlow.Entry, {{After}}};
  }

  [[nodiscard]] FlowFragment buildAsm(ast::AstNodeId Node) {
    const ast::AstNodeMetadata &Metadata = Ast.node(Node).Payload.Metadata;
    std::vector<ast::AstNodeId> ControlChildren;
    for (const ast::AstNodeId Child : children(Node)) {
      if (containsControlExpression(Child))
        ControlChildren.push_back(Child);
    }

    std::optional<FlowFragment> Prefix;
    if (!ControlChildren.empty())
      Prefix = buildControlExpressionsInAstOrder(ControlChildren, Node);

    if (!Metadata.IsAsmGoto) {
      FlowFragment Operation = makeOperation(Node);
      if (Prefix) {
        connect(Prefix->Exits, Operation.Entry);
        Operation.Entry = Prefix->Entry;
      }
      return Operation;
    }
    if (Metadata.TargetLabels.empty())
      fail(CfgBuildErrorCode::InvalidAst, Node,
           "asm goto statement has no target labels");

    const CfgNodeId Branch =
        addAstNode(CfgNodeKind::Condition, Node, "asm goto");
    for (const std::string &Target : Metadata.TargetLabels) {
      const auto Known = Labels.find(Target);
      if (Known != Labels.end()) {
        addEdge(Branch, Known->second, CfgEdgeKind::AsmBranch, Node, Target);
      } else {
        PendingLabelBranches.push_back(
            PendingLabelBranch{Branch, Node, Target, CfgEdgeKind::AsmBranch});
      }
    }

    FlowFragment Result{Branch,
                        {FragmentExit{Branch, CfgEdgeKind::Fallthrough, Node}}};
    if (Prefix) {
      connect(Prefix->Exits, Branch);
      Result.Entry = Prefix->Entry;
    }
    return Result;
  }

  [[nodiscard]] FlowFragment buildOtherStatement(ast::AstNodeId Node) {
    const std::vector<ast::AstNodeId> Contents = children(Node);
    if (Contents.empty())
      return makeOperation(Node);
    return buildSequence(Contents, Node);
  }

  [[nodiscard]] FlowFragment buildIf(ast::AstNodeId Node) {
    const ast::AstNode &Payload = Ast.node(Node).Payload;
    const std::vector<ast::AstNodeId> Initializers =
        children(Node, ast::AstEdgeRole::Initializer);
    std::optional<FlowFragment> Initializer;
    if (!Initializers.empty())
      Initializer = buildSequence(Initializers, Node);

    const std::optional<ast::AstNodeId> Then =
        child(Node, ast::AstEdgeRole::ThenBranch);
    const std::optional<ast::AstNodeId> Else =
        child(Node, ast::AstEdgeRole::ElseBranch);

    if (Payload.Kind == ast::AstNodeKind::ConstexprIfStatement &&
        Payload.Metadata.ConstexprSelection &&
        *Payload.Metadata.ConstexprSelection !=
            ast::ConstexprBranchSelection::Dependent) {
      const CfgNodeId Marker =
          addAstNode(CfgNodeKind::Operation, Node,
                     *Payload.Metadata.ConstexprSelection ==
                             ast::ConstexprBranchSelection::Then
                         ? "if constexpr [then]"
                         : "if constexpr [else]");
      FlowFragment Result{Marker, {{Marker}}};
      const std::optional<ast::AstNodeId> Selected =
          *Payload.Metadata.ConstexprSelection ==
                  ast::ConstexprBranchSelection::Then
              ? Then
              : Else;
      if (Selected) {
        FlowFragment Branch = buildNode(*Selected);
        connect(Result.Exits, Branch.Entry);
        Result.Exits = std::move(Branch.Exits);
      }
      if (Initializer) {
        connect(Initializer->Exits, Result.Entry);
        Result.Entry = Initializer->Entry;
      }
      return Result;
    }

    ConditionFragment Condition = buildParentCondition(Node);
    FlowFragment ThenFlow = Then ? buildNode(*Then) : makeEmptyAnchor(Node);
    connect(Condition.TrueExits, ThenFlow.Entry, CfgEdgeKind::TrueBranch);

    std::vector<FragmentExit> Exits = std::move(ThenFlow.Exits);
    if (Else) {
      FlowFragment ElseFlow = buildNode(*Else);
      connect(Condition.FalseExits, ElseFlow.Entry, CfgEdgeKind::FalseBranch);
      Exits.insert(Exits.end(), ElseFlow.Exits.begin(), ElseFlow.Exits.end());
    } else {
      for (const CfgNodeId FalseExit : Condition.FalseExits)
        Exits.push_back(
            FragmentExit{FalseExit, CfgEdgeKind::FalseBranch, Node});
    }

    FlowFragment Result{Condition.Entry, std::move(Exits)};
    if (Initializer) {
      connect(Initializer->Exits, Result.Entry);
      Result.Entry = Initializer->Entry;
    }
    return Result;
  }

  [[nodiscard]] ConditionFragment buildParentCondition(ast::AstNodeId Parent) {
    const std::vector<ast::AstNodeId> Conditions =
        children(Parent, ast::AstEdgeRole::Condition);
    if (Conditions.empty())
      fail(CfgBuildErrorCode::InvalidAst, Parent,
           "control statement has no condition");

    ConditionFragment Result = buildCondition(Conditions.back());
    if (Conditions.size() == 1)
      return Result;

    std::vector<ast::AstNodeId> Prefix(Conditions.begin(),
                                       Conditions.end() - 1);
    FlowFragment PrefixFlow = buildSequence(Prefix, Parent);
    connect(PrefixFlow.Exits, Result.Entry);
    Result.Entry = PrefixFlow.Entry;
    return Result;
  }

  [[nodiscard]] ConditionFragment
  buildAtomicCondition(ast::AstNodeId Node,
                       const std::vector<ast::AstNodeId> &ControlChildren) {
    const CfgNodeId Condition = addAstNode(
        CfgNodeKind::Condition, Node, Ast.node(Node).Payload.Display.Label);
    if (ControlChildren.empty())
      return ConditionFragment{Condition, {Condition}, {Condition}};

    FlowFragment Prefix =
        buildControlExpressionsInAstOrder(ControlChildren, Node);
    connect(Prefix.Exits, Condition);
    return ConditionFragment{Prefix.Entry, {Condition}, {Condition}};
  }

  [[nodiscard]] std::vector<ast::AstNodeId>
  assignmentControlOperands(ast::AstNodeId Node) {
    const auto Left = child(Node, ast::AstEdgeRole::LeftOperand);
    const auto Right = child(Node, ast::AstEdgeRole::RightOperand);
    if (!Left || !Right)
      fail(CfgBuildErrorCode::InvalidAst, Node,
           "assignment expression is missing an operand");

    std::vector<ast::AstNodeId> Result;
    if (containsControlExpression(*Right))
      Result.push_back(*Right);
    if (containsControlExpression(*Left))
      Result.push_back(*Left);
    return Result;
  }

  [[nodiscard]] ConditionFragment buildCondition(ast::AstNodeId Node) {
    const ast::AstNode &Payload = Ast.node(Node).Payload;

    if (Payload.Kind == ast::AstNodeKind::LambdaExpression) {
      FlowFragment Lambda = buildLambda(Node);
      const CfgNodeId Condition = addAstNode(
          CfgNodeKind::Condition, Node, Ast.node(Node).Payload.Display.Label);
      connect(Lambda.Exits, Condition);
      return ConditionFragment{Lambda.Entry, {Condition}, {Condition}};
    }
    if (Payload.Metadata.ChildrenAreUnevaluated) {
      const CfgNodeId Condition = addAstNode(
          CfgNodeKind::Condition, Node, Ast.node(Node).Payload.Display.Label);
      return ConditionFragment{Condition, {Condition}, {Condition}};
    }

    if (Payload.Kind == ast::AstNodeKind::BinaryExpression &&
        Payload.Metadata.Operator) {
      const std::string &Operator = *Payload.Metadata.Operator;
      if (Operator == "&&" || Operator == "||") {
        const std::optional<ast::AstNodeId> Left =
            child(Node, ast::AstEdgeRole::LeftOperand);
        const std::optional<ast::AstNodeId> Right =
            child(Node, ast::AstEdgeRole::RightOperand);
        if (!Left || !Right)
          fail(CfgBuildErrorCode::InvalidAst, Node,
               "short-circuit expression is missing an operand");
        ConditionFragment LeftFlow = buildCondition(*Left);
        ConditionFragment RightFlow = buildCondition(*Right);
        if (Operator == "&&") {
          connect(LeftFlow.TrueExits, RightFlow.Entry, CfgEdgeKind::TrueBranch);
          LeftFlow.TrueExits = std::move(RightFlow.TrueExits);
          LeftFlow.FalseExits.insert(LeftFlow.FalseExits.end(),
                                     RightFlow.FalseExits.begin(),
                                     RightFlow.FalseExits.end());
        } else {
          connect(LeftFlow.FalseExits, RightFlow.Entry,
                  CfgEdgeKind::FalseBranch);
          LeftFlow.FalseExits = std::move(RightFlow.FalseExits);
          LeftFlow.TrueExits.insert(LeftFlow.TrueExits.end(),
                                    RightFlow.TrueExits.begin(),
                                    RightFlow.TrueExits.end());
        }
        return LeftFlow;
      }
      if (isAssignmentOperator(Operator)) {
        return buildAtomicCondition(Node, assignmentControlOperands(Node));
      }
    }

    if (Payload.Kind == ast::AstNodeKind::UnaryExpression &&
        Payload.Metadata.Operator && *Payload.Metadata.Operator == "!") {
      const std::optional<ast::AstNodeId> Operand =
          child(Node, ast::AstEdgeRole::Operand);
      if (!Operand)
        fail(CfgBuildErrorCode::InvalidAst, Node,
             "logical negation is missing its operand");
      ConditionFragment Result = buildCondition(*Operand);
      std::swap(Result.TrueExits, Result.FalseExits);
      return Result;
    }

    if (Payload.Kind == ast::AstNodeKind::ConditionalExpression ||
        Payload.Kind == ast::AstNodeKind::CompileTimeConditionalExpression) {
      const auto ConditionNode = child(Node, ast::AstEdgeRole::Condition);
      const auto TrueNode = child(Node, ast::AstEdgeRole::ThenBranch);
      const auto FalseNode = child(Node, ast::AstEdgeRole::ElseBranch);
      if (!ConditionNode || !TrueNode || !FalseNode)
        fail(CfgBuildErrorCode::InvalidAst, Node,
             "conditional expression is incomplete");
      if (Payload.Kind == ast::AstNodeKind::CompileTimeConditionalExpression &&
          Payload.Metadata.ConstexprSelection &&
          *Payload.Metadata.ConstexprSelection !=
              ast::ConstexprBranchSelection::Dependent) {
        return buildCondition(*Payload.Metadata.ConstexprSelection ==
                                      ast::ConstexprBranchSelection::Then
                                  ? *TrueNode
                                  : *FalseNode);
      }

      ConditionFragment Selector = buildCondition(*ConditionNode);
      ConditionFragment TrueFlow = buildCondition(*TrueNode);
      ConditionFragment FalseFlow = buildCondition(*FalseNode);
      connect(Selector.TrueExits, TrueFlow.Entry, CfgEdgeKind::TrueBranch);
      connect(Selector.FalseExits, FalseFlow.Entry, CfgEdgeKind::FalseBranch);
      TrueFlow.TrueExits.insert(TrueFlow.TrueExits.end(),
                                FalseFlow.TrueExits.begin(),
                                FalseFlow.TrueExits.end());
      TrueFlow.FalseExits.insert(TrueFlow.FalseExits.end(),
                                 FalseFlow.FalseExits.begin(),
                                 FalseFlow.FalseExits.end());
      TrueFlow.Entry = Selector.Entry;
      const std::vector<ast::AstNodeId> Initializers =
          children(Node, ast::AstEdgeRole::Initializer);
      if (!Initializers.empty()) {
        FlowFragment Prefix = buildSequence(Initializers, Node);
        connect(Prefix.Exits, TrueFlow.Entry);
        TrueFlow.Entry = Prefix.Entry;
      }
      return TrueFlow;
    }

    std::vector<ast::AstNodeId> ControlChildren;
    for (const ast::AstNodeId Child : children(Node)) {
      if (containsControlExpression(Child))
        ControlChildren.push_back(Child);
    }
    if (ControlChildren.size() == 1) {
      if (Payload.Kind == ast::AstNodeKind::CastExpression ||
          Payload.Metadata.IsTransparent)
        return buildCondition(ControlChildren.front());

      return buildAtomicCondition(Node, ControlChildren);
    }
    return buildAtomicCondition(Node, ControlChildren);
  }

  [[nodiscard]] FlowFragment buildWhile(ast::AstNodeId Node) {
    ConditionFragment Condition = buildParentCondition(Node);
    const CfgNodeId After =
        addSyntheticNode(CfgNodeKind::Anchor, "after while");

    BreakTargets.push_back(After);
    ContinueTargets.push_back(Condition.Entry);
    const auto BodyNode = child(Node, ast::AstEdgeRole::Body);
    FlowFragment Body = BodyNode ? buildNode(*BodyNode) : makeEmptyAnchor(Node);
    ContinueTargets.pop_back();
    BreakTargets.pop_back();

    connect(Condition.TrueExits, Body.Entry, CfgEdgeKind::TrueBranch);
    connect(Condition.FalseExits, After, CfgEdgeKind::FalseBranch);
    connect(Body.Exits, Condition.Entry);
    return FlowFragment{Condition.Entry, {{After}}};
  }

  [[nodiscard]] FlowFragment buildDo(ast::AstNodeId Node) {
    ConditionFragment Condition = buildParentCondition(Node);
    const CfgNodeId After = addSyntheticNode(CfgNodeKind::Anchor, "after do");

    BreakTargets.push_back(After);
    ContinueTargets.push_back(Condition.Entry);
    const auto BodyNode = child(Node, ast::AstEdgeRole::Body);
    FlowFragment Body = BodyNode ? buildNode(*BodyNode) : makeEmptyAnchor(Node);
    ContinueTargets.pop_back();
    BreakTargets.pop_back();

    connect(Body.Exits, Condition.Entry);
    connect(Condition.TrueExits, Body.Entry, CfgEdgeKind::TrueBranch);
    connect(Condition.FalseExits, After, CfgEdgeKind::FalseBranch);
    return FlowFragment{Body.Entry, {{After}}};
  }

  [[nodiscard]] FlowFragment buildFor(ast::AstNodeId Node) {
    std::optional<FlowFragment> Initializer;
    const std::vector<ast::AstNodeId> Initializers =
        children(Node, ast::AstEdgeRole::Initializer);
    if (!Initializers.empty())
      Initializer = buildSequence(Initializers, Node);

    std::optional<ConditionFragment> Condition;
    const std::vector<ast::AstNodeId> Conditions =
        children(Node, ast::AstEdgeRole::Condition);
    if (!Conditions.empty())
      Condition = buildParentCondition(Node);
    else {
      const CfgNodeId Always = addAstNode(CfgNodeKind::Condition, Node, "true");
      Condition = ConditionFragment{Always, {Always}, {}};
    }

    const CfgNodeId After = addSyntheticNode(CfgNodeKind::Anchor, "after for");
    std::optional<FlowFragment> Increment;
    if (const auto IncrementNode = child(Node, ast::AstEdgeRole::Increment))
      Increment = buildExpression(*IncrementNode);
    const CfgNodeId ContinueTarget =
        Increment ? Increment->Entry : Condition->Entry;

    BreakTargets.push_back(After);
    ContinueTargets.push_back(ContinueTarget);
    const auto BodyNode = child(Node, ast::AstEdgeRole::Body);
    FlowFragment Body = BodyNode ? buildNode(*BodyNode) : makeEmptyAnchor(Node);
    ContinueTargets.pop_back();
    BreakTargets.pop_back();

    connect(Condition->TrueExits, Body.Entry, CfgEdgeKind::TrueBranch);
    connect(Condition->FalseExits, After, CfgEdgeKind::FalseBranch);
    connect(Body.Exits, ContinueTarget);
    if (Increment)
      connect(Increment->Exits, Condition->Entry);
    if (Initializer)
      connect(Initializer->Exits, Condition->Entry);

    return FlowFragment{Initializer ? Initializer->Entry : Condition->Entry,
                        {{After}}};
  }

  [[nodiscard]] FlowFragment buildRangeFor(ast::AstNodeId Node) {
    const ast::RangeForSemantics Semantics =
        Ast.node(Node).Payload.Metadata.RangeFor.value_or(
            ast::RangeForSemantics{});
    std::optional<FlowFragment> Initializer;
    const std::vector<ast::AstNodeId> Initializers =
        children(Node, ast::AstEdgeRole::Initializer);
    if (!Initializers.empty())
      Initializer = buildSequence(Initializers, Node);

    std::optional<ast::AstNodeId> RangeNode =
        child(Node, ast::AstEdgeRole::Range);
    if (!RangeNode) {
      const std::vector<ast::AstNodeId> Unclassified =
          children(Node, ast::AstEdgeRole::Child);
      if (Unclassified.size() == 1)
        RangeNode = Unclassified.front();
    }
    const auto LoopVariableNode = child(Node, ast::AstEdgeRole::LoopVariable);
    const auto BodyNode = child(Node, ast::AstEdgeRole::Body);
    if (!RangeNode || !LoopVariableNode || !BodyNode)
      fail(CfgBuildErrorCode::InvalidAst, Node,
           "range-for statement is missing a required component");

    FlowFragment Range = buildExpression(*RangeNode);
    const CfgNodeId Initialize =
        addSemanticNode(CfgNodeKind::Operation, Node, "initialize range",
                        Semantics.InitializationMayThrow);
    const CfgNodeId Condition =
        addSemanticNode(CfgNodeKind::Condition, Node, "range has next",
                        Semantics.ConditionMayThrow);
    const CfgNodeId After =
        addSyntheticNode(CfgNodeKind::Anchor, "after range-for");
    FlowFragment LoopVariable = buildNode(*LoopVariableNode);
    const CfgNodeId ReadElement =
        addSemanticNode(CfgNodeKind::Operation, Node, "read range element",
                        Semantics.ElementMayThrow);
    const CfgNodeId Increment =
        addSemanticNode(CfgNodeKind::Operation, Node, "advance range",
                        Semantics.IncrementMayThrow);

    BreakTargets.push_back(After);
    ContinueTargets.push_back(Increment);
    FlowFragment Body = buildNode(*BodyNode);
    ContinueTargets.pop_back();
    BreakTargets.pop_back();

    connect(Range.Exits, Initialize);
    addEdge(Initialize, Condition, CfgEdgeKind::Flow, Node);
    addEdge(Condition, ReadElement, CfgEdgeKind::TrueBranch, Node);
    addEdge(Condition, After, CfgEdgeKind::FalseBranch, Node);
    addEdge(ReadElement, LoopVariable.Entry, CfgEdgeKind::Flow, Node);
    connect(LoopVariable.Exits, Body.Entry);
    connect(Body.Exits, Increment);
    addEdge(Increment, Condition, CfgEdgeKind::Flow, Node);
    if (Initializer)
      connect(Initializer->Exits, Range.Entry);
    return FlowFragment{Initializer ? Initializer->Entry : Range.Entry,
                        {{After}}};
  }

  [[nodiscard]] FlowFragment buildSwitch(ast::AstNodeId Node) {
    const std::vector<ast::AstNodeId> Initializers =
        children(Node, ast::AstEdgeRole::Initializer);
    std::optional<FlowFragment> Prefix;
    if (!Initializers.empty())
      Prefix = buildSequence(Initializers, Node);

    const std::vector<ast::AstNodeId> Conditions =
        children(Node, ast::AstEdgeRole::Condition);
    if (Conditions.empty())
      fail(CfgBuildErrorCode::InvalidAst, Node,
           "switch statement has no condition");
    const ast::AstNodeId ConditionOrigin = Conditions.back();
    const CfgNodeId Condition =
        addAstNode(CfgNodeKind::Condition, ConditionOrigin,
                   "switch " + Ast.node(ConditionOrigin).Payload.Display.Label,
                   std::nullopt, false);
    std::optional<FlowFragment> ConditionValue;
    if (containsControlExpression(ConditionOrigin)) {
      ConditionValue = buildExpression(ConditionOrigin);
      connect(ConditionValue->Exits, Condition);
    }
    if (Conditions.size() > 1) {
      std::vector<ast::AstNodeId> ConditionPrefixNodes(Conditions.begin(),
                                                       Conditions.end() - 1);
      FlowFragment ConditionPrefix = buildSequence(ConditionPrefixNodes, Node);
      if (Prefix) {
        connect(Prefix->Exits, ConditionPrefix.Entry);
        Prefix->Exits = std::move(ConditionPrefix.Exits);
      } else {
        Prefix = std::move(ConditionPrefix);
      }
    }

    const CfgNodeId After =
        addSyntheticNode(CfgNodeKind::Anchor, "after switch");
    std::vector<SwitchTarget> Targets;
    SwitchTargets.push_back(&Targets);
    BreakTargets.push_back(After);
    const auto BodyNode = child(Node, ast::AstEdgeRole::Body);
    FlowFragment Body = BodyNode ? buildNode(*BodyNode) : makeEmptyAnchor(Node);
    BreakTargets.pop_back();
    SwitchTargets.pop_back();
    connect(Body.Exits, After);

    bool HasDefault = false;
    for (const SwitchTarget &Target : Targets) {
      addEdge(Condition, Target.Node, Target.Kind, Target.Origin, Target.Label);
      HasDefault = HasDefault || Target.Kind == CfgEdgeKind::DefaultBranch;
    }
    if (!HasDefault)
      addEdge(Condition, After, CfgEdgeKind::DefaultBranch, Node);
    const CfgNodeId ConditionEntry =
        ConditionValue ? ConditionValue->Entry : Condition;
    if (Prefix)
      connect(Prefix->Exits, ConditionEntry);
    return FlowFragment{Prefix ? Prefix->Entry : ConditionEntry, {{After}}};
  }

  [[nodiscard]] FlowFragment buildCase(ast::AstNodeId Node, bool IsDefault) {
    if (SwitchTargets.empty())
      fail(CfgBuildErrorCode::InvalidAst, Node,
           "case label has no enclosing switch");

    std::string Label = "default";
    if (!IsDefault) {
      const std::optional<std::string> &CaseLabel =
          Ast.node(Node).Payload.Metadata.CaseLabel;
      Label = "case " + (CaseLabel ? *CaseLabel : std::string("?"));
    }
    const CfgNodeId Anchor = addAstNode(CfgNodeKind::Anchor, Node, Label);
    SwitchTargets.back()->push_back(SwitchTarget{
        Anchor,
        IsDefault ? CfgEdgeKind::DefaultBranch : CfgEdgeKind::CaseBranch, Node,
        Label});

    const ast::AstEdgeRole BodyRole =
        IsDefault ? ast::AstEdgeRole::DefaultBody : ast::AstEdgeRole::CaseBody;
    const auto BodyNode = child(Node, BodyRole);
    if (!BodyNode)
      return FlowFragment{Anchor, {{Anchor}}};
    FlowFragment Body = buildNode(*BodyNode);
    addEdge(Anchor, Body.Entry, CfgEdgeKind::Flow);
    return FlowFragment{Anchor, std::move(Body.Exits)};
  }

  [[nodiscard]] FlowFragment buildExpression(ast::AstNodeId Node) {
    const ast::AstNode &Payload = Ast.node(Node).Payload;

    if (Payload.Kind == ast::AstNodeKind::LambdaExpression)
      return buildLambda(Node);
    if (Payload.Metadata.ChildrenAreUnevaluated)
      return makeOperation(Node);

    if (Payload.Kind == ast::AstNodeKind::ThrowExpression)
      return buildThrow(Node);
    if (Payload.Kind == ast::AstNodeKind::CoAwaitExpression)
      return buildCoroutineSuspend(Node, "co_await");
    if (Payload.Kind == ast::AstNodeKind::CoYieldExpression)
      return buildCoroutineSuspend(Node, "co_yield");

    if (Payload.Kind == ast::AstNodeKind::BinaryExpression &&
        Payload.Metadata.Operator) {
      const std::string &Operator = *Payload.Metadata.Operator;
      if (Operator == "&&" || Operator == "||")
        return conditionAsValue(buildCondition(Node), Node);
      if (Operator == ",") {
        const auto Left = child(Node, ast::AstEdgeRole::LeftOperand);
        const auto Right = child(Node, ast::AstEdgeRole::RightOperand);
        if (!Left || !Right)
          fail(CfgBuildErrorCode::InvalidAst, Node,
               "comma expression is missing an operand");
        FlowFragment LeftFlow = buildExpression(*Left);
        FlowFragment RightFlow = buildExpression(*Right);
        connect(LeftFlow.Exits, RightFlow.Entry);
        LeftFlow.Exits = std::move(RightFlow.Exits);
        return LeftFlow;
      }
      if (isAssignmentOperator(Operator)) {
        const std::vector<ast::AstNodeId> EvaluatedOperands =
            assignmentControlOperands(Node);
        if (!EvaluatedOperands.empty()) {
          FlowFragment Operands =
              buildControlExpressionsInAstOrder(EvaluatedOperands, Node);
          FlowFragment Assignment = makeOperation(Node);
          connect(Operands.Exits, Assignment.Entry);
          Assignment.Entry = Operands.Entry;
          return Assignment;
        }
      }
    }

    if (Payload.Kind == ast::AstNodeKind::ConditionalExpression ||
        Payload.Kind == ast::AstNodeKind::CompileTimeConditionalExpression) {
      const auto ConditionNode = child(Node, ast::AstEdgeRole::Condition);
      const auto TrueNode = child(Node, ast::AstEdgeRole::ThenBranch);
      const auto FalseNode = child(Node, ast::AstEdgeRole::ElseBranch);
      if (!ConditionNode || !TrueNode || !FalseNode)
        fail(CfgBuildErrorCode::InvalidAst, Node,
             "conditional expression is incomplete");
      if (Payload.Kind == ast::AstNodeKind::CompileTimeConditionalExpression &&
          Payload.Metadata.ConstexprSelection &&
          *Payload.Metadata.ConstexprSelection !=
              ast::ConstexprBranchSelection::Dependent) {
        return buildExpression(*Payload.Metadata.ConstexprSelection ==
                                       ast::ConstexprBranchSelection::Then
                                   ? *TrueNode
                                   : *FalseNode);
      }

      ConditionFragment Condition = buildCondition(*ConditionNode);
      FlowFragment TrueFlow = buildExpression(*TrueNode);
      FlowFragment FalseFlow = buildExpression(*FalseNode);
      connect(Condition.TrueExits, TrueFlow.Entry, CfgEdgeKind::TrueBranch);
      connect(Condition.FalseExits, FalseFlow.Entry, CfgEdgeKind::FalseBranch);
      TrueFlow.Exits.insert(TrueFlow.Exits.end(), FalseFlow.Exits.begin(),
                            FalseFlow.Exits.end());
      TrueFlow.Entry = Condition.Entry;
      const std::vector<ast::AstNodeId> Initializers =
          children(Node, ast::AstEdgeRole::Initializer);
      if (!Initializers.empty()) {
        FlowFragment Prefix = buildSequence(Initializers, Node);
        connect(Prefix.Exits, TrueFlow.Entry);
        TrueFlow.Entry = Prefix.Entry;
      }
      return TrueFlow;
    }

    if (Payload.Kind == ast::AstNodeKind::CastExpression) {
      const std::vector<ast::AstNodeId> Children = children(Node);
      if (Children.size() == 1 && Ast.node(Children.front()).Payload.Kind ==
                                      ast::AstNodeKind::LambdaExpression)
        return buildLambda(Children.front());
      if (Children.size() == 1 && containsControlExpression(Children.front())) {
        FlowFragment Operand = buildNode(Children.front());
        FlowFragment Cast = makeOperation(Node);
        connect(Operand.Exits, Cast.Entry);
        Cast.Entry = Operand.Entry;
        return Cast;
      }
    }

    std::vector<ast::AstNodeId> ControlChildren;
    for (const ast::AstNodeId Child : children(Node)) {
      if (containsControlExpression(Child))
        ControlChildren.push_back(Child);
    }
    if (ControlChildren.size() == 1) {
      FlowFragment Child = buildNode(ControlChildren.front());
      FlowFragment Operation = makeOperation(Node);
      connect(Child.Exits, Operation.Entry);
      Operation.Entry = Child.Entry;
      return Operation;
    }
    if (!ControlChildren.empty()) {
      FlowFragment Prefix =
          buildControlExpressionsInAstOrder(ControlChildren, Node);
      FlowFragment Operation = makeOperation(Node);
      connect(Prefix.Exits, Operation.Entry);
      Operation.Entry = Prefix.Entry;
      return Operation;
    }
    return makeOperation(Node);
  }

  [[nodiscard]] FlowFragment conditionAsValue(ConditionFragment Condition,
                                              ast::AstNodeId Origin) {
    std::vector<FragmentExit> Exits;
    for (const CfgNodeId Node : Condition.TrueExits)
      Exits.push_back(FragmentExit{Node, CfgEdgeKind::TrueBranch, Origin});
    for (const CfgNodeId Node : Condition.FalseExits)
      Exits.push_back(FragmentExit{Node, CfgEdgeKind::FalseBranch, Origin});
    return FlowFragment{Condition.Entry, std::move(Exits)};
  }

  [[nodiscard]] bool containsControlExpression(ast::AstNodeId Node) const {
    const ast::AstNode &Payload = Ast.node(Node).Payload;
    if (Payload.Kind == ast::AstNodeKind::LambdaExpression) {
      for (const ast::AstEdgeId EdgeId : Ast.outgoingEdges(Node)) {
        const ast::AstGraph::Edge &Edge = Ast.edge(EdgeId);
        if (Edge.Payload.Role != ast::AstEdgeRole::LambdaBody &&
            containsControlExpression(Edge.Target))
          return true;
      }
      return false;
    }
    if (Payload.Metadata.MayThrow || Payload.Metadata.DoesNotReturn)
      return true;
    if (Payload.Metadata.ChildrenAreUnevaluated)
      return false;
    switch (Payload.Kind) {
    case ast::AstNodeKind::IfStatement:
    case ast::AstNodeKind::ConstexprIfStatement:
    case ast::AstNodeKind::SwitchStatement:
    case ast::AstNodeKind::ForStatement:
    case ast::AstNodeKind::RangeForStatement:
    case ast::AstNodeKind::WhileStatement:
    case ast::AstNodeKind::DoStatement:
    case ast::AstNodeKind::ReturnStatement:
    case ast::AstNodeKind::BreakStatement:
    case ast::AstNodeKind::ContinueStatement:
    case ast::AstNodeKind::GotoStatement:
    case ast::AstNodeKind::IndirectGotoStatement:
    case ast::AstNodeKind::LabelStatement:
    case ast::AstNodeKind::TryStatement:
    case ast::AstNodeKind::CoroutineBodyStatement:
    case ast::AstNodeKind::CoReturnStatement:
      return true;
    default:
      break;
    }
    if (Payload.Kind == ast::AstNodeKind::AsmStatement &&
        Payload.Metadata.IsAsmGoto)
      return true;
    if (Payload.Kind == ast::AstNodeKind::ConditionalExpression ||
        Payload.Kind == ast::AstNodeKind::CompileTimeConditionalExpression ||
        Payload.Kind == ast::AstNodeKind::ThrowExpression ||
        Payload.Kind == ast::AstNodeKind::CoAwaitExpression ||
        Payload.Kind == ast::AstNodeKind::CoYieldExpression)
      return true;
    if (Payload.Kind == ast::AstNodeKind::BinaryExpression &&
        Payload.Metadata.Operator) {
      const std::string &Operator = *Payload.Metadata.Operator;
      if (Operator == "&&" || Operator == "||" || Operator == ",")
        return true;
    }
    return containsControlExpressionBelow(Node);
  }

  [[nodiscard]] bool containsControlExpressionBelow(ast::AstNodeId Node) const {
    for (const ast::AstNodeId Child : children(Node)) {
      if (containsControlExpression(Child))
        return true;
    }
    return false;
  }

  [[nodiscard]] static bool isAssignmentOperator(const std::string &Operator) {
    static const std::vector<std::string> AssignmentOperators{
        "=", "*=", "/=", "%=", "+=", "-=", "<<=", ">>=", "&=", "^=", "|="};
    for (const std::string &Candidate : AssignmentOperators) {
      if (Candidate == Operator)
        return true;
    }
    return false;
  }

  void resolveLabelBranches() {
    for (const PendingLabelBranch &Branch : PendingLabelBranches) {
      const auto Target = Labels.find(Branch.Label);
      if (Target == Labels.end())
        fail(CfgBuildErrorCode::UnresolvedLabel, Branch.Origin,
             "unresolved label " + Branch.Label);
      addEdge(Branch.Node, Target->second, Branch.Kind, Branch.Origin,
              Branch.Kind == CfgEdgeKind::Flow
                  ? std::optional<std::string>{}
                  : std::optional<std::string>{Branch.Label});
    }
  }

  const ast::FunctionAstGraph &FunctionGraph;
  const ast::AstGraph &Ast;
  CfgGraph Graph;
  CfgNodeId Entry{0};
  CfgNodeId Exit{0};
  std::vector<CfgNodeId> BreakTargets;
  std::vector<CfgNodeId> ContinueTargets;
  std::vector<CfgNodeId> ExceptionTargets;
  std::vector<std::vector<SwitchTarget> *> SwitchTargets;
  std::unordered_map<std::string, CfgNodeId> Labels;
  std::vector<PendingLabelBranch> PendingLabelBranches;
  std::vector<std::string> AllLabels;
  std::vector<std::string> AddressTakenLabels;
};

} // namespace

FunctionCfgBuildResult
buildFunctionCfgGraph(const ast::FunctionAstGraph &AstGraph) {
  try {
    return FunctionCfgBuilder(AstGraph).build();
  } catch (BuildFailure &Failure) {
    return std::move(Failure.Error);
  } catch (const std::invalid_argument &Error) {
    return CfgBuildError{CfgBuildErrorCode::InternalInvariant,
                         AstGraph.function().Id, std::nullopt,
                         std::string("constructed CFG is invalid: ") +
                             Error.what()};
  }
}

FunctionCfgGraphsBuildResult
buildFunctionCfgGraphs(const ast::FunctionAstGraphs &AstGraphs) {
  FunctionCfgGraphs Result;
  Result.reserve(AstGraphs.size());
  for (const ast::FunctionAstGraph &AstGraph : AstGraphs) {
    FunctionCfgBuildResult FunctionResult = buildFunctionCfgGraph(AstGraph);
    if (auto *Error = std::get_if<CfgBuildError>(&FunctionResult))
      return std::move(*Error);
    Result.push_back(std::move(std::get<FunctionCfgGraph>(FunctionResult)));
  }
  return Result;
}

} // namespace mycpptools::cfg
