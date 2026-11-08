#include "mycpptools/presentation/graph_dot_serializer.hpp"

#include "support/test_support.hpp"

#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <utility>

namespace ast = mycpptools::ast;
namespace cfg = mycpptools::cfg;
namespace presentation = mycpptools::presentation;
namespace source = mycpptools::source;

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

  source::FunctionInfo Function{source::FunctionId("run-id"), "run",
                                "sample::run", "void ()",
                                source::SourceRange{0, 10}};
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

static cfg::FunctionCfgGraph makeCfgGraph() {
  cfg::CfgGraph Graph;
  const cfg::CfgNodeId Entry = Graph.addNode(
      cfg::CfgNode{cfg::CfgNodeKind::Entry, {}, cfg::CfgNodeDisplay{"Entry"}});
  const cfg::CfgNodeId Exit = Graph.addNode(
      cfg::CfgNode{cfg::CfgNodeKind::Exit, {}, cfg::CfgNodeDisplay{"Exit"}});
  const cfg::CfgNodeId Condition = Graph.addNode(cfg::CfgNode{
      cfg::CfgNodeKind::Condition, {}, cfg::CfgNodeDisplay{"ready"}});
  (void)Graph.addNode(cfg::CfgNode{
      cfg::CfgNodeKind::Operation, {}, cfg::CfgNodeDisplay{"unreachable"}});
  Graph.setRoot(Entry);
  (void)Graph.addEdge(
      Entry, Condition,
      cfg::CfgEdge{cfg::CfgEdgeKind::Flow, std::nullopt, std::nullopt});
  (void)Graph.addEdge(
      Condition, Exit,
      cfg::CfgEdge{cfg::CfgEdgeKind::TrueBranch, std::nullopt, std::nullopt});
  (void)Graph.addEdge(
      Condition, Exit,
      cfg::CfgEdge{cfg::CfgEdgeKind::FalseBranch, std::nullopt, std::nullopt});

  source::FunctionInfo Function{source::FunctionId("run-id"), "run",
                                "sample::run", "void ()",
                                source::SourceRange{0, 10}};
  return cfg::FunctionCfgGraph(std::move(Function), std::move(Graph), Exit);
}

static void writesStableCfgPresentation() {
  cfg::FunctionCfgGraphs Graphs;
  Graphs.push_back(makeCfgGraph());

  std::ostringstream Output;
  const presentation::GraphDotSerializer Serializer;
  MYCPPTOOLS_EXPECT_TRUE(Serializer.writeCfg(Output, Graphs));

  const std::string Expected =
      "digraph CFG {\n"
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
      "    f0_n0 [label=\"Entry\", shape=\"oval\", style=\"filled\", "
      "fillcolor=\"#1E3A5F\"];\n"
      "    f0_n1 [label=\"Exit\", shape=\"oval\", style=\"filled\", "
      "fillcolor=\"#1E3A5F\"];\n"
      "    f0_n2 [label=\"ready\", shape=\"diamond\", style=\"filled\", "
      "fillcolor=\"#1F4D3A\"];\n"
      "    f0_n3 [label=\"unreachable\", fillcolor=\"#1E3A5F\", "
      "style=\"dashed,filled\", color=\"#D4D4D4\", "
      "fontcolor=\"#D4D4D4\"];\n"
      "    f0_n0 -> f0_n2;\n"
      "    f0_n2 -> f0_n1 [label=\"true\"];\n"
      "    f0_n2 -> f0_n1 [label=\"false\"];\n"
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

static void writesUnreachableStyledNode() {
  cfg::CfgGraph Graph;
  const cfg::CfgNodeId Entry = Graph.addNode(
      cfg::CfgNode{cfg::CfgNodeKind::Entry, {}, cfg::CfgNodeDisplay{"Entry"}});
  const cfg::CfgNodeId Exit = Graph.addNode(
      cfg::CfgNode{cfg::CfgNodeKind::Exit, {}, cfg::CfgNodeDisplay{"Exit"}});
  const cfg::CfgNodeId Unreachable =
      Graph.addNode(cfg::CfgNode{cfg::CfgNodeKind::Condition,
                                 {},
                                 cfg::CfgNodeDisplay{"unreachable branch"}});
  Graph.setRoot(Entry);
  (void)Graph.addEdge(
      Entry, Exit,
      cfg::CfgEdge{cfg::CfgEdgeKind::Flow, std::nullopt, std::nullopt});
  (void)Graph.addEdge(
      Unreachable, Exit,
      cfg::CfgEdge{cfg::CfgEdgeKind::TrueBranch, std::nullopt, std::nullopt});
  (void)Graph.addEdge(
      Unreachable, Exit,
      cfg::CfgEdge{cfg::CfgEdgeKind::FalseBranch, std::nullopt, std::nullopt});

  source::FunctionInfo Function{source::FunctionId("run-id"), "run", "run",
                                "void ()", std::nullopt};
  cfg::FunctionCfgGraphs Graphs;
  Graphs.emplace_back(std::move(Function), std::move(Graph), Exit);

  std::ostringstream Output;
  const presentation::GraphDotSerializer Serializer;
  MYCPPTOOLS_EXPECT_TRUE(Serializer.writeCfg(Output, Graphs));
  MYCPPTOOLS_EXPECT_TRUE(Output.str().find("style=\"dashed,filled\"") !=
                         std::string::npos);
}

int main() {
  return mycpptools::test::runTests({
      {"writes a stable AST presentation", writesStableAstPresentation},
      {"writes a stable CFG presentation", writesStableCfgPresentation},
      {"writes an unreachable styled node", writesUnreachableStyledNode},
      {"reports an output failure", reportsOutputFailure},
  });
}
