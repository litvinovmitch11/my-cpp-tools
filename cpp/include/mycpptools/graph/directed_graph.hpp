#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mycpptools::graph {

template <typename NodePayload, typename EdgePayload> class DirectedGraph {
public:
  class NodeId {
  public:
    explicit constexpr NodeId(std::size_t Value) noexcept : Value(Value) {}

    [[nodiscard]] constexpr std::size_t value() const noexcept { return Value; }

    friend constexpr bool operator==(NodeId Left, NodeId Right) noexcept {
      return Left.Value == Right.Value;
    }

    friend constexpr bool operator!=(NodeId Left, NodeId Right) noexcept {
      return !(Left == Right);
    }

    friend constexpr bool operator<(NodeId Left, NodeId Right) noexcept {
      return Left.Value < Right.Value;
    }

  private:
    std::size_t Value;
  };

  class EdgeId {
  public:
    explicit constexpr EdgeId(std::size_t Value) noexcept : Value(Value) {}

    [[nodiscard]] constexpr std::size_t value() const noexcept { return Value; }

    friend constexpr bool operator==(EdgeId Left, EdgeId Right) noexcept {
      return Left.Value == Right.Value;
    }

    friend constexpr bool operator!=(EdgeId Left, EdgeId Right) noexcept {
      return !(Left == Right);
    }

    friend constexpr bool operator<(EdgeId Left, EdgeId Right) noexcept {
      return Left.Value < Right.Value;
    }

  private:
    std::size_t Value;
  };

  struct Node {
    NodeId Id;
    NodePayload Payload;
  };

  struct Edge {
    EdgeId Id;
    NodeId Source;
    NodeId Target;
    EdgePayload Payload;
  };

  [[nodiscard]] NodeId addNode(NodePayload Payload) {
    const NodeId Id(Nodes.size());
    Nodes.push_back(Node{Id, std::move(Payload)});
    try {
      OutgoingEdges.emplace_back();
      try {
        IncomingEdges.emplace_back();
      } catch (...) {
        OutgoingEdges.pop_back();
        throw;
      }
    } catch (...) {
      Nodes.pop_back();
      throw;
    }
    return Id;
  }

  [[nodiscard]] EdgeId addEdge(NodeId Source, NodeId Target,
                               EdgePayload Payload) {
    validateNodeId(Source);
    validateNodeId(Target);

    const EdgeId Id(Edges.size());
    Edges.push_back(Edge{Id, Source, Target, std::move(Payload)});
    try {
      OutgoingEdges[Source.value()].push_back(Id);
      try {
        IncomingEdges[Target.value()].push_back(Id);
      } catch (...) {
        OutgoingEdges[Source.value()].pop_back();
        throw;
      }
    } catch (...) {
      Edges.pop_back();
      throw;
    }
    return Id;
  }

  void setRoot(NodeId Node) {
    validateNodeId(Node);
    Root = Node;
  }

  [[nodiscard]] std::optional<NodeId> root() const noexcept { return Root; }

  [[nodiscard]] const Node &node(NodeId Id) const {
    validateNodeId(Id);
    return Nodes[Id.value()];
  }

  [[nodiscard]] const Edge &edge(EdgeId Id) const {
    validateEdgeId(Id);
    return Edges[Id.value()];
  }

  [[nodiscard]] const std::vector<Node> &nodes() const noexcept {
    return Nodes;
  }

  [[nodiscard]] const std::vector<Edge> &edges() const noexcept {
    return Edges;
  }

  [[nodiscard]] const std::vector<EdgeId> &outgoingEdges(NodeId Id) const {
    validateNodeId(Id);
    return OutgoingEdges[Id.value()];
  }

  [[nodiscard]] const std::vector<EdgeId> &incomingEdges(NodeId Id) const {
    validateNodeId(Id);
    return IncomingEdges[Id.value()];
  }

  [[nodiscard]] std::size_t nodeCount() const noexcept { return Nodes.size(); }
  [[nodiscard]] std::size_t edgeCount() const noexcept { return Edges.size(); }
  [[nodiscard]] bool empty() const noexcept { return Nodes.empty(); }

private:
  void validateNodeId(NodeId Id) const {
    if (Id.value() >= Nodes.size())
      throw std::out_of_range("directed graph node ID is out of range");
  }

  void validateEdgeId(EdgeId Id) const {
    if (Id.value() >= Edges.size())
      throw std::out_of_range("directed graph edge ID is out of range");
  }

  std::vector<Node> Nodes;
  std::vector<Edge> Edges;
  std::vector<std::vector<EdgeId>> OutgoingEdges;
  std::vector<std::vector<EdgeId>> IncomingEdges;
  std::optional<NodeId> Root;
};

} // namespace mycpptools::graph
