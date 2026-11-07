#include "clang_ast_mapper.hpp"

#include "clang_ast_classifier.hpp"

#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Index/USRGeneration.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Casting.h"

#include <string>
#include <utility>

namespace mycpptools::clang_frontend {

ClangAstMapper::ClangAstMapper(clang::ASTContext &Context) : Context(Context) {}

ast::AstNode ClangAstMapper::makeNode(const clang::Decl &Declaration) const {
  ast::AstNodeMetadata Metadata;
  Metadata.ConcreteKind = Declaration.getDeclKindName();
  Metadata.Source = sourceRange(Declaration.getSourceRange());

  if (const auto *Named = llvm::dyn_cast<clang::NamedDecl>(&Declaration)) {
    const std::string Name = Named->getNameAsString();
    if (!Name.empty())
      Metadata.Name = Name;

    const std::string QualifiedName = Named->getQualifiedNameAsString();
    if (!QualifiedName.empty())
      Metadata.QualifiedName = QualifiedName;
  }

  if (const auto *Value = llvm::dyn_cast<clang::ValueDecl>(&Declaration))
    Metadata.TypeName = Value->getType().getAsString();

  std::string Label = Declaration.getDeclKindName();
  if (Metadata.Name) {
    Label += " ";
    Label += *Metadata.Name;
  }

  return ast::AstNode{classifyAstNode(Declaration), std::move(Metadata),
                      ast::AstNodeDisplay{std::move(Label)}};
}

ast::AstNode ClangAstMapper::makeNode(const clang::Stmt &Statement) const {
  ast::AstNodeMetadata Metadata;
  Metadata.ConcreteKind = Statement.getStmtClassName();
  Metadata.Source = sourceRange(Statement.getSourceRange());

  if (const auto *Expression = llvm::dyn_cast<clang::Expr>(&Statement))
    Metadata.TypeName = Expression->getType().getAsString();

  const ast::AstNodeKind Kind = classifyAstNode(Statement);
  std::string Label = Statement.getStmtClassName();

  if (const auto *Reference = llvm::dyn_cast<clang::DeclRefExpr>(&Statement)) {
    const std::string Name = Reference->getDecl()->getNameAsString();
    if (!Name.empty()) {
      Metadata.Name = Name;
      Label += " ";
      Label += Name;
    }
  } else if (const auto *Member =
                 llvm::dyn_cast<clang::MemberExpr>(&Statement)) {
    const std::string Name = Member->getMemberDecl()->getNameAsString();
    if (!Name.empty()) {
      Metadata.Name = Name;
      Label += " ";
      Label += Name;
    }
  } else if (const auto *Binary =
                 llvm::dyn_cast<clang::BinaryOperator>(&Statement)) {
    Metadata.Operator = Binary->getOpcodeStr().str();
    Label += " ";
    Label += *Metadata.Operator;
  } else if (const auto *Unary =
                 llvm::dyn_cast<clang::UnaryOperator>(&Statement)) {
    Metadata.Operator =
        clang::UnaryOperator::getOpcodeStr(Unary->getOpcode()).str();
    Label += " ";
    Label += *Metadata.Operator;
  } else if (Kind == ast::AstNodeKind::LiteralExpression) {
    std::string Value = sourceText(Statement.getSourceRange());
    if (!Value.empty()) {
      Metadata.Value = Value;
      Label += " ";
      Label += Value;
    }
  }

  return ast::AstNode{Kind, std::move(Metadata),
                      ast::AstNodeDisplay{std::move(Label)}};
}

ast::AstEdgeRole ClangAstMapper::edgeRole(const clang::DynTypedNode &Parent,
                                          const clang::Decl &Child) const {
  return classifyAstEdge(Parent, Child);
}

ast::AstEdgeRole ClangAstMapper::edgeRole(const clang::DynTypedNode &Parent,
                                          const clang::Stmt &Child) const {
  return classifyAstEdge(Parent, Child);
}

ast::FunctionInfo
ClangAstMapper::makeFunctionInfo(const clang::FunctionDecl &Function) const {
  std::string Name = Function.getNameAsString();
  if (Name.empty())
    Name = "<anonymous>";

  std::string QualifiedName = Function.getQualifiedNameAsString();
  if (QualifiedName.empty())
    QualifiedName = Name;

  std::string Signature = Function.getType().getAsString();

  llvm::SmallString<128> Usr;
  if (clang::index::generateUSRForDecl(&Function, Usr)) {
    const clang::SourceManager &Sources = Context.getSourceManager();
    const clang::SourceLocation Location =
        Sources.getExpansionLoc(Function.getLocation());
    QualifiedName += "@";
    if (Location.isValid())
      QualifiedName += std::to_string(Sources.getFileOffset(Location));
    else
      QualifiedName += "unknown";
    Usr = QualifiedName;
  }

  return ast::FunctionInfo{ast::FunctionId(Usr.str().str()), std::move(Name),
                           std::move(QualifiedName), std::move(Signature),
                           sourceRange(Function.getSourceRange())};
}

std::optional<ast::SourceRange>
ClangAstMapper::sourceRange(clang::SourceRange Range) const {
  if (Range.isInvalid())
    return std::nullopt;

  const clang::SourceManager &Sources = Context.getSourceManager();
  clang::SourceLocation Begin = Sources.getExpansionLoc(Range.getBegin());
  clang::SourceLocation End = Sources.getExpansionLoc(Range.getEnd());
  if (Begin.isInvalid() || End.isInvalid())
    return std::nullopt;

  End =
      clang::Lexer::getLocForEndOfToken(End, 0, Sources, Context.getLangOpts());
  if (End.isInvalid() || Sources.getFileID(Begin) != Sources.getFileID(End) ||
      !Sources.isWrittenInMainFile(Begin) || !Sources.isWrittenInMainFile(End))
    return std::nullopt;

  return ast::SourceRange{Sources.getFileOffset(Begin),
                          Sources.getFileOffset(End)};
}

std::string ClangAstMapper::sourceText(clang::SourceRange Range) const {
  if (Range.isInvalid())
    return {};

  const clang::SourceManager &Sources = Context.getSourceManager();
  bool Invalid = false;
  const llvm::StringRef Text =
      clang::Lexer::getSourceText(clang::CharSourceRange::getTokenRange(Range),
                                  Sources, Context.getLangOpts(), &Invalid);
  return Invalid ? std::string{} : Text.str();
}

} // namespace mycpptools::clang_frontend
