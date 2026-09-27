#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <memory>
#include <string>
#include <vector>

using namespace clang;
using namespace llvm;

static std::string escapeDotLabel(llvm::StringRef Text) {
  std::string Result;
  for (char C : Text) {
    switch (C) {
    case '\\':
    case '"':
      Result += '\\';
      Result += C;
      break;
    case '\n':
      Result += "\\n";
      break;
    case '\r':
      break;
    default:
      Result += C;
      break;
    }
  }
  return Result;
}

namespace {

class PrintFunctionAstVisitor
    : public RecursiveASTVisitor<PrintFunctionAstVisitor> {
public:
  PrintFunctionAstVisitor(llvm::raw_ostream &OS, unsigned &NextNodeId)
      : OS(OS), NextNodeId(NextNodeId) {}

  bool TraverseDecl(Decl *D) {
    if (!D || D->isImplicit())
      return true;

    const unsigned Id = addNode(labelForDecl(D), "#DCEBFA");
    ParentStack.push_back(Id);
    const bool Result =
        RecursiveASTVisitor<PrintFunctionAstVisitor>::TraverseDecl(D);
    ParentStack.pop_back();
    return Result;
  }

  bool TraverseStmt(Stmt *S) {
    if (!S)
      return true;

    const char *Color = isa<Expr>(S) ? "#FFF2CC" : "#E2F0D9";
    const unsigned Id = addNode(labelForStmt(S), Color);
    ParentStack.push_back(Id);
    const bool Result =
        RecursiveASTVisitor<PrintFunctionAstVisitor>::TraverseStmt(S);
    ParentStack.pop_back();
    return Result;
  }

private:
  unsigned addNode(llvm::StringRef Label, llvm::StringRef Color) {
    const unsigned Id = NextNodeId++;
    OS << "    n" << Id << " [label=\"" << escapeDotLabel(Label)
       << "\", fillcolor=\"" << Color << "\"];\n";

    if (!ParentStack.empty())
      OS << "    n" << ParentStack.back() << " -> n" << Id << ";\n";

    return Id;
  }

  static std::string labelForDecl(const Decl *D) {
    std::string Label = D->getDeclKindName();
    if (const auto *Named = dyn_cast<NamedDecl>(D)) {
      const std::string Name = Named->getNameAsString();
      if (!Name.empty()) {
        Label += " ";
        Label += Name;
      }
    }
    return Label;
  }

  static std::string labelForStmt(const Stmt *S) {
    std::string Label = S->getStmtClassName();

    if (const auto *Literal = dyn_cast<IntegerLiteral>(S)) {
      std::string Value;
      llvm::raw_string_ostream Stream(Value);
      Stream << Literal->getValue();
      Label += " ";
      Label += Stream.str();
    } else if (const auto *Reference = dyn_cast<DeclRefExpr>(S)) {
      Label += " ";
      Label += Reference->getDecl()->getNameAsString();
    } else if (const auto *Binary = dyn_cast<BinaryOperator>(S)) {
      Label += " ";
      Label += Binary->getOpcodeStr();
    } else if (const auto *Unary = dyn_cast<UnaryOperator>(S)) {
      Label += " ";
      Label += UnaryOperator::getOpcodeStr(Unary->getOpcode());
    }

    return Label;
  }

  llvm::raw_ostream &OS;
  unsigned &NextNodeId;
  std::vector<unsigned> ParentStack;
};

class PrintAstVisitor : public RecursiveASTVisitor<PrintAstVisitor> {
public:
  PrintAstVisitor(ASTContext &Context, llvm::raw_ostream &OS)
      : Context(Context), OS(OS) {}

  bool VisitFunctionDecl(FunctionDecl *Function) {
    if (!Function->doesThisDeclarationHaveABody())
      return true;

    SourceManager &Sources = Context.getSourceManager();
    SourceLocation Location = Sources.getExpansionLoc(Function->getLocation());
    if (!Sources.isWrittenInMainFile(Location))
      return true;

    const unsigned ClusterId = NextClusterId++;
    std::string FunctionName = Function->getQualifiedNameAsString();
    if (FunctionName.empty())
      FunctionName = "<anonymous>";

    OS << "  subgraph cluster_" << ClusterId << " {\n"
       << "    label=\"" << escapeDotLabel(FunctionName) << "\";\n"
       << "    color=\"#CBD5E1\";\n";

    PrintFunctionAstVisitor Visitor(OS, NextNodeId);
    Visitor.TraverseDecl(Function);

    OS << "  }\n";
    return true;
  }

private:
  ASTContext &Context;
  llvm::raw_ostream &OS;
  unsigned NextNodeId = 0;
  unsigned NextClusterId = 0;
};

class PrintAstConsumer : public clang::ASTConsumer {
public:
  explicit PrintAstConsumer(ASTContext &Context)
      : Visitor(Context, llvm::outs()) {}

  void HandleTranslationUnit(clang::ASTContext &Context) override {
    llvm::outs() << "digraph AST {\n"
                 << "  rankdir=TB;\n"
                 << "  graph [fontname=\"DejaVu Sans Mono\"];\n"
                 << "  node [shape=box, style=filled, "
                    "fontname=\"DejaVu Sans Mono\", "
                    "fontsize=10];\n";
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    llvm::outs() << "}\n";
  }

private:
  PrintAstVisitor Visitor;
};

class PrintAstFrontendAction : public clang::ASTFrontendAction {
public:
  std::unique_ptr<clang::ASTConsumer>
  CreateASTConsumer(clang::CompilerInstance &Compiler,
                    llvm::StringRef) override {
    return std::make_unique<PrintAstConsumer>(Compiler.getASTContext());
  }
};

cl::OptionCategory AstPrinterCategory("ast-printer options");

} // namespace

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
