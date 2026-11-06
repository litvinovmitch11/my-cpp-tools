#pragma once

#include "mycpptools/graph/directed_graph.hpp"

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
  DeclarationReferenceExpression,
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
  ThrownValue,
};

[[nodiscard]] std::string_view toString(AstNodeKind Kind) noexcept;
[[nodiscard]] std::string_view toString(AstEdgeRole Role) noexcept;

struct SourceRange {
  // Half-open byte offsets into the main source file: [BeginOffset, EndOffset).
  std::size_t BeginOffset = 0;
  std::size_t EndOffset = 0;
};

struct AstNodeMetadata {
  // Exact parser node name for presentation and diagnostics only. Analysis
  // code must use AstNodeKind instead of interpreting this string.
  std::string ConcreteKind;
  std::optional<std::string> Name;
  std::optional<std::string> QualifiedName;
  std::optional<std::string> TypeName;
  std::optional<std::string> Value;
  std::optional<std::string> Operator;
  std::optional<SourceRange> Source;
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

class FunctionId {
public:
  // Throws std::invalid_argument when Value is empty.
  explicit FunctionId(std::string Value);

  [[nodiscard]] const std::string &value() const noexcept;

  friend bool operator==(const FunctionId &Left,
                         const FunctionId &Right) noexcept;
  friend bool operator!=(const FunctionId &Left,
                         const FunctionId &Right) noexcept;
  friend bool operator<(const FunctionId &Left,
                        const FunctionId &Right) noexcept;

private:
  std::string Value;
};

struct FunctionInfo {
  FunctionId Id;
  std::string Name;
  std::string QualifiedName;
  std::string Signature;
  std::optional<SourceRange> Definition;
};

class FunctionAstGraph {
public:
  // Throws std::invalid_argument unless Graph is a rooted AST tree whose root
  // is a function declaration and whose child ordinals are contiguous.
  FunctionAstGraph(FunctionInfo Function, AstGraph Graph);

  [[nodiscard]] const FunctionInfo &function() const noexcept;
  [[nodiscard]] const AstGraph &graph() const noexcept;

private:
  FunctionInfo Function;
  AstGraph Graph;
};

using FunctionAstGraphs = std::vector<FunctionAstGraph>;

} // namespace mycpptools::ast
