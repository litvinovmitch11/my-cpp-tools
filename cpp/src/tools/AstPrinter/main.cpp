#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;
using namespace llvm;

class PrintAstVisitor : public RecursiveASTVisitor<PrintAstVisitor> {
public:
  explicit PrintAstVisitor(ASTContext *Context) : Context(Context) {}

  bool TraverseDecl(Decl *D) {
    // int id = addNode(D);
    // if (!stack.empty())
    //   addEdge(stack.back(), id);
    // stack.push_back(id);
    llvm::outs() << "Found decl\n";
    bool ok = clang::RecursiveASTVisitor<PrintAstVisitor>::TraverseDecl(D);
    // stack.pop_back();
    return ok;
  }

  bool TraverseStmt(Stmt *S) {
    int id = addNode(S);
    // if (!stack.empty())
    //   addEdge(stack.back(), id);
    stack.push_back(id);
    llvm::outs() << "Found stmt\n";
    bool ok = clang::RecursiveASTVisitor<PrintAstVisitor>::TraverseStmt(S);
    stack.pop_back();
    return ok;
  }

private:
  struct NodeInfo {
    int id;
    std::string label;
    std::string color;
  };

  void pushNode(std::string label, std::string color) {
    int id = nextId++;
    nodes.push_back({id, std::move(label), std::move(color)});
    if (!stack.empty())
      edges.emplace_back(stack.back(), id);
    stack.push_back(id);
  }

  ASTContext *Context;
  std::vector<NodeInfo> nodes;
  std::vector<std::pair<int, int>> edges;
  std::vector<int> stack;
  int nextId = 0;
};

class PrintAstConsumer : public clang::ASTConsumer {
public:
  explicit PrintAstConsumer(ASTContext *Context) : Visitor(Context) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }

private:
  PrintAstVisitor Visitor;
};

class PrintAstFrontendAction : public clang::ASTFrontendAction {
public:
  virtual std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &Compiler,
                    llvm::StringRef InFile) override {
    return std::make_unique<PrintAstConsumer>(&Compiler.getASTContext());
  }
};

static cl::OptionCategory AstPrinterCategory("ast-printer options");

int main(int argc, const char **argv) {
  auto ExpectedParser =
      tooling::CommonOptionsParser::create(argc, argv, AstPrinterCategory);
  if (!ExpectedParser) {
    llvm::errs() << llvm::toString(ExpectedParser.takeError());
    return 1;
  }
  tooling::CommonOptionsParser &OptionsParser = ExpectedParser.get();
  tooling::ClangTool Tool(OptionsParser.getCompilations(),
                          OptionsParser.getSourcePathList());
  return Tool.run(
      tooling::newFrontendActionFactory<PrintAstFrontendAction>().get());
}
