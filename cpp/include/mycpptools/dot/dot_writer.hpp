#pragma once

#include <cstddef>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace mycpptools::dot {

enum class DotElement {
  Graph,
  Node,
  Edge,
};

struct DotAttribute {
  std::string Name;
  std::string Value;
};

using DotAttributes = std::vector<DotAttribute>;

class DotWriter {
public:
  DotWriter(std::ostream &Output, std::string_view GraphId);

  DotWriter(const DotWriter &) = delete;
  DotWriter &operator=(const DotWriter &) = delete;
  DotWriter(DotWriter &&) = delete;
  DotWriter &operator=(DotWriter &&) = delete;

  void writeAttribute(std::string_view Name, std::string_view Value);
  void writeDefaultAttributes(DotElement Element,
                              const DotAttributes &Attributes);

  void beginSubgraph(std::string_view SubgraphId);
  void endSubgraph();

  void writeNode(std::string_view NodeId, const DotAttributes &Attributes = {});
  void writeEdge(std::string_view SourceId, std::string_view TargetId,
                 const DotAttributes &Attributes = {});

  void finish();

  [[nodiscard]] bool good() const noexcept;

private:
  void ensureOpen() const;
  void writeIndentation();
  void writeAttributes(const DotAttributes &Attributes);
  void writeQuoted(std::string_view Value);

  static void validateIdentifier(std::string_view Identifier);
  static void validateAttributes(const DotAttributes &Attributes);

  std::ostream &Output;
  std::size_t IndentationLevel = 1;
  bool Finished = false;
};

} // namespace mycpptools::dot
