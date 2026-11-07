#pragma once

#include "mycpptools/ast/ast_graph.hpp"

namespace clang {

class Decl;
class DynTypedNode;
class Stmt;

} // namespace clang

namespace mycpptools::clang_frontend {

[[nodiscard]] ast::AstNodeKind classifyAstNode(const clang::Decl &Declaration);
[[nodiscard]] ast::AstNodeKind classifyAstNode(const clang::Stmt &Statement);

[[nodiscard]] ast::AstEdgeRole
classifyAstEdge(const clang::DynTypedNode &Parent, const clang::Decl &Child);
[[nodiscard]] ast::AstEdgeRole
classifyAstEdge(const clang::DynTypedNode &Parent, const clang::Stmt &Child);

} // namespace mycpptools::clang_frontend
