#include "mycpptools/graph/graph.hpp"
#include <mycpptools/clang/ast_graph_builder.hpp>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/raw_ostream.h"

#include <iostream>
#include <memory>

namespace {

// работает на уровне юнита трансляции (можно диспетчер назвать)
class BuildAstGraphVisitor
    : public clang::RecursiveASTVisitor<BuildAstGraphVisitor> {
public:
  BuildAstGraphVisitor(clang::ASTContext &Context, llvm::raw_ostream &OS)
      : Context(Context), OS(OS) {
    std::cout << "Visitor\n";
  }

  bool VisitFunctionDecl(clang::FunctionDecl *Function) {}

private:
  clang::ASTContext &Context;
  llvm::raw_ostream &OS;
  size_t NextNodeId = 0;
  size_t NextClusterId = 0;
};

// как раз консьюмер
class BuildAstGraphConsumer : public clang::ASTConsumer {
public:
  void HandleTranslationUnit(clang::ASTContext &Context) override {}
};

class BuildAstGraphConsumerFactory {
public:
  BuildAstGraphConsumerFactory(
      std::vector<mycpptools::FunctionAstGraph> &Functions)
      : Functions(Functions) {
    std::cout << "Factory\n";
  }

  std::unique_ptr<clang::ASTConsumer> newASTConsumer() {
    return std::make_unique<BuildAstGraphConsumer>();
  }

private:
  std::vector<mycpptools::FunctionAstGraph> &Functions;
};

} // namespace

namespace mycpptools::clang_frontend {

std::unique_ptr<FrontendActionFactory>
createAstGraphActionFactory(FunctionAstGraphs &Output) {
  BuildAstGraphConsumerFactory ConsumerFactory(Output);
  return clang::tooling::newFrontendActionFactory(&ConsumerFactory);
}

} // namespace mycpptools::clang_frontend
