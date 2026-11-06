#include "mycpptools/graph/directed_graph.hpp"

#include "support/test_support.hpp"

#include <stdexcept>
#include <string>

using Graph = mycpptools::graph::DirectedGraph<std::string, int>;

static void preservesInsertionOrderAndAdjacency() {
  Graph Graph;
  const Graph::NodeId First = Graph.addNode("first");
  const Graph::NodeId Second = Graph.addNode("second");
  const Graph::NodeId Third = Graph.addNode("third");
  const Graph::EdgeId FirstEdge = Graph.addEdge(First, Second, 12);
  const Graph::EdgeId SecondEdge = Graph.addEdge(First, Third, 13);

  Graph.setRoot(First);

  MYCPPTOOLS_EXPECT_EQ(First.value(), 0U);
  MYCPPTOOLS_EXPECT_EQ(Second.value(), 1U);
  MYCPPTOOLS_EXPECT_EQ(FirstEdge.value(), 0U);
  MYCPPTOOLS_EXPECT_EQ(Graph.nodeCount(), 3U);
  MYCPPTOOLS_EXPECT_EQ(Graph.edgeCount(), 2U);
  MYCPPTOOLS_EXPECT_EQ(Graph.nodes()[1].Payload, "second");
  MYCPPTOOLS_EXPECT_EQ(Graph.edge(SecondEdge).Payload, 13);
  MYCPPTOOLS_EXPECT_EQ(Graph.outgoingEdges(First).size(), 2U);
  MYCPPTOOLS_EXPECT_EQ(Graph.outgoingEdges(First)[0], FirstEdge);
  MYCPPTOOLS_EXPECT_EQ(Graph.incomingEdges(Second)[0], FirstEdge);
  MYCPPTOOLS_EXPECT_EQ(*Graph.root(), First);
}

static void rejectsUnknownIds() {
  Graph Graph;
  const Graph::NodeId Node = Graph.addNode("node");
  const Graph::NodeId UnknownNode(1);
  const Graph::EdgeId UnknownEdge(0);

  MYCPPTOOLS_EXPECT_THROWS(Graph.node(UnknownNode), std::out_of_range);
  MYCPPTOOLS_EXPECT_THROWS(Graph.edge(UnknownEdge), std::out_of_range);
  MYCPPTOOLS_EXPECT_THROWS(Graph.addEdge(Node, UnknownNode, 1),
                           std::out_of_range);
  MYCPPTOOLS_EXPECT_THROWS(Graph.setRoot(UnknownNode), std::out_of_range);
  MYCPPTOOLS_EXPECT_THROWS(Graph.outgoingEdges(UnknownNode), std::out_of_range);
  MYCPPTOOLS_EXPECT_THROWS(Graph.incomingEdges(UnknownNode), std::out_of_range);
  MYCPPTOOLS_EXPECT_EQ(Graph.nodeCount(), 1U);
  MYCPPTOOLS_EXPECT_EQ(Graph.edgeCount(), 0U);
}

static void startsEmpty() {
  const Graph Graph;
  MYCPPTOOLS_EXPECT_TRUE(Graph.empty());
  MYCPPTOOLS_EXPECT_TRUE(!Graph.root());
  MYCPPTOOLS_EXPECT_EQ(Graph.nodeCount(), 0U);
  MYCPPTOOLS_EXPECT_EQ(Graph.edgeCount(), 0U);
}

int main() {
  return mycpptools::test::runTests({
      {"preserves insertion order and adjacency",
       preservesInsertionOrderAndAdjacency},
      {"rejects unknown IDs", rejectsUnknownIds},
      {"starts empty", startsEmpty},
  });
}
