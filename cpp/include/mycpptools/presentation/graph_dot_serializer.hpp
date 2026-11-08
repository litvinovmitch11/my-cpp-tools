#pragma once

#include "mycpptools/ast/ast_graph.hpp"
#include "mycpptools/cfg/cfg_graph.hpp"

#include <iosfwd>
#include <string>

namespace mycpptools::presentation {

struct DotPalette {
  std::string BackgroundColor;
  std::string TextColor;
  std::string MutedTextColor;
  std::string BorderColor;
  std::string EdgeColor;
  std::string PrimaryNodeColor;
  std::string SecondaryNodeColor;
  std::string AccentNodeColor;
};

[[nodiscard]] DotPalette makeDarkDotPalette();

class GraphDotSerializer {
public:
  GraphDotSerializer();
  explicit GraphDotSerializer(DotPalette Palette);

  [[nodiscard]] bool
  writeAst(std::ostream &Output,
           const ast::FunctionAstGraphs &FunctionGraphs) const;

  [[nodiscard]] bool
  writeCfg(std::ostream &Output,
           const cfg::FunctionCfgGraphs &FunctionGraphs) const;

private:
  DotPalette Palette;
};

} // namespace mycpptools::presentation
