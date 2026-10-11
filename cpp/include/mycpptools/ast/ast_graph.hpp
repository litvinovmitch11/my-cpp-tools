#pragma once

#include "mycpptools/graph/graph.hpp"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include <cstddef>
#include <memory>


// работает на уровне функции
class BuildFunctionAstGraphVisitor
    : public clang::RecursiveASTVisitor<BuildFunctionAstGraphVisitor> {
public:
  BuildFunctionAstGraphVisitor(std::unique_ptr<mycpptools::Graph> Graph,
                               unsigned &NextNodeId);

  bool TraverseDecl(clang::Decl *D);
  bool TraverseStmt(clang::Stmt *S);

private:
  std::unique_ptr<mycpptools::Graph> Grpah;
  size_t &NextNodeId;
  std::vector<size_t> ParentStack;
};
