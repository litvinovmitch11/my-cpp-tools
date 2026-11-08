#include "mycpptools/ast/ast_graph.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace mycpptools::ast {

static void validateAstGraph(const AstGraph &Graph) {
  if (Graph.empty())
    throw std::invalid_argument("function AST graph must not be empty");

  const std::optional<AstNodeId> Root = Graph.root();
  if (!Root)
    throw std::invalid_argument("function AST graph must have a root");
  if (Graph.node(*Root).Payload.Kind != AstNodeKind::FunctionDeclaration)
    throw std::invalid_argument(
        "function AST graph root must be a function declaration");
  if (Graph.edgeCount() != Graph.nodeCount() - 1)
    throw std::invalid_argument(
        "function AST graph must contain exactly one parent edge per "
        "non-root node");

  std::vector<bool> Visited(Graph.nodeCount(), false);
  std::vector<AstNodeId> Pending{*Root};

  while (!Pending.empty()) {
    const AstNodeId Node = Pending.back();
    Pending.pop_back();
    if (Visited[Node.value()])
      throw std::invalid_argument("function AST graph must be acyclic");
    Visited[Node.value()] = true;

    const auto &Incoming = Graph.incomingEdges(Node);
    const std::size_t ExpectedParentCount = Node == *Root ? 0 : 1;
    if (Incoming.size() != ExpectedParentCount)
      throw std::invalid_argument(
          "function AST graph nodes must have exactly one parent except for "
          "the root");

    const auto &Outgoing = Graph.outgoingEdges(Node);
    for (std::size_t Ordinal = 0; Ordinal < Outgoing.size(); ++Ordinal) {
      const AstGraph::Edge &Edge = Graph.edge(Outgoing[Ordinal]);
      if (Edge.Source != Node || Edge.Payload.Ordinal != Ordinal)
        throw std::invalid_argument(
            "function AST graph child ordinals must be contiguous and match "
            "edge order");
      Pending.push_back(Edge.Target);
    }
  }

  for (const bool WasVisited : Visited) {
    if (!WasVisited)
      throw std::invalid_argument(
          "function AST graph nodes must be reachable from the root");
  }
}

std::string_view toString(AstNodeKind Kind) noexcept {
  switch (Kind) {
  case AstNodeKind::FunctionDeclaration:
    return "function declaration";
  case AstNodeKind::ParameterDeclaration:
    return "parameter declaration";
  case AstNodeKind::VariableDeclaration:
    return "variable declaration";
  case AstNodeKind::FieldDeclaration:
    return "field declaration";
  case AstNodeKind::RecordDeclaration:
    return "record declaration";
  case AstNodeKind::OtherDeclaration:
    return "other declaration";
  case AstNodeKind::CompoundStatement:
    return "compound statement";
  case AstNodeKind::DeclarationStatement:
    return "declaration statement";
  case AstNodeKind::IfStatement:
    return "if statement";
  case AstNodeKind::ConstexprIfStatement:
    return "constexpr if statement";
  case AstNodeKind::SwitchStatement:
    return "switch statement";
  case AstNodeKind::CaseStatement:
    return "case statement";
  case AstNodeKind::DefaultStatement:
    return "default statement";
  case AstNodeKind::ForStatement:
    return "for statement";
  case AstNodeKind::RangeForStatement:
    return "range-for statement";
  case AstNodeKind::WhileStatement:
    return "while statement";
  case AstNodeKind::DoStatement:
    return "do statement";
  case AstNodeKind::ReturnStatement:
    return "return statement";
  case AstNodeKind::BreakStatement:
    return "break statement";
  case AstNodeKind::ContinueStatement:
    return "continue statement";
  case AstNodeKind::GotoStatement:
    return "goto statement";
  case AstNodeKind::IndirectGotoStatement:
    return "indirect goto statement";
  case AstNodeKind::LabelStatement:
    return "label statement";
  case AstNodeKind::TryStatement:
    return "try statement";
  case AstNodeKind::CatchStatement:
    return "catch statement";
  case AstNodeKind::AsmStatement:
    return "asm statement";
  case AstNodeKind::CoroutineBodyStatement:
    return "coroutine body statement";
  case AstNodeKind::CoReturnStatement:
    return "co-return statement";
  case AstNodeKind::OtherStatement:
    return "other statement";
  case AstNodeKind::LambdaExpression:
    return "lambda expression";
  case AstNodeKind::ThrowExpression:
    return "throw expression";
  case AstNodeKind::CoAwaitExpression:
    return "co-await expression";
  case AstNodeKind::CoYieldExpression:
    return "co-yield expression";
  case AstNodeKind::CallExpression:
    return "call expression";
  case AstNodeKind::BinaryExpression:
    return "binary expression";
  case AstNodeKind::UnaryExpression:
    return "unary expression";
  case AstNodeKind::ConditionalExpression:
    return "conditional expression";
  case AstNodeKind::CompileTimeConditionalExpression:
    return "compile-time conditional expression";
  case AstNodeKind::DeclarationReferenceExpression:
    return "declaration reference expression";
  case AstNodeKind::AddressOfLabelExpression:
    return "address-of-label expression";
  case AstNodeKind::MemberExpression:
    return "member expression";
  case AstNodeKind::CastExpression:
    return "cast expression";
  case AstNodeKind::LiteralExpression:
    return "literal expression";
  case AstNodeKind::OtherExpression:
    return "other expression";
  }

  return "unknown AST node";
}

std::string_view toString(AstEdgeRole Role) noexcept {
  switch (Role) {
  case AstEdgeRole::Child:
    return "child";
  case AstEdgeRole::Declaration:
    return "declaration";
  case AstEdgeRole::Parameter:
    return "parameter";
  case AstEdgeRole::Body:
    return "body";
  case AstEdgeRole::Initializer:
    return "initializer";
  case AstEdgeRole::Condition:
    return "condition";
  case AstEdgeRole::Increment:
    return "increment";
  case AstEdgeRole::ThenBranch:
    return "then";
  case AstEdgeRole::ElseBranch:
    return "else";
  case AstEdgeRole::LeftOperand:
    return "left operand";
  case AstEdgeRole::RightOperand:
    return "right operand";
  case AstEdgeRole::Operand:
    return "operand";
  case AstEdgeRole::Callee:
    return "callee";
  case AstEdgeRole::Argument:
    return "argument";
  case AstEdgeRole::ReturnValue:
    return "return value";
  case AstEdgeRole::Range:
    return "range";
  case AstEdgeRole::LoopVariable:
    return "loop variable";
  case AstEdgeRole::CaseValue:
    return "case value";
  case AstEdgeRole::CaseRangeEnd:
    return "case range end";
  case AstEdgeRole::CaseBody:
    return "case body";
  case AstEdgeRole::DefaultBody:
    return "default body";
  case AstEdgeRole::LabelBody:
    return "label body";
  case AstEdgeRole::LambdaBody:
    return "lambda body";
  case AstEdgeRole::TryBody:
    return "try body";
  case AstEdgeRole::Handler:
    return "handler";
  case AstEdgeRole::CatchBody:
    return "catch body";
  case AstEdgeRole::ExceptionDeclaration:
    return "exception declaration";
  case AstEdgeRole::ThrownValue:
    return "thrown value";
  }

  return "unknown AST edge";
}

FunctionAstGraph::FunctionAstGraph(source::FunctionInfo Function,
                                   AstGraph Graph)
    : Function(std::move(Function)), Graph(std::move(Graph)) {
  validateAstGraph(this->Graph);
}

const source::FunctionInfo &FunctionAstGraph::function() const noexcept {
  return Function;
}

const AstGraph &FunctionAstGraph::graph() const noexcept { return Graph; }

} // namespace mycpptools::ast
