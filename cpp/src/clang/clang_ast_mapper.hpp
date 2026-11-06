#pragma once

#include "mycpptools/ast/ast_graph.hpp"

#include <optional>
#include <string>

namespace clang {

class ASTContext;
class Decl;
class DynTypedNode;
class FunctionDecl;
class SourceRange;
class Stmt;

} // namespace clang

namespace mycpptools::clang_frontend {

class ClangAstMapper {
public:
  explicit ClangAstMapper(clang::ASTContext &Context);

  [[nodiscard]] ast::AstNode makeNode(const clang::Decl &Declaration) const;
  [[nodiscard]] ast::AstNode makeNode(const clang::Stmt &Statement) const;

  [[nodiscard]] ast::AstEdgeRole edgeRole(const clang::DynTypedNode &Parent,
                                          const clang::Decl &Child) const;
  [[nodiscard]] ast::AstEdgeRole edgeRole(const clang::DynTypedNode &Parent,
                                          const clang::Stmt &Child) const;

  [[nodiscard]] ast::FunctionInfo
  makeFunctionInfo(const clang::FunctionDecl &Function) const;

private:
  [[nodiscard]] std::optional<ast::SourceRange>
  sourceRange(clang::SourceRange Range) const;
  [[nodiscard]] std::string sourceText(clang::SourceRange Range) const;

  clang::ASTContext &Context;
};

} // namespace mycpptools::clang_frontend
