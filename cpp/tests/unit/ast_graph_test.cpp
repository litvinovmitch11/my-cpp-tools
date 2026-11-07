#include "mycpptools/ast/ast_graph.hpp"

#include "support/test_support.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace ast = mycpptools::ast;

static ast::AstNode makeNode(ast::AstNodeKind Kind, std::string Label) {
  ast::AstNodeMetadata Metadata;
  Metadata.ConcreteKind = Label;
  return ast::AstNode{Kind, std::move(Metadata),
                      ast::AstNodeDisplay{std::move(Label)}};
}

static ast::FunctionInfo makeFunctionInfo() {
  return ast::FunctionInfo{ast::FunctionId("function-id"), "function",
                           "sample::function", "int ()",
                           ast::SourceRange{0, 10}};
}

static ast::AstGraph makeValidGraph() {
  ast::AstGraph Graph;
  const ast::AstNodeId Root = Graph.addNode(
      makeNode(ast::AstNodeKind::FunctionDeclaration, "Function function"));
  const ast::AstNodeId Body = Graph.addNode(
      makeNode(ast::AstNodeKind::CompoundStatement, "CompoundStmt"));
  Graph.setRoot(Root);
  (void)Graph.addEdge(Root, Body, ast::AstEdge{ast::AstEdgeRole::Body, 0});
  return Graph;
}

static void acceptsRootedTree() {
  const ast::FunctionAstGraph FunctionGraph(makeFunctionInfo(),
                                            makeValidGraph());
  MYCPPTOOLS_EXPECT_EQ(FunctionGraph.function().Id.value(), "function-id");
  MYCPPTOOLS_EXPECT_EQ(FunctionGraph.graph().nodeCount(), 2U);
}

static void rejectsInvalidFunctionId() {
  MYCPPTOOLS_EXPECT_THROWS(ast::FunctionId(""), std::invalid_argument);
}

static void comparesFunctionIdsByValue() {
  const ast::FunctionId First("first");
  const ast::FunctionId FirstCopy("first");
  const ast::FunctionId Second("second");

  MYCPPTOOLS_EXPECT_TRUE(First == FirstCopy);
  MYCPPTOOLS_EXPECT_TRUE(First != Second);
  MYCPPTOOLS_EXPECT_TRUE(First < Second);
}

static void rejectsEmptyGraph() {
  MYCPPTOOLS_EXPECT_THROWS(
      ast::FunctionAstGraph(makeFunctionInfo(), ast::AstGraph{}),
      std::invalid_argument);
}

static void rejectsMissingRoot() {
  ast::AstGraph Graph;
  (void)Graph.addNode(
      makeNode(ast::AstNodeKind::FunctionDeclaration, "Function function"));
  MYCPPTOOLS_EXPECT_THROWS(
      ast::FunctionAstGraph(makeFunctionInfo(), std::move(Graph)),
      std::invalid_argument);
}

static void rejectsWrongRootKind() {
  ast::AstGraph Graph;
  const ast::AstNodeId Root = Graph.addNode(
      makeNode(ast::AstNodeKind::CompoundStatement, "CompoundStmt"));
  Graph.setRoot(Root);
  MYCPPTOOLS_EXPECT_THROWS(
      ast::FunctionAstGraph(makeFunctionInfo(), std::move(Graph)),
      std::invalid_argument);
}

static void rejectsIncorrectEdgeCount() {
  ast::AstGraph Graph;
  const ast::AstNodeId Root = Graph.addNode(
      makeNode(ast::AstNodeKind::FunctionDeclaration, "Function function"));
  (void)Graph.addNode(
      makeNode(ast::AstNodeKind::CompoundStatement, "CompoundStmt"));
  Graph.setRoot(Root);

  MYCPPTOOLS_EXPECT_THROWS(
      ast::FunctionAstGraph(makeFunctionInfo(), std::move(Graph)),
      std::invalid_argument);
}

static void rejectsRootWithParent() {
  ast::AstGraph Graph;
  const ast::AstNodeId Root = Graph.addNode(
      makeNode(ast::AstNodeKind::FunctionDeclaration, "Function function"));
  const ast::AstNodeId Parent = Graph.addNode(
      makeNode(ast::AstNodeKind::CompoundStatement, "CompoundStmt"));
  Graph.setRoot(Root);
  (void)Graph.addEdge(Parent, Root, ast::AstEdge{ast::AstEdgeRole::Child, 0});

  MYCPPTOOLS_EXPECT_THROWS(
      ast::FunctionAstGraph(makeFunctionInfo(), std::move(Graph)),
      std::invalid_argument);
}

static void rejectsMultipleParents() {
  ast::AstGraph Graph;
  const ast::AstNodeId Root = Graph.addNode(
      makeNode(ast::AstNodeKind::FunctionDeclaration, "Function function"));
  const ast::AstNodeId First =
      Graph.addNode(makeNode(ast::AstNodeKind::CompoundStatement, "First"));
  const ast::AstNodeId Second =
      Graph.addNode(makeNode(ast::AstNodeKind::CompoundStatement, "Second"));
  (void)Graph.addNode(
      makeNode(ast::AstNodeKind::CompoundStatement, "Disconnected"));
  Graph.setRoot(Root);
  (void)Graph.addEdge(Root, First, ast::AstEdge{ast::AstEdgeRole::Child, 0});
  (void)Graph.addEdge(Root, Second, ast::AstEdge{ast::AstEdgeRole::Child, 1});
  (void)Graph.addEdge(First, Second, ast::AstEdge{ast::AstEdgeRole::Child, 0});

  MYCPPTOOLS_EXPECT_THROWS(
      ast::FunctionAstGraph(makeFunctionInfo(), std::move(Graph)),
      std::invalid_argument);
}

static void rejectsUnreachableNodes() {
  ast::AstGraph Graph;
  const ast::AstNodeId Root = Graph.addNode(
      makeNode(ast::AstNodeKind::FunctionDeclaration, "Function function"));
  const ast::AstNodeId Reachable =
      Graph.addNode(makeNode(ast::AstNodeKind::CompoundStatement, "Reachable"));
  const ast::AstNodeId FirstDisconnected = Graph.addNode(
      makeNode(ast::AstNodeKind::CompoundStatement, "FirstDisconnected"));
  const ast::AstNodeId SecondDisconnected = Graph.addNode(
      makeNode(ast::AstNodeKind::CompoundStatement, "SecondDisconnected"));
  Graph.setRoot(Root);
  (void)Graph.addEdge(Root, Reachable,
                      ast::AstEdge{ast::AstEdgeRole::Child, 0});
  (void)Graph.addEdge(FirstDisconnected, SecondDisconnected,
                      ast::AstEdge{ast::AstEdgeRole::Child, 0});
  (void)Graph.addEdge(SecondDisconnected, FirstDisconnected,
                      ast::AstEdge{ast::AstEdgeRole::Child, 0});

  MYCPPTOOLS_EXPECT_THROWS(
      ast::FunctionAstGraph(makeFunctionInfo(), std::move(Graph)),
      std::invalid_argument);
}

static void rejectsNonContiguousOrdinals() {
  ast::AstGraph Graph;
  const ast::AstNodeId Root = Graph.addNode(
      makeNode(ast::AstNodeKind::FunctionDeclaration, "Function function"));
  const ast::AstNodeId Body = Graph.addNode(
      makeNode(ast::AstNodeKind::CompoundStatement, "CompoundStmt"));
  Graph.setRoot(Root);
  (void)Graph.addEdge(Root, Body, ast::AstEdge{ast::AstEdgeRole::Body, 1});

  MYCPPTOOLS_EXPECT_THROWS(
      ast::FunctionAstGraph(makeFunctionInfo(), std::move(Graph)),
      std::invalid_argument);
}

int main() {
  return mycpptools::test::runTests({
      {"accepts a rooted AST tree", acceptsRootedTree},
      {"rejects an empty function ID", rejectsInvalidFunctionId},
      {"compares function IDs by value", comparesFunctionIdsByValue},
      {"rejects an empty graph", rejectsEmptyGraph},
      {"rejects a missing root", rejectsMissingRoot},
      {"rejects a non-function root", rejectsWrongRootKind},
      {"rejects an incorrect edge count", rejectsIncorrectEdgeCount},
      {"rejects a root with a parent", rejectsRootWithParent},
      {"rejects multiple parents", rejectsMultipleParents},
      {"rejects unreachable nodes", rejectsUnreachableNodes},
      {"rejects non-contiguous child ordinals", rejectsNonContiguousOrdinals},
  });
}
