#include "mycpptools/dot/dot_writer.hpp"

#include <array>
#include <ostream>
#include <stdexcept>

namespace mycpptools::dot {

[[nodiscard]] static bool isAsciiLetter(unsigned char Character) noexcept {
  return (Character >= 'A' && Character <= 'Z') ||
         (Character >= 'a' && Character <= 'z');
}

[[nodiscard]] static bool isIdentifierStart(unsigned char Character) noexcept {
  return isAsciiLetter(Character) || Character == '_';
}

[[nodiscard]] static bool
isIdentifierContinuation(unsigned char Character) noexcept {
  return isAsciiLetter(Character) || (Character >= '0' && Character <= '9') ||
         Character == '_';
}

[[nodiscard]] static char toAsciiLower(unsigned char Character) noexcept {
  return Character >= 'A' && Character <= 'Z'
             ? static_cast<char>(Character - 'A' + 'a')
             : static_cast<char>(Character);
}

[[nodiscard]] static bool isReservedKeyword(std::string_view Identifier) {
  constexpr std::array<std::string_view, 6> Keywords{
      "strict", "graph", "digraph", "node", "edge", "subgraph"};

  for (const std::string_view Keyword : Keywords) {
    if (Identifier.size() != Keyword.size())
      continue;

    bool Matches = true;
    for (std::size_t Index = 0; Index < Identifier.size(); ++Index) {
      const auto Character = static_cast<unsigned char>(Identifier[Index]);
      if (toAsciiLower(Character) != Keyword[Index]) {
        Matches = false;
        break;
      }
    }
    if (Matches)
      return true;
  }

  return false;
}

[[nodiscard]] static std::string_view elementName(DotElement Element) {
  switch (Element) {
  case DotElement::Graph:
    return "graph";
  case DotElement::Node:
    return "node";
  case DotElement::Edge:
    return "edge";
  }

  throw std::invalid_argument("unknown DOT element");
}

DotWriter::DotWriter(std::ostream &Output, std::string_view GraphId)
    : Output(Output) {
  validateIdentifier(GraphId);
  Output << "digraph " << GraphId << " {\n";
}

void DotWriter::writeAttribute(std::string_view Name, std::string_view Value) {
  ensureOpen();
  validateIdentifier(Name);

  writeIndentation();
  Output << Name << '=';
  writeQuoted(Value);
  Output << ";\n";
}

void DotWriter::writeDefaultAttributes(DotElement Element,
                                       const DotAttributes &Attributes) {
  ensureOpen();
  validateAttributes(Attributes);
  const std::string_view ElementName = elementName(Element);

  writeIndentation();
  Output << ElementName << ' ';
  writeAttributes(Attributes);
  Output << ";\n";
}

void DotWriter::beginSubgraph(std::string_view SubgraphId) {
  ensureOpen();
  validateIdentifier(SubgraphId);

  writeIndentation();
  Output << "subgraph " << SubgraphId << " {\n";
  ++IndentationLevel;
}

void DotWriter::endSubgraph() {
  ensureOpen();
  if (IndentationLevel == 1)
    throw std::logic_error("no DOT subgraph is open");

  --IndentationLevel;
  writeIndentation();
  Output << "}\n";
}

void DotWriter::writeNode(std::string_view NodeId,
                          const DotAttributes &Attributes) {
  ensureOpen();
  validateIdentifier(NodeId);
  validateAttributes(Attributes);

  writeIndentation();
  Output << NodeId;
  if (!Attributes.empty()) {
    Output << ' ';
    writeAttributes(Attributes);
  }
  Output << ";\n";
}

void DotWriter::writeEdge(std::string_view SourceId, std::string_view TargetId,
                          const DotAttributes &Attributes) {
  ensureOpen();
  validateIdentifier(SourceId);
  validateIdentifier(TargetId);
  validateAttributes(Attributes);

  writeIndentation();
  Output << SourceId << " -> " << TargetId;
  if (!Attributes.empty()) {
    Output << ' ';
    writeAttributes(Attributes);
  }
  Output << ";\n";
}

void DotWriter::finish() {
  ensureOpen();
  if (IndentationLevel != 1)
    throw std::logic_error("cannot finish DOT graph with an open subgraph");

  Finished = true;
  Output << "}\n";
}

bool DotWriter::good() const noexcept { return static_cast<bool>(Output); }

void DotWriter::ensureOpen() const {
  if (Finished)
    throw std::logic_error("DOT graph is already finished");
}

void DotWriter::writeIndentation() {
  for (std::size_t Level = 0; Level < IndentationLevel; ++Level)
    Output << "  ";
}

void DotWriter::writeAttributes(const DotAttributes &Attributes) {
  Output << '[';
  for (std::size_t Index = 0; Index < Attributes.size(); ++Index) {
    if (Index != 0)
      Output << ", ";
    Output << Attributes[Index].Name << '=';
    writeQuoted(Attributes[Index].Value);
  }
  Output << ']';
}

void DotWriter::writeQuoted(std::string_view Value) {
  Output << '"';
  for (const char Character : Value) {
    switch (Character) {
    case '\\':
      Output << "\\\\";
      break;
    case '"':
      Output << "\\\"";
      break;
    case '\n':
      Output << "\\n";
      break;
    case '\r':
      Output << "\\r";
      break;
    case '\t':
      Output << "\\t";
      break;
    default:
      const auto Byte = static_cast<unsigned char>(Character);
      Output << (Byte < 0x20 || Byte == 0x7F ? '?' : Character);
      break;
    }
  }
  Output << '"';
}

void DotWriter::validateIdentifier(std::string_view Identifier) {
  if (Identifier.empty() ||
      !isIdentifierStart(static_cast<unsigned char>(Identifier.front())))
    throw std::invalid_argument("invalid DOT identifier");

  for (const char Character : Identifier.substr(1)) {
    if (!isIdentifierContinuation(static_cast<unsigned char>(Character)))
      throw std::invalid_argument("invalid DOT identifier");
  }

  if (isReservedKeyword(Identifier))
    throw std::invalid_argument("DOT identifier is a reserved keyword");
}

void DotWriter::validateAttributes(const DotAttributes &Attributes) {
  for (std::size_t Index = 0; Index < Attributes.size(); ++Index) {
    validateIdentifier(Attributes[Index].Name);
    for (std::size_t Previous = 0; Previous < Index; ++Previous) {
      if (Attributes[Previous].Name == Attributes[Index].Name)
        throw std::invalid_argument("duplicate DOT attribute");
    }
  }
}

} // namespace mycpptools::dot
