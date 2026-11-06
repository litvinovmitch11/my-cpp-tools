#include "mycpptools/dot/dot_writer.hpp"

#include "support/test_support.hpp"

#include <sstream>
#include <stdexcept>
#include <string>

namespace dot = mycpptools::dot;

static void writesAndEscapesDirectedDot() {
  std::ostringstream Output;
  dot::DotWriter Writer(Output, "AST");
  Writer.writeAttribute("label", "quote\" slash\\ line\ncontrol\x01");
  Writer.writeDefaultAttributes(dot::DotElement::Node, {{"shape", "box"}});
  Writer.beginSubgraph("cluster_0");
  Writer.writeNode("n0", {{"label", "root"}});
  Writer.writeNode("n1");
  Writer.writeEdge("n0", "n1", {{"label", "body"}});
  Writer.endSubgraph();
  Writer.finish();

  const std::string Expected =
      "digraph AST {\n"
      "  label=\"quote\\\" slash\\\\ line\\ncontrol?\";\n"
      "  node [shape=\"box\"];\n"
      "  subgraph cluster_0 {\n"
      "    n0 [label=\"root\"];\n"
      "    n1;\n"
      "    n0 -> n1 [label=\"body\"];\n"
      "  }\n"
      "}\n";
  MYCPPTOOLS_EXPECT_EQ(Output.str(), Expected);
  MYCPPTOOLS_EXPECT_TRUE(Writer.good());
}

static void rejectsInvalidSyntaxRequests() {
  std::ostringstream Output;
  MYCPPTOOLS_EXPECT_THROWS(dot::DotWriter(Output, "digraph"),
                           std::invalid_argument);
  MYCPPTOOLS_EXPECT_THROWS(dot::DotWriter(Output, "9graph"),
                           std::invalid_argument);

  dot::DotWriter Writer(Output, "AST");
  MYCPPTOOLS_EXPECT_THROWS(Writer.writeNode("invalid-id"),
                           std::invalid_argument);
  MYCPPTOOLS_EXPECT_THROWS(
      Writer.writeNode("n0", {{"label", "one"}, {"label", "two"}}),
      std::invalid_argument);
  MYCPPTOOLS_EXPECT_THROWS(Writer.endSubgraph(), std::logic_error);
  Writer.finish();
  MYCPPTOOLS_EXPECT_THROWS(Writer.writeNode("n0"), std::logic_error);
}

static void rejectsFinishingAnOpenSubgraph() {
  std::ostringstream Output;
  dot::DotWriter Writer(Output, "AST");
  Writer.beginSubgraph("cluster_0");
  MYCPPTOOLS_EXPECT_THROWS(Writer.finish(), std::logic_error);
  Writer.endSubgraph();
  Writer.finish();
}

int main() {
  return mycpptools::test::runTests({
      {"writes and escapes directed DOT", writesAndEscapesDirectedDot},
      {"rejects invalid syntax requests", rejectsInvalidSyntaxRequests},
      {"rejects finishing an open subgraph", rejectsFinishingAnOpenSubgraph},
  });
}
