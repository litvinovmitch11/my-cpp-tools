#include "mycpptools/clang/ast_graph_builder.hpp"

#include "function_ast_graph_builder.hpp"

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTLambda.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Tooling/Tooling.h"

#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>

namespace mycpptools::clang_frontend {

namespace {

struct AstGraphBuildState {
  ast::FunctionAstGraphs Graphs;
  std::set<source::FunctionId> FunctionIds;
  bool Failed = false;
};

class FunctionAstGraphCollector
    : public clang::RecursiveASTVisitor<FunctionAstGraphCollector> {
public:
  FunctionAstGraphCollector(clang::ASTContext &Context,
                            AstGraphBuildState &State)
      : Context(Context), State(State) {}

  bool VisitFunctionDecl(clang::FunctionDecl *Function) {
    if (!shouldCollect(*Function))
      return true;

    clang::SourceManager &Sources = Context.getSourceManager();
    const clang::SourceLocation Location =
        Sources.getExpansionLoc(Function->getLocation());
    if (!Sources.isWrittenInMainFile(Location))
      return true;

    std::optional<ast::FunctionAstGraph> Graph;
    try {
      Graph = buildAstGraph(*Function, Context);
    } catch (const std::invalid_argument &Error) {
      reportError(*Function, Error.what());
      return false;
    }

    if (!Graph) {
      reportError(*Function, "AST traversal failed");
      return false;
    }

    if (!State.FunctionIds.insert(Graph->function().Id).second)
      return true;

    State.Graphs.push_back(std::move(*Graph));
    return true;
  }

private:
  void reportError(const clang::FunctionDecl &Function, const char *Detail) {
    State.Failed = true;

    std::string Name = Function.getQualifiedNameAsString();
    if (Name.empty())
      Name = "<anonymous>";

    clang::DiagnosticsEngine &Diagnostics = Context.getDiagnostics();
    const unsigned DiagnosticId = Diagnostics.getCustomDiagID(
        clang::DiagnosticsEngine::Error,
        "failed to build AST graph for function '%0': %1");
    Diagnostics.Report(Function.getLocation(), DiagnosticId) << Name << Detail;
  }

  [[nodiscard]] static bool shouldCollect(const clang::FunctionDecl &Function) {
    if (!Function.isThisDeclarationADefinition() || Function.isImplicit() ||
        Function.getTemplateSpecializationKind() ==
            clang::TSK_ImplicitInstantiation)
      return false;

    const auto *Method = llvm::dyn_cast<clang::CXXMethodDecl>(&Function);
    return Method == nullptr || !clang::isLambdaCallOperator(Method);
  }

  clang::ASTContext &Context;
  AstGraphBuildState &State;
};

class FunctionAstGraphConsumer : public clang::ASTConsumer {
public:
  explicit FunctionAstGraphConsumer(AstGraphBuildState &State) : State(State) {}

  void HandleTranslationUnit(clang::ASTContext &Context) override {
    if (State.Failed)
      return;

    FunctionAstGraphCollector Collector(Context, State);
    if (!Collector.TraverseDecl(Context.getTranslationUnitDecl()))
      State.Failed = true;
  }

private:
  AstGraphBuildState &State;
};

class FunctionAstGraphConsumerProvider {
public:
  explicit FunctionAstGraphConsumerProvider(AstGraphBuildState &State)
      : State(State) {}

  std::unique_ptr<clang::ASTConsumer> newASTConsumer() {
    return std::make_unique<FunctionAstGraphConsumer>(State);
  }

private:
  AstGraphBuildState &State;
};

} // namespace

int buildFunctionAstGraphs(clang::tooling::ClangTool &Tool,
                           ast::FunctionAstGraphs &Graphs) {
  AstGraphBuildState State;
  FunctionAstGraphConsumerProvider ConsumerProvider(State);
  auto ActionFactory =
      clang::tooling::newFrontendActionFactory(&ConsumerProvider);
  const int ToolResult = Tool.run(ActionFactory.get());
  if (ToolResult != 0)
    return ToolResult;
  if (State.Failed)
    return 1;

  Graphs = std::move(State.Graphs);
  return 0;
}

} // namespace mycpptools::clang_frontend
