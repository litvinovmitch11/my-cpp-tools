#include "mycpptools/clang/ast_graph_builder.hpp"

#include "support/test_support.hpp"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace ast = mycpptools::ast;
namespace frontend = mycpptools::clang_frontend;

static std::string fixturePath(const std::string &Name) {
  return std::string(MYCPPTOOLS_AST_FIXTURE_DIR) + '/' + Name;
}

static std::string readFile(const std::string &Path) {
  std::ifstream Input(Path);
  return std::string(std::istreambuf_iterator<char>(Input),
                     std::istreambuf_iterator<char>());
}

static frontend::AstGraphBuildStatus
build(const std::vector<std::string> &Sources, ast::FunctionAstGraphs &Graphs) {
  clang::tooling::FixedCompilationDatabase Compilations(
      ".", {"-std=c++20", "-fsyntax-only"});
  clang::tooling::ClangTool Tool(Compilations, Sources);
  return frontend::buildFunctionAstGraphs(Tool, Graphs);
}

static void mapsMacroExpansionRangesToTheMainFile() {
  const std::string Path = fixturePath("macro_expansion.cpp");
  const std::string Source = readFile(Path);
  const std::size_t Invocation = Source.find("DEFINE_FUNCTION(expanded)");
  ast::FunctionAstGraphs Graphs;

  MYCPPTOOLS_EXPECT_EQ(build({Path}, Graphs),
                       frontend::AstGraphBuildStatus::Success);
  MYCPPTOOLS_EXPECT_EQ(Graphs.size(), 1U);
  MYCPPTOOLS_EXPECT_EQ(Graphs[0].function().Name, "expanded");
  MYCPPTOOLS_EXPECT_TRUE(Graphs[0].function().Definition.has_value());
  MYCPPTOOLS_EXPECT_EQ(Graphs[0].function().Definition->BeginOffset,
                       Invocation);
  MYCPPTOOLS_EXPECT_EQ(Graphs[0].function().Definition->EndOffset,
                       Invocation + std::string("DEFINE_FUNCTION").size());

  const ast::AstGraph &Graph = Graphs[0].graph();
  const ast::AstNode &Root = Graph.node(*Graph.root()).Payload;
  MYCPPTOOLS_EXPECT_TRUE(Root.Metadata.Source.has_value());
  MYCPPTOOLS_EXPECT_EQ(Root.Metadata.Source->BeginOffset, Invocation);
  MYCPPTOOLS_EXPECT_EQ(Root.Metadata.Source->EndOffset,
                       Graphs[0].function().Definition->EndOffset);
}

static void collectsEveryMainFileInOneToolRun() {
  const std::string FirstPath = fixturePath("multi_file_first.cpp");
  const std::string SecondPath = fixturePath("multi_file_second.cpp");
  ast::FunctionAstGraphs Graphs;

  MYCPPTOOLS_EXPECT_EQ(build({FirstPath, SecondPath}, Graphs),
                       frontend::AstGraphBuildStatus::Success);
  MYCPPTOOLS_EXPECT_EQ(Graphs.size(), 2U);
  MYCPPTOOLS_EXPECT_EQ(Graphs[0].function().Name, "first");
  MYCPPTOOLS_EXPECT_EQ(Graphs[1].function().Name, "second");
  MYCPPTOOLS_EXPECT_TRUE(Graphs[0].function().Id != Graphs[1].function().Id);
  MYCPPTOOLS_EXPECT_EQ(Graphs[0].function().Definition->BeginOffset, 0U);
  MYCPPTOOLS_EXPECT_EQ(Graphs[1].function().Definition->BeginOffset, 0U);
}

int main() {
  return mycpptools::test::runTests({
      {"maps macro expansion ranges to the main file",
       mapsMacroExpansionRangesToTheMainFile},
      {"collects every main file in one tool run",
       collectsEveryMainFileInOneToolRun},
  });
}
