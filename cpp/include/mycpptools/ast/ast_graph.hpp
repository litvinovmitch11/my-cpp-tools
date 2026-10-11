#pragma once

#include <string>
#include <vector>

namespace mycpptools {

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

using FunctionAstGraphs = std::vector<FunctionAstGraph>;

} // namespace mycpptools
