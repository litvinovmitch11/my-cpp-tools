#include "function_ast_graph_builder.hpp"

#include "clang_ast_mapper.hpp"

#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/Decl.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "llvm/Support/Casting.h"

#include <cassert>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace mycpptools::clang_frontend {

namespace {

class FunctionAstGraphBuilder
    : public clang::RecursiveASTVisitor<FunctionAstGraphBuilder> {
public:
  FunctionAstGraphBuilder(ast::AstGraph &Graph, ClangAstMapper &Mapper)
      : Graph(Graph), Mapper(Mapper) {}

  bool TraverseDecl(clang::Decl *Declaration) {
    if (Declaration == nullptr)
      return Base::TraverseDecl(Declaration);
    if (!ParentStack.empty() && llvm::isa<clang::FunctionDecl>(Declaration))
      return true;
    if (Declaration->isImplicit())
      return Base::TraverseDecl(Declaration);

    const ast::AstNodeId Node = Graph.addNode(Mapper.makeNode(*Declaration));
    attachNode(Node, *Declaration);
    ParentStack.push_back(
        TraversalFrame{Node, clang::DynTypedNode::create(*Declaration), 0});

    const bool Result = Base::TraverseDecl(Declaration);
    ParentStack.pop_back();
    return Result;
  }

  bool dataTraverseStmtPre(clang::Stmt *Statement) {
    assert(Statement != nullptr);

    const ast::AstNodeId Node = Graph.addNode(Mapper.makeNode(*Statement));
    attachNode(Node, *Statement);
    ParentStack.push_back(
        TraversalFrame{Node, clang::DynTypedNode::create(*Statement), 0});
    return true;
  }

  bool dataTraverseStmtPost(clang::Stmt *Statement) {
    assert(Statement != nullptr);
    assert(!ParentStack.empty());
    assert(ParentStack.back().Source.get<clang::Stmt>() == Statement);
    (void)Statement;

    ParentStack.pop_back();
    return true;
  }

private:
  using Base = clang::RecursiveASTVisitor<FunctionAstGraphBuilder>;

  struct TraversalFrame {
    ast::AstNodeId Node;
    clang::DynTypedNode Source;
    std::size_t NextChildOrdinal;
  };

  template <typename ClangNode>
  void attachNode(ast::AstNodeId Node, const ClangNode &Child) {
    if (ParentStack.empty()) {
      Graph.setRoot(Node);
      return;
    }

    TraversalFrame &Parent = ParentStack.back();
    ast::AstEdge Edge{Mapper.edgeRole(Parent.Source, Child),
                      Parent.NextChildOrdinal++};
    (void)Graph.addEdge(Parent.Node, Node, std::move(Edge));
  }

  ast::AstGraph &Graph;
  ClangAstMapper &Mapper;
  std::vector<TraversalFrame> ParentStack;
};

} // namespace

std::optional<ast::FunctionAstGraph>
buildAstGraph(clang::FunctionDecl &Function, clang::ASTContext &Context) {
  ast::AstGraph Graph;
  ClangAstMapper Mapper(Context);
  FunctionAstGraphBuilder Builder(Graph, Mapper);
  if (!Builder.TraverseDecl(&Function))
    return std::nullopt;

  return ast::FunctionAstGraph(Mapper.makeFunctionInfo(Function),
                               std::move(Graph));
}

} // namespace mycpptools::clang_frontend
