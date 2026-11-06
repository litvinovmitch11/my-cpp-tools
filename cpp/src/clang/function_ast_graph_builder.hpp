#pragma once

#include "mycpptools/ast/ast_graph.hpp"

#include <optional>

namespace clang {

class ASTContext;
class FunctionDecl;

} // namespace clang

namespace mycpptools::clang_frontend {

[[nodiscard]] std::optional<ast::FunctionAstGraph>
buildAstGraph(clang::FunctionDecl &Function, clang::ASTContext &Context);

} // namespace mycpptools::clang_frontend
