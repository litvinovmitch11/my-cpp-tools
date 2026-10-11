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

} // namespace mycpptools
