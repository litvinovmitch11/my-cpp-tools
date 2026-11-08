#include "mycpptools/presentation/graph_dot_serializer.hpp"

#include "mycpptools/dot/dot_writer.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace mycpptools::presentation {

namespace {

enum class AstNodeCategory {
  Declaration,
  Statement,
  Expression,
};

} // namespace

[[nodiscard]] static AstNodeCategory astNodeCategory(ast::AstNodeKind Kind) {
  switch (Kind) {
  case ast::AstNodeKind::FunctionDeclaration:
  case ast::AstNodeKind::ParameterDeclaration:
  case ast::AstNodeKind::VariableDeclaration:
  case ast::AstNodeKind::FieldDeclaration:
  case ast::AstNodeKind::RecordDeclaration:
  case ast::AstNodeKind::OtherDeclaration:
    return AstNodeCategory::Declaration;

  case ast::AstNodeKind::CompoundStatement:
  case ast::AstNodeKind::DeclarationStatement:
  case ast::AstNodeKind::IfStatement:
  case ast::AstNodeKind::ConstexprIfStatement:
  case ast::AstNodeKind::SwitchStatement:
  case ast::AstNodeKind::CaseStatement:
  case ast::AstNodeKind::DefaultStatement:
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
  case ast::AstNodeKind::CatchStatement:
  case ast::AstNodeKind::AsmStatement:
  case ast::AstNodeKind::CoroutineBodyStatement:
  case ast::AstNodeKind::CoReturnStatement:
  case ast::AstNodeKind::OtherStatement:
    return AstNodeCategory::Statement;

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
    return AstNodeCategory::Expression;
  }

  throw std::invalid_argument("unknown AST node kind");
}

[[nodiscard]] static const std::string &nodeColor(const DotPalette &Palette,
                                                  ast::AstNodeKind Kind) {
  switch (astNodeCategory(Kind)) {
  case AstNodeCategory::Declaration:
    return Palette.PrimaryNodeColor;
  case AstNodeCategory::Statement:
    return Palette.SecondaryNodeColor;
  case AstNodeCategory::Expression:
    return Palette.AccentNodeColor;
  }

  throw std::invalid_argument("unknown AST node category");
}

[[nodiscard]] static std::string clusterId(std::size_t FunctionIndex) {
  return "cluster_" + std::to_string(FunctionIndex);
}

[[nodiscard]] static std::string nodeId(std::size_t FunctionIndex,
                                        ast::AstNodeId Node) {
  return "f" + std::to_string(FunctionIndex) + "_n" +
         std::to_string(Node.value());
}

[[nodiscard]] static std::string nodeId(std::size_t FunctionIndex,
                                        cfg::CfgNodeId Node) {
  return "f" + std::to_string(FunctionIndex) + "_n" +
         std::to_string(Node.value());
}

[[nodiscard]] static dot::DotAttributes
cfgNodeAttributes(const DotPalette &Palette, const cfg::CfgNode &Node,
                  bool Reachable) {
  dot::DotAttributes Attributes{{"label", Node.Display.Label}};
  switch (Node.Kind) {
  case cfg::CfgNodeKind::Entry:
  case cfg::CfgNodeKind::Exit:
    Attributes.push_back({"shape", "oval"});
    Attributes.push_back({"style", "filled"});
    Attributes.push_back({"fillcolor", Palette.PrimaryNodeColor});
    break;
  case cfg::CfgNodeKind::Operation:
    Attributes.push_back({"fillcolor", Palette.PrimaryNodeColor});
    break;
  case cfg::CfgNodeKind::Condition:
    Attributes.push_back({"shape", "diamond"});
    Attributes.push_back({"style", "filled"});
    Attributes.push_back({"fillcolor", Palette.SecondaryNodeColor});
    break;
  case cfg::CfgNodeKind::Transfer:
    Attributes.push_back({"fillcolor", Palette.AccentNodeColor});
    break;
  case cfg::CfgNodeKind::Anchor:
    Attributes.push_back({"shape", "ellipse"});
    Attributes.push_back({"style", "filled"});
    Attributes.push_back({"fillcolor", Palette.BackgroundColor});
    break;
  }

  if (!Reachable) {
    bool ReplacedStyle = false;
    for (dot::DotAttribute &Attribute : Attributes) {
      if (Attribute.Name == "style") {
        Attribute.Value = "dashed,filled";
        ReplacedStyle = true;
        break;
      }
    }
    if (!ReplacedStyle)
      Attributes.push_back({"style", "dashed,filled"});
    Attributes.push_back({"color", Palette.MutedTextColor});
    Attributes.push_back({"fontcolor", Palette.MutedTextColor});
  }
  return Attributes;
}

[[nodiscard]] static dot::DotAttributes
cfgEdgeAttributes(const cfg::CfgEdge &Edge, bool Reachable,
                  const DotPalette &Palette) {
  dot::DotAttributes Attributes;
  if (Edge.Label)
    Attributes.push_back({"label", *Edge.Label});
  else if (Edge.Kind != cfg::CfgEdgeKind::Flow)
    Attributes.push_back({"label", std::string(cfg::toString(Edge.Kind))});
  if (!Reachable) {
    Attributes.push_back({"style", "dashed"});
    Attributes.push_back({"color", Palette.MutedTextColor});
  }
  return Attributes;
}

static void writeDefaults(dot::DotWriter &Writer, const DotPalette &Palette) {
  Writer.writeAttribute("rankdir", "TB");
  Writer.writeAttribute("bgcolor", Palette.BackgroundColor);
  Writer.writeDefaultAttributes(dot::DotElement::Graph,
                                {{"fontname", "Arial"},
                                 {"fontcolor", Palette.MutedTextColor},
                                 {"pad", "0.2"},
                                 {"nodesep", "0.25"},
                                 {"ranksep", "0.45"}});
  Writer.writeDefaultAttributes(dot::DotElement::Node,
                                {{"shape", "box"},
                                 {"style", "rounded,filled"},
                                 {"fontname", "Arial"},
                                 {"fontsize", "10"},
                                 {"fontcolor", Palette.TextColor},
                                 {"color", Palette.BorderColor},
                                 {"margin", "0.12,0.08"}});
  Writer.writeDefaultAttributes(dot::DotElement::Edge,
                                {{"color", Palette.EdgeColor},
                                 {"penwidth", "1.0"},
                                 {"arrowsize", "0.65"},
                                 {"fontname", "Arial"},
                                 {"fontsize", "9"},
                                 {"fontcolor", Palette.MutedTextColor}});
}

static void writeFunctionGraph(dot::DotWriter &Writer,
                               const DotPalette &Palette,
                               const ast::FunctionAstGraph &FunctionGraph,
                               std::size_t FunctionIndex) {
  Writer.beginSubgraph(clusterId(FunctionIndex));
  const source::FunctionInfo &Function = FunctionGraph.function();
  Writer.writeAttribute("label",
                        Function.QualifiedName + " : " + Function.Signature);
  Writer.writeAttribute("color", Palette.BorderColor);
  Writer.writeAttribute("fontcolor", Palette.MutedTextColor);
  Writer.writeAttribute("style", "rounded");
  Writer.writeAttribute("margin", "12");

  const ast::AstGraph &Graph = FunctionGraph.graph();
  for (const ast::AstGraph::Node &Node : Graph.nodes()) {
    Writer.writeNode(nodeId(FunctionIndex, Node.Id),
                     {{"label", Node.Payload.Display.Label},
                      {"fillcolor", nodeColor(Palette, Node.Payload.Kind)}});
  }

  for (const ast::AstGraph::Edge &Edge : Graph.edges()) {
    dot::DotAttributes Attributes;
    if (Edge.Payload.Role != ast::AstEdgeRole::Child)
      Attributes.push_back(
          {"label", std::string(ast::toString(Edge.Payload.Role))});

    Writer.writeEdge(nodeId(FunctionIndex, Edge.Source),
                     nodeId(FunctionIndex, Edge.Target), Attributes);
  }

  Writer.endSubgraph();
}

static void writeFunctionGraph(dot::DotWriter &Writer,
                               const DotPalette &Palette,
                               const cfg::FunctionCfgGraph &FunctionGraph,
                               std::size_t FunctionIndex) {
  Writer.beginSubgraph(clusterId(FunctionIndex));
  const source::FunctionInfo &Function = FunctionGraph.function();
  Writer.writeAttribute("label",
                        Function.QualifiedName + " : " + Function.Signature);
  Writer.writeAttribute("color", Palette.BorderColor);
  Writer.writeAttribute("fontcolor", Palette.MutedTextColor);
  Writer.writeAttribute("style", "rounded");
  Writer.writeAttribute("margin", "12");

  const cfg::CfgGraph &Graph = FunctionGraph.graph();
  const std::vector<bool> Reachable = cfg::computeReachableNodes(FunctionGraph);
  for (const cfg::CfgGraph::Node &Node : Graph.nodes()) {
    Writer.writeNode(
        nodeId(FunctionIndex, Node.Id),
        cfgNodeAttributes(Palette, Node.Payload, Reachable[Node.Id.value()]));
  }

  for (const cfg::CfgGraph::Edge &Edge : Graph.edges()) {
    Writer.writeEdge(
        nodeId(FunctionIndex, Edge.Source), nodeId(FunctionIndex, Edge.Target),
        cfgEdgeAttributes(Edge.Payload, Reachable[Edge.Source.value()],
                          Palette));
  }

  Writer.endSubgraph();
}

DotPalette makeDarkDotPalette() {
  return DotPalette{
      "transparent", "#F3F4F6", "#D4D4D4", "#4B5563",
      "#6B7280",     "#1E3A5F", "#1F4D3A", "#4A3B16",
  };
}

GraphDotSerializer::GraphDotSerializer()
    : GraphDotSerializer(makeDarkDotPalette()) {}

GraphDotSerializer::GraphDotSerializer(DotPalette Palette)
    : Palette(std::move(Palette)) {}

bool GraphDotSerializer::writeAst(
    std::ostream &Output, const ast::FunctionAstGraphs &FunctionGraphs) const {
  dot::DotWriter Writer(Output, "AST");
  writeDefaults(Writer, Palette);

  for (std::size_t Index = 0; Index < FunctionGraphs.size(); ++Index)
    writeFunctionGraph(Writer, Palette, FunctionGraphs[Index], Index);

  Writer.finish();
  return Writer.good();
}

bool GraphDotSerializer::writeCfg(
    std::ostream &Output, const cfg::FunctionCfgGraphs &FunctionGraphs) const {
  dot::DotWriter Writer(Output, "CFG");
  writeDefaults(Writer, Palette);

  for (std::size_t Index = 0; Index < FunctionGraphs.size(); ++Index)
    writeFunctionGraph(Writer, Palette, FunctionGraphs[Index], Index);

  Writer.finish();
  return Writer.good();
}

} // namespace mycpptools::presentation
