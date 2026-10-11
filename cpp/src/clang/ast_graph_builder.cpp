#include <mycpptools/clang/ast_graph_builder.hpp>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/Tooling.h"

#include <memory>
#include <vector>

namespace mycpptools::clang_frontend {

namespace {

class FunctionAstGraphBuilder
    : public clang::RecursiveASTVisitor<FunctionAstGraphBuilder> {
public:
  FunctionAstGraphBuilder(mycpptools::FunctionAstGraph &Graph) : Graph(Graph) {}

  bool TraverseDecl(clang::Decl *D) {
    if (!D)
      return true;

    (void)Graph;
    ParentStack.push_back(1);
    const bool Result =
        RecursiveASTVisitor<FunctionAstGraphBuilder>::TraverseDecl(D);
    ParentStack.pop_back();

    return Result;
  }

  bool dataTraverseStmtPre(clang::Stmt *S) {
    (void)Graph;
    (void)S;
    ParentStack.push_back(1);

    return true;
  }

  bool dataTraverseStmtPost(clang::Stmt *S) {
    (void)Graph;
    (void)S;
    ParentStack.pop_back();

    return true;
  }

private:
  mycpptools::FunctionAstGraph &Graph;
  std::vector<size_t> ParentStack;
};

class FunctionAstGraphCollector
    : public clang::RecursiveASTVisitor<FunctionAstGraphCollector> {
public:
  FunctionAstGraphCollector(clang::ASTContext &Context,
                            FunctionAstGraphs &Graphs)
      : Context(Context), Graphs(Graphs) {}

  bool VisitFunctionDecl(clang::FunctionDecl *Function) {
    if (!Function->doesThisDeclarationHaveABody())
      return true;

    clang::SourceManager &Sources = Context.getSourceManager();
    clang::SourceLocation Location =
        Sources.getExpansionLoc(Function->getLocation());
    if (!Sources.isWrittenInMainFile(Location))
      return true;

    FunctionAstGraph Graph(Function->getQualifiedNameAsString());

    FunctionAstGraphBuilder Builder(Graph);
    if (!Builder.TraverseDecl(Function))
      return false;

    Graphs.push_back(std::move(Graph));
    return true;
  }

private:
  clang::ASTContext &Context;
  FunctionAstGraphs &Graphs;
};

class FunctionAstGraphConsumer : public clang::ASTConsumer {
public:
  FunctionAstGraphConsumer(FunctionAstGraphs &Graphs) : Graphs(Graphs) {}

  void HandleTranslationUnit(clang::ASTContext &Context) override {
    FunctionAstGraphCollector Collector(Context, Graphs);
    Collector.TraverseDecl(Context.getTranslationUnitDecl());
  }

private:
  FunctionAstGraphs &Graphs;
};

class FunctionAstGraphConsumerProvider {
public:
  FunctionAstGraphConsumerProvider(FunctionAstGraphs &Graphs)
      : Graphs(Graphs) {}

  std::unique_ptr<clang::ASTConsumer> newASTConsumer() {
    return std::make_unique<FunctionAstGraphConsumer>(Graphs);
  }

private:
  FunctionAstGraphs &Graphs;
};

} // namespace

int buildFunctionAstGraphs(::clang::tooling::ClangTool &Tool,
                           FunctionAstGraphs &Graphs) {
  FunctionAstGraphConsumerProvider ConsumerProvider(Graphs);
  auto ActionFactory =
      ::clang::tooling::newFrontendActionFactory(&ConsumerProvider);
  return Tool.run(ActionFactory.get());
}

} // namespace mycpptools::clang_frontend
