#include "clang_ast_mapper.hpp"

#include "clang_ast_classifier.hpp"

#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/ExprConcepts.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtCXX.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Index/USRGeneration.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/Casting.h"

#include <string>
#include <utility>

namespace mycpptools::clang_frontend {

static bool containsPotentiallyThrowingOperation(const clang::Stmt *Statement,
                                                 clang::ASTContext &Context) {
  if (Statement == nullptr)
    return false;

  if (const auto *Call = llvm::dyn_cast<clang::CallExpr>(Statement))
    return !Call->isUnevaluatedBuiltinCall(Context);
  if (llvm::isa<clang::CXXConstructExpr, clang::CXXNewExpr,
                clang::CXXDeleteExpr, clang::CXXDynamicCastExpr,
                clang::CXXThrowExpr>(Statement))
    return true;
  if (const auto *TypeId = llvm::dyn_cast<clang::CXXTypeidExpr>(Statement))
    return TypeId->isPotentiallyEvaluated();
  if (llvm::isa<clang::OffsetOfExpr, clang::UnaryExprOrTypeTraitExpr,
                clang::ExpressionTraitExpr, clang::CXXUuidofExpr,
                clang::CXXNoexceptExpr, clang::RequiresExpr>(Statement))
    return false;

  for (const clang::Stmt *Child : Statement->children()) {
    if (containsPotentiallyThrowingOperation(Child, Context))
      return true;
  }
  return false;
}

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

  if (llvm::isa<clang::CXXConstructorDecl>(Declaration))
    Metadata.FunctionCategory = ast::FunctionKind::Constructor;
  else if (llvm::isa<clang::CXXDestructorDecl>(Declaration))
    Metadata.FunctionCategory = ast::FunctionKind::Destructor;

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

  if (const auto *Call = llvm::dyn_cast<clang::CallExpr>(&Statement)) {
    if (const clang::FunctionDecl *Callee = Call->getDirectCallee()) {
      Metadata.DoesNotReturn = Callee->isNoReturn();
      const std::string Name = Callee->getQualifiedNameAsString();
      if (!Name.empty()) {
        Metadata.Name = Name;
        Label += " ";
        Label += Name;
      }
    }
  }

  Metadata.IsTransparent =
      llvm::isa<clang::ParenExpr, clang::ConstantExpr, clang::ExprWithCleanups>(
          Statement);
  Metadata.ChildrenAreUnevaluated =
      llvm::isa<clang::OffsetOfExpr, clang::UnaryExprOrTypeTraitExpr,
                clang::ExpressionTraitExpr, clang::CXXUuidofExpr,
                clang::CXXNoexceptExpr, clang::RequiresExpr>(Statement);
  Metadata.MayThrow =
      llvm::isa<clang::CallExpr, clang::CXXConstructExpr, clang::CXXNewExpr,
                clang::CXXDeleteExpr, clang::CXXDynamicCastExpr>(Statement);

  if (const auto *TypeId = llvm::dyn_cast<clang::CXXTypeidExpr>(&Statement)) {
    Metadata.MayThrow = TypeId->isPotentiallyEvaluated();
    Metadata.ChildrenAreUnevaluated = !TypeId->isPotentiallyEvaluated();
  }
  if (const auto *Call = llvm::dyn_cast<clang::CallExpr>(&Statement);
      Call != nullptr && Call->isUnevaluatedBuiltinCall(Context)) {
    Metadata.MayThrow = false;
    Metadata.DoesNotReturn = false;
    Metadata.ChildrenAreUnevaluated = true;
  }

  if (const auto *LabelStatement =
          llvm::dyn_cast<clang::LabelStmt>(&Statement)) {
    const std::string Name = LabelStatement->getName();
    Metadata.Name = Name;
    Label += " ";
    Label += Name;
  } else if (const auto *Goto = llvm::dyn_cast<clang::GotoStmt>(&Statement)) {
    Metadata.TargetLabel = Goto->getLabel()->getNameAsString();
    Label += " ";
    Label += *Metadata.TargetLabel;
  } else if (const auto *Address =
                 llvm::dyn_cast<clang::AddrLabelExpr>(&Statement)) {
    Metadata.TargetLabel = Address->getLabel()->getNameAsString();
    Label += " ";
    Label += *Metadata.TargetLabel;
  }

  if (const auto *Catch = llvm::dyn_cast<clang::CXXCatchStmt>(&Statement)) {
    Metadata.IsCatchAll = Catch->getExceptionDecl() == nullptr;
    Metadata.ExceptionType =
        Metadata.IsCatchAll ? "..." : Catch->getCaughtType().getAsString();
  }

  if (const auto *Case = llvm::dyn_cast<clang::CaseStmt>(&Statement)) {
    std::string CaseLabel = sourceText(Case->getLHS()->getSourceRange());
    if (Case->getRHS() != nullptr) {
      CaseLabel += " ... ";
      CaseLabel += sourceText(Case->getRHS()->getSourceRange());
    }
    if (!CaseLabel.empty())
      Metadata.CaseLabel = std::move(CaseLabel);
  }

  if (const auto *If = llvm::dyn_cast<clang::IfStmt>(&Statement);
      If != nullptr && If->isConstexpr()) {
    const std::optional<const clang::Stmt *> Selected =
        If->getNondiscardedCase(Context);
    if (!Selected)
      Metadata.ConstexprSelection = ast::ConstexprBranchSelection::Dependent;
    else if (*Selected == If->getThen())
      Metadata.ConstexprSelection = ast::ConstexprBranchSelection::Then;
    else
      Metadata.ConstexprSelection = ast::ConstexprBranchSelection::Else;
  }

  if (const auto *Choose = llvm::dyn_cast<clang::ChooseExpr>(&Statement)) {
    if (Choose->isConditionDependent())
      Metadata.ConstexprSelection = ast::ConstexprBranchSelection::Dependent;
    else if (Choose->isConditionTrue())
      Metadata.ConstexprSelection = ast::ConstexprBranchSelection::Then;
    else
      Metadata.ConstexprSelection = ast::ConstexprBranchSelection::Else;
  }

  if (const auto *Assembly = llvm::dyn_cast<clang::GCCAsmStmt>(&Statement)) {
    Metadata.IsAsmGoto = Assembly->isAsmGoto();
    for (unsigned Index = 0; Index < Assembly->getNumLabels(); ++Index)
      Metadata.TargetLabels.push_back(Assembly->getLabelName(Index).str());
  }

  if (const auto *RangeFor =
          llvm::dyn_cast<clang::CXXForRangeStmt>(&Statement)) {
    ast::RangeForSemantics Semantics;
    Semantics.InitializationMayThrow =
        containsPotentiallyThrowingOperation(RangeFor->getBeginStmt(),
                                             Context) ||
        containsPotentiallyThrowingOperation(RangeFor->getEndStmt(), Context);
    Semantics.ConditionMayThrow =
        containsPotentiallyThrowingOperation(RangeFor->getCond(), Context);
    Semantics.ElementMayThrow = containsPotentiallyThrowingOperation(
        RangeFor->getLoopVarStmt(), Context);
    Semantics.IncrementMayThrow =
        containsPotentiallyThrowingOperation(RangeFor->getInc(), Context);
    Metadata.RangeFor = Semantics;
  }

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

source::FunctionInfo
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

  return source::FunctionInfo{source::FunctionId(Usr.str().str()),
                              std::move(Name), std::move(QualifiedName),
                              std::move(Signature),
                              sourceRange(Function.getSourceRange())};
}

std::optional<source::SourceRange>
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

  return source::SourceRange{Sources.getFileOffset(Begin),
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
