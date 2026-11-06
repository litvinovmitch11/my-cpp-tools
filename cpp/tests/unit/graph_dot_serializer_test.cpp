#include "mycpptools/presentation/graph_dot_serializer.hpp"

#include "support/test_support.hpp"

#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <utility>

namespace ast = mycpptools::ast;
namespace presentation = mycpptools::presentation;

static ast::FunctionAstGraph makeFunctionGraph() {
  ast::AstGraph Graph;
  ast::AstNodeMetadata FunctionMetadata;
  FunctionMetadata.ConcreteKind = "Function";
  const ast::AstNodeId Root = Graph.addNode(ast::AstNode{
      ast::AstNodeKind::FunctionDeclaration, std::move(FunctionMetadata),
      ast::AstNodeDisplay{"Function run"}});
  ast::AstNodeMetadata BodyMetadata;
  BodyMetadata.ConcreteKind = "CompoundStmt";
  const ast::AstNodeId Body = Graph.addNode(
      ast::AstNode{ast::AstNodeKind::CompoundStatement, std::move(BodyMetadata),
                   ast::AstNodeDisplay{"CompoundStmt"}});
  Graph.setRoot(Root);
  (void)Graph.addEdge(Root, Body, ast::AstEdge{ast::AstEdgeRole::Body, 0});

  ast::FunctionInfo Function{ast::FunctionId("run-id"), "run", "sample::run",
                             "void ()", ast::SourceRange{0, 10}};
  return ast::FunctionAstGraph(std::move(Function), std::move(Graph));
}

static void writesStableAstPresentation() {
  ast::FunctionAstGraphs Graphs;
  Graphs.push_back(makeFunctionGraph());

  std::ostringstream Output;
  const presentation::GraphDotSerializer Serializer;
  MYCPPTOOLS_EXPECT_TRUE(Serializer.writeAst(Output, Graphs));

  const std::string Expected =
      "digraph AST {\n"
      "  rankdir=\"TB\";\n"
      "  bgcolor=\"transparent\";\n"
      "  graph [fontname=\"Arial\", fontcolor=\"#D4D4D4\", pad=\"0.2\", "
      "nodesep=\"0.25\", ranksep=\"0.45\"];\n"
      "  node [shape=\"box\", style=\"rounded,filled\", "
      "fontname=\"Arial\", fontsize=\"10\", fontcolor=\"#F3F4F6\", "
      "color=\"#4B5563\", margin=\"0.12,0.08\"];\n"
      "  edge [color=\"#6B7280\", penwidth=\"1.0\", "
      "arrowsize=\"0.65\", fontname=\"Arial\", fontsize=\"9\", "
      "fontcolor=\"#D4D4D4\"];\n"
      "  subgraph cluster_0 {\n"
      "    label=\"sample::run : void ()\";\n"
      "    color=\"#4B5563\";\n"
      "    fontcolor=\"#D4D4D4\";\n"
      "    style=\"rounded\";\n"
      "    margin=\"12\";\n"
      "    f0_n0 [label=\"Function run\", fillcolor=\"#1E3A5F\"];\n"
      "    f0_n1 [label=\"CompoundStmt\", fillcolor=\"#1F4D3A\"];\n"
      "    f0_n0 -> f0_n1 [label=\"body\"];\n"
      "  }\n"
      "}\n";
  MYCPPTOOLS_EXPECT_EQ(Output.str(), Expected);
}

namespace {

class FailingBuffer : public std::streambuf {
protected:
  int_type overflow(int_type) override { return traits_type::eof(); }
};

} // namespace

static void reportsOutputFailure() {
  FailingBuffer Buffer;
  std::ostream Output(&Buffer);
  const presentation::GraphDotSerializer Serializer;
  MYCPPTOOLS_EXPECT_TRUE(!Serializer.writeAst(Output, {}));
}

int main() {
  return mycpptools::test::runTests({
      {"writes a stable AST presentation", writesStableAstPresentation},
      {"reports an output failure", reportsOutputFailure},
  });
}
