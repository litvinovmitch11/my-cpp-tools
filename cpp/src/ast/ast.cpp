// #include "clang/AST/ASTConsumer.h"
// #include "clang/AST/Decl.h"
// #include "clang/AST/Expr.h"
// #include "clang/AST/RecursiveASTVisitor.h"
// #include "clang/Basic/SourceManager.h"
// #include "clang/Frontend/CompilerInstance.h"
// #include "clang/Frontend/FrontendAction.h"
// #include "clang/Tooling/CommonOptionsParser.h"
// #include "clang/Tooling/Tooling.h"
// #include "llvm/ADT/StringRef.h"
// #include "llvm/Support/raw_ostream.h"

// #include <memory>
// #include <string>
// #include <vector>

// #include "mycpptools/ast/ast.hpp"
// #include "mycpptools/graph/graph.hpp"

// using namespace clang;

// static std::string escapeDotLabel(llvm::StringRef Text) {
//   std::string Result;
//   for (char C : Text) {
//     switch (C) {
//     case '\\':
//     case '"':
//       Result += '\\';
//       Result += C;
//       break;
//     case '\n':
//       Result += "\\n";
//       break;
//     case '\r':
//       break;
//     default:
//       Result += C;
//       break;
//     }
//   }
//   return Result;
// }

// mycpptools::Graph grpahFromLLVMAst() {}

// class PrintAstVisitor : public RecursiveASTVisitor<PrintAstVisitor> {
// public:
//   PrintAstVisitor(ASTContext &Context, llvm::raw_ostream &OS)
//       : Context(Context), OS(OS) {}

//   bool VisitFunctionDecl(FunctionDecl *Function) {
//     if (!Function->doesThisDeclarationHaveABody())
//       return true;

//     SourceManager &Sources = Context.getSourceManager();
//     SourceLocation Location = Sources.getExpansionLoc(Function->getLocation());
//     if (!Sources.isWrittenInMainFile(Location))
//       return true;

//     const unsigned ClusterId = NextClusterId++;
//     std::string FunctionName = Function->getQualifiedNameAsString();
//     if (FunctionName.empty())
//       FunctionName = "<anonymous>";

//     OS << "  subgraph cluster_" << ClusterId << " {\n"
//        << "    label=\"" << escapeDotLabel(FunctionName) << "\";\n"
//        << "    color=\"#4B5563\";\n"
//        << "    fontcolor=\"#D4D4D4\";\n"
//        << "    style=\"rounded\";\n"
//        << "    margin=12;\n";

//     PrintFunctionAstVisitor Visitor(OS, NextNodeId);
//     Visitor.TraverseDecl(Function);

//     OS << "  }\n";
//     return true;
//   }

// private:
//   ASTContext &Context;
//   llvm::raw_ostream &OS;
//   unsigned NextNodeId = 0;
//   unsigned NextClusterId = 0;
// };

// class PrintAstConsumer : public clang::ASTConsumer {
// public:
//   explicit PrintAstConsumer(ASTContext &Context)
//       : Visitor(Context, llvm::outs()) {}

//   void HandleTranslationUnit(clang::ASTContext &Context) override {
//     llvm::outs() << "digraph AST {\n"
//                  << "  rankdir=TB;\n"
//                  << "  bgcolor=\"transparent\";\n"
//                  << "  graph [fontname=\"Arial\", fontcolor=\"#D4D4D4\", "
//                     "pad=0.2, nodesep=0.25, ranksep=0.45];\n"
//                  << "  node [shape=box, style=\"rounded,filled\", "
//                     "fontname=\"Arial\", fontsize=10, "
//                     "fontcolor=\"#F3F4F6\", color=\"#4B5563\", "
//                     "margin=\"0.12,0.08\"];\n"
//                  << "  edge [color=\"#6B7280\", penwidth=1.0, "
//                     "arrowsize=0.65];\n";
//     Visitor.TraverseDecl(Context.getTranslationUnitDecl());
//     llvm::outs() << "}\n";
//   }

// private:
//   PrintAstVisitor Visitor;
// };

// class BuildAstGraphFrontendAction : public clang::ASTFrontendAction {
// protected:
//   std::unique_ptr<clang::ASTConsumer>
//   CreateASTConsumer(clang::CompilerInstance &Compiler,
//                     llvm::StringRef) override {
//     return std::make_unique<PrintAstConsumer>(Compiler.getASTContext());
//   }
// };
