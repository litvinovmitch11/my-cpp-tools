#pragma once

#include <cstddef>
#include <string>

namespace mycpptools {

class Node {
public:
  Node() : Id(0), Meta("empty meta") {}
  Node(size_t Id) : Id(Id), Meta("empty meta") {}
  Node(size_t Id, const std::string &Meta) : Id(Id), Meta(Meta) {}

  size_t getId() { return Id; }
  std::string getMeta() { return Meta; }

private:
  size_t Id;
  std::string Meta;
};

class FunctionAstGraph {
public:
  FunctionAstGraph(const std::string &Name) : Name(Name) {}
  bool addNode();
  bool addNeighboor();
  std::string getName() const { return Name; };

private:
  std::string Name;
  // std::unique_ptr<Node> Start;
  // std::unordered_map<size_t, std::unordered_set<std::unique_ptr<Node>>>
  //     Neighboors;
};

// void walk(const FunctionAstGraph &Graph);

} // namespace mycpptools
