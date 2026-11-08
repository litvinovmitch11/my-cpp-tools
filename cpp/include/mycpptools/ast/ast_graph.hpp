#pragma once

#include "mycpptools/graph/directed_graph.hpp"
#include "mycpptools/source/function_info.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mycpptools::ast {

enum class AstNodeKind {
  FunctionDeclaration,
  ParameterDeclaration,
  VariableDeclaration,
  FieldDeclaration,
  RecordDeclaration,
  OtherDeclaration,

  CompoundStatement,
  DeclarationStatement,
  IfStatement,
  ConstexprIfStatement,
  SwitchStatement,
  CaseStatement,
  DefaultStatement,
  ForStatement,
  RangeForStatement,
  WhileStatement,
  DoStatement,
  ReturnStatement,
  BreakStatement,
  ContinueStatement,
  GotoStatement,
  IndirectGotoStatement,
  LabelStatement,
  TryStatement,
  CatchStatement,
  AsmStatement,
  CoroutineBodyStatement,
  CoReturnStatement,
  OtherStatement,

  LambdaExpression,
  ThrowExpression,
  CoAwaitExpression,
  CoYieldExpression,
  CallExpression,
  BinaryExpression,
  UnaryExpression,
  ConditionalExpression,
  CompileTimeConditionalExpression,
  DeclarationReferenceExpression,
  AddressOfLabelExpression,
  MemberExpression,
  CastExpression,
  LiteralExpression,
  OtherExpression,
};

enum class AstEdgeRole {
  Child,
  Declaration,
  Parameter,
  Body,
  Initializer,
  Condition,
  Increment,
  ThenBranch,
  ElseBranch,
  LeftOperand,
  RightOperand,
  Operand,
  Callee,
  Argument,
  ReturnValue,
  Range,
  LoopVariable,
  CaseValue,
  CaseRangeEnd,
  CaseBody,
  DefaultBody,
  LabelBody,
  LambdaBody,
  TryBody,
  Handler,
  CatchBody,
  ExceptionDeclaration,
  ThrownValue,
};

enum class ConstexprBranchSelection {
  Then,
  Else,
  Dependent,
};

enum class FunctionKind {
  Ordinary,
  Constructor,
  Destructor,
};

struct RangeForSemantics {
  bool InitializationMayThrow = false;
  bool ConditionMayThrow = false;
  bool ElementMayThrow = false;
  bool IncrementMayThrow = false;
};

[[nodiscard]] std::string_view toString(AstNodeKind Kind) noexcept;
[[nodiscard]] std::string_view toString(AstEdgeRole Role) noexcept;

struct AstNodeMetadata {
  // Exact parser node name for presentation and diagnostics only. Analysis
  // code must use AstNodeKind instead of interpreting this string.
  std::string ConcreteKind;
  std::optional<std::string> Name;
  std::optional<std::string> QualifiedName;
  std::optional<std::string> TypeName;
  std::optional<std::string> Value;
  std::optional<std::string> Operator;
  std::optional<std::string> TargetLabel;
  std::vector<std::string> TargetLabels;
  std::optional<std::string> CaseLabel;
  std::optional<std::string> ExceptionType;
  std::optional<ConstexprBranchSelection> ConstexprSelection;
  std::optional<RangeForSemantics> RangeFor;
  std::optional<source::SourceRange> Source;
  FunctionKind FunctionCategory = FunctionKind::Ordinary;
  bool IsAsmGoto = false;
  bool IsCatchAll = false;
  bool IsTransparent = false;
  bool MayThrow = false;
  bool DoesNotReturn = false;
  bool ChildrenAreUnevaluated = false;
};

struct AstNodeDisplay {
  std::string Label;
};

struct AstNode {
  AstNodeKind Kind;
  AstNodeMetadata Metadata;
  AstNodeDisplay Display;
};

struct AstEdge {
  AstEdgeRole Role;
  std::size_t Ordinal;
};

using AstGraph = graph::DirectedGraph<AstNode, AstEdge>;
using AstNodeId = AstGraph::NodeId;
using AstEdgeId = AstGraph::EdgeId;

class FunctionAstGraph {
public:
  // Throws std::invalid_argument unless Graph is a rooted AST tree whose root
  // is a function declaration and whose child ordinals are contiguous.
  FunctionAstGraph(source::FunctionInfo Function, AstGraph Graph);

  [[nodiscard]] const source::FunctionInfo &function() const noexcept;
  [[nodiscard]] const AstGraph &graph() const noexcept;

private:
  source::FunctionInfo Function;
  AstGraph Graph;
};

using FunctionAstGraphs = std::vector<FunctionAstGraph>;

} // namespace mycpptools::ast
