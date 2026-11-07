#include "clang_ast_classifier.hpp"

#include "clang/AST/ASTTypeTraits.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtCXX.h"
#include "llvm/Support/Casting.h"

namespace mycpptools::clang_frontend {

namespace {

class AstNodeClassifier {
public:
  [[nodiscard]] static ast::AstNodeKind
  classify(const clang::Decl &Declaration) {
    if (llvm::isa<clang::FunctionDecl>(Declaration))
      return ast::AstNodeKind::FunctionDeclaration;
    if (llvm::isa<clang::ParmVarDecl>(Declaration))
      return ast::AstNodeKind::ParameterDeclaration;
    if (llvm::isa<clang::VarDecl>(Declaration))
      return ast::AstNodeKind::VariableDeclaration;
    if (llvm::isa<clang::FieldDecl>(Declaration))
      return ast::AstNodeKind::FieldDeclaration;
    if (llvm::isa<clang::RecordDecl>(Declaration))
      return ast::AstNodeKind::RecordDeclaration;
    return ast::AstNodeKind::OtherDeclaration;
  }

  [[nodiscard]] static ast::AstNodeKind classify(const clang::Stmt &Statement) {
    if (llvm::isa<clang::CompoundStmt>(Statement))
      return ast::AstNodeKind::CompoundStatement;
    if (llvm::isa<clang::DeclStmt>(Statement))
      return ast::AstNodeKind::DeclarationStatement;
    if (const auto *If = llvm::dyn_cast<clang::IfStmt>(&Statement))
      return If->isConstexpr() ? ast::AstNodeKind::ConstexprIfStatement
                               : ast::AstNodeKind::IfStatement;
    if (llvm::isa<clang::SwitchStmt>(Statement))
      return ast::AstNodeKind::SwitchStatement;
    if (llvm::isa<clang::CaseStmt>(Statement))
      return ast::AstNodeKind::CaseStatement;
    if (llvm::isa<clang::DefaultStmt>(Statement))
      return ast::AstNodeKind::DefaultStatement;
    if (llvm::isa<clang::ForStmt>(Statement))
      return ast::AstNodeKind::ForStatement;
    if (llvm::isa<clang::CXXForRangeStmt>(Statement))
      return ast::AstNodeKind::RangeForStatement;
    if (llvm::isa<clang::WhileStmt>(Statement))
      return ast::AstNodeKind::WhileStatement;
    if (llvm::isa<clang::DoStmt>(Statement))
      return ast::AstNodeKind::DoStatement;
    if (llvm::isa<clang::ReturnStmt>(Statement))
      return ast::AstNodeKind::ReturnStatement;
    if (llvm::isa<clang::BreakStmt>(Statement))
      return ast::AstNodeKind::BreakStatement;
    if (llvm::isa<clang::ContinueStmt>(Statement))
      return ast::AstNodeKind::ContinueStatement;
    if (llvm::isa<clang::GotoStmt>(Statement))
      return ast::AstNodeKind::GotoStatement;
    if (llvm::isa<clang::IndirectGotoStmt>(Statement))
      return ast::AstNodeKind::IndirectGotoStatement;
    if (llvm::isa<clang::LabelStmt>(Statement))
      return ast::AstNodeKind::LabelStatement;
    if (llvm::isa<clang::CXXTryStmt>(Statement))
      return ast::AstNodeKind::TryStatement;
    if (llvm::isa<clang::CXXCatchStmt>(Statement))
      return ast::AstNodeKind::CatchStatement;
    if (llvm::isa<clang::AsmStmt>(Statement))
      return ast::AstNodeKind::AsmStatement;
    if (llvm::isa<clang::CoroutineBodyStmt>(Statement))
      return ast::AstNodeKind::CoroutineBodyStatement;
    if (llvm::isa<clang::CoreturnStmt>(Statement))
      return ast::AstNodeKind::CoReturnStatement;
    if (llvm::isa<clang::LambdaExpr>(Statement))
      return ast::AstNodeKind::LambdaExpression;
    if (llvm::isa<clang::CXXThrowExpr>(Statement))
      return ast::AstNodeKind::ThrowExpression;
    if (llvm::isa<clang::CoawaitExpr>(Statement))
      return ast::AstNodeKind::CoAwaitExpression;
    if (llvm::isa<clang::CoyieldExpr>(Statement))
      return ast::AstNodeKind::CoYieldExpression;
    if (llvm::isa<clang::CallExpr>(Statement))
      return ast::AstNodeKind::CallExpression;
    if (llvm::isa<clang::BinaryOperator>(Statement))
      return ast::AstNodeKind::BinaryExpression;
    if (llvm::isa<clang::UnaryOperator>(Statement))
      return ast::AstNodeKind::UnaryExpression;
    if (llvm::isa<clang::ConditionalOperator>(Statement))
      return ast::AstNodeKind::ConditionalExpression;
    if (llvm::isa<clang::DeclRefExpr>(Statement))
      return ast::AstNodeKind::DeclarationReferenceExpression;
    if (llvm::isa<clang::MemberExpr>(Statement))
      return ast::AstNodeKind::MemberExpression;
    if (llvm::isa<clang::CastExpr>(Statement))
      return ast::AstNodeKind::CastExpression;
    if (llvm::isa<clang::IntegerLiteral, clang::FloatingLiteral,
                  clang::CharacterLiteral, clang::StringLiteral,
                  clang::CXXBoolLiteralExpr, clang::CXXNullPtrLiteralExpr>(
            Statement))
      return ast::AstNodeKind::LiteralExpression;
    if (llvm::isa<clang::Expr>(Statement))
      return ast::AstNodeKind::OtherExpression;
    return ast::AstNodeKind::OtherStatement;
  }
};

class AstEdgeClassifier {
public:
  [[nodiscard]] static ast::AstEdgeRole
  classify(const clang::DynTypedNode &Parent, const clang::Decl &Child) {
    if (const auto *Declaration = Parent.get<clang::Decl>())
      return classify(*Declaration, Child);
    if (const auto *Statement = Parent.get<clang::Stmt>())
      return classify(*Statement, Child);
    return ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole
  classify(const clang::DynTypedNode &Parent, const clang::Stmt &Child) {
    if (const auto *Declaration = Parent.get<clang::Decl>())
      return classify(*Declaration, Child);
    if (const auto *Statement = Parent.get<clang::Stmt>())
      return classify(*Statement, Child);
    return ast::AstEdgeRole::Child;
  }

private:
  [[nodiscard]] static ast::AstEdgeRole classify(const clang::Decl &Parent,
                                                 const clang::Decl &Child) {
    const auto *Function = llvm::dyn_cast<clang::FunctionDecl>(&Parent);
    if (Function != nullptr && llvm::isa<clang::ParmVarDecl>(Child) &&
        Child.getDeclContext() == Function)
      return ast::AstEdgeRole::Parameter;
    return ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole classify(const clang::Stmt &Parent,
                                                 const clang::Decl &) {
    return llvm::isa<clang::DeclStmt>(Parent) ? ast::AstEdgeRole::Declaration
                                              : ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole classify(const clang::Decl &Parent,
                                                 const clang::Stmt &Child) {
    if (const auto *Function = llvm::dyn_cast<clang::FunctionDecl>(&Parent)) {
      if (Function->getBody() == &Child)
        return ast::AstEdgeRole::Body;
    }

    if (const auto *Variable = llvm::dyn_cast<clang::VarDecl>(&Parent)) {
      if (Variable->getInit() == &Child)
        return ast::AstEdgeRole::Initializer;
    }

    return ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole classify(const clang::Stmt &Parent,
                                                 const clang::Stmt &Child) {
    if (const auto *Statement = llvm::dyn_cast<clang::IfStmt>(&Parent))
      return classify(*Statement, Child);
    if (const auto *Statement = llvm::dyn_cast<clang::ForStmt>(&Parent))
      return classify(*Statement, Child);
    if (const auto *Statement = llvm::dyn_cast<clang::CXXForRangeStmt>(&Parent))
      return classify(*Statement, Child);
    if (const auto *Statement = llvm::dyn_cast<clang::WhileStmt>(&Parent))
      return classify(*Statement, Child);
    if (const auto *Statement = llvm::dyn_cast<clang::DoStmt>(&Parent))
      return classify(*Statement, Child);
    if (const auto *Statement = llvm::dyn_cast<clang::SwitchStmt>(&Parent))
      return classify(*Statement, Child);
    if (const auto *Statement = llvm::dyn_cast<clang::CaseStmt>(&Parent))
      return classify(*Statement, Child);
    if (const auto *Statement = llvm::dyn_cast<clang::DefaultStmt>(&Parent))
      return classify(*Statement, Child);
    if (const auto *Statement = llvm::dyn_cast<clang::LabelStmt>(&Parent))
      return classify(*Statement, Child);
    if (const auto *Expression = llvm::dyn_cast<clang::LambdaExpr>(&Parent))
      return classify(*Expression, Child);
    if (const auto *Statement = llvm::dyn_cast<clang::CXXTryStmt>(&Parent))
      return classify(*Statement, Child);
    if (const auto *Statement = llvm::dyn_cast<clang::CXXCatchStmt>(&Parent))
      return classify(*Statement, Child);
    if (const auto *Expression = llvm::dyn_cast<clang::CXXThrowExpr>(&Parent))
      return classify(*Expression, Child);
    if (const auto *Statement = llvm::dyn_cast<clang::CoreturnStmt>(&Parent))
      return classify(*Statement, Child);
    if (const auto *Statement =
            llvm::dyn_cast<clang::IndirectGotoStmt>(&Parent))
      return classify(*Statement, Child);
    if (const auto *Expression = llvm::dyn_cast<clang::CoawaitExpr>(&Parent))
      return classify(*Expression, Child);
    if (const auto *Expression = llvm::dyn_cast<clang::CoyieldExpr>(&Parent))
      return classify(*Expression, Child);
    if (const auto *Statement =
            llvm::dyn_cast<clang::CoroutineBodyStmt>(&Parent))
      return classify(*Statement, Child);
    if (const auto *Expression = llvm::dyn_cast<clang::BinaryOperator>(&Parent))
      return classify(*Expression, Child);
    if (const auto *Expression = llvm::dyn_cast<clang::UnaryOperator>(&Parent))
      return classify(*Expression, Child);
    if (const auto *Expression =
            llvm::dyn_cast<clang::ConditionalOperator>(&Parent))
      return classify(*Expression, Child);
    if (const auto *Expression = llvm::dyn_cast<clang::CallExpr>(&Parent))
      return classify(*Expression, Child);
    if (const auto *Statement = llvm::dyn_cast<clang::ReturnStmt>(&Parent))
      return classify(*Statement, Child);
    return ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole classify(const clang::IfStmt &Parent,
                                                 const clang::Stmt &Child) {
    if (Parent.getInit() == &Child)
      return ast::AstEdgeRole::Initializer;
    if (Parent.getConditionVariableDeclStmt() == &Child ||
        Parent.getCond() == &Child)
      return ast::AstEdgeRole::Condition;
    if (Parent.getThen() == &Child)
      return ast::AstEdgeRole::ThenBranch;
    if (Parent.getElse() == &Child)
      return ast::AstEdgeRole::ElseBranch;
    return ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole classify(const clang::ForStmt &Parent,
                                                 const clang::Stmt &Child) {
    if (Parent.getInit() == &Child)
      return ast::AstEdgeRole::Initializer;
    if (Parent.getConditionVariableDeclStmt() == &Child ||
        Parent.getCond() == &Child)
      return ast::AstEdgeRole::Condition;
    if (Parent.getInc() == &Child)
      return ast::AstEdgeRole::Increment;
    if (Parent.getBody() == &Child)
      return ast::AstEdgeRole::Body;
    return ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole
  classify(const clang::CXXForRangeStmt &Parent, const clang::Stmt &Child) {
    if (Parent.getInit() == &Child)
      return ast::AstEdgeRole::Initializer;
    if (Parent.getRangeStmt() == &Child)
      return ast::AstEdgeRole::Range;
    if (Parent.getLoopVarStmt() == &Child)
      return ast::AstEdgeRole::LoopVariable;
    if (Parent.getCond() == &Child)
      return ast::AstEdgeRole::Condition;
    if (Parent.getInc() == &Child)
      return ast::AstEdgeRole::Increment;
    if (Parent.getBody() == &Child)
      return ast::AstEdgeRole::Body;
    return ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole classify(const clang::WhileStmt &Parent,
                                                 const clang::Stmt &Child) {
    if (Parent.getConditionVariableDeclStmt() == &Child ||
        Parent.getCond() == &Child)
      return ast::AstEdgeRole::Condition;
    if (Parent.getBody() == &Child)
      return ast::AstEdgeRole::Body;
    return ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole classify(const clang::DoStmt &Parent,
                                                 const clang::Stmt &Child) {
    if (Parent.getCond() == &Child)
      return ast::AstEdgeRole::Condition;
    if (Parent.getBody() == &Child)
      return ast::AstEdgeRole::Body;
    return ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole
  classify(const clang::SwitchStmt &Parent, const clang::Stmt &Child) {
    if (Parent.getInit() == &Child)
      return ast::AstEdgeRole::Initializer;
    if (Parent.getConditionVariableDeclStmt() == &Child ||
        Parent.getCond() == &Child)
      return ast::AstEdgeRole::Condition;
    if (Parent.getBody() == &Child)
      return ast::AstEdgeRole::Body;
    return ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole classify(const clang::CaseStmt &Parent,
                                                 const clang::Stmt &Child) {
    if (Parent.getLHS() == &Child)
      return ast::AstEdgeRole::CaseValue;
    if (Parent.getRHS() == &Child)
      return ast::AstEdgeRole::CaseRangeEnd;
    if (Parent.getSubStmt() == &Child)
      return ast::AstEdgeRole::CaseBody;
    return ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole
  classify(const clang::DefaultStmt &Parent, const clang::Stmt &Child) {
    return Parent.getSubStmt() == &Child ? ast::AstEdgeRole::DefaultBody
                                         : ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole classify(const clang::LabelStmt &Parent,
                                                 const clang::Stmt &Child) {
    return Parent.getSubStmt() == &Child ? ast::AstEdgeRole::LabelBody
                                         : ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole
  classify(const clang::LambdaExpr &Parent, const clang::Stmt &Child) {
    return Parent.getBody() == &Child ? ast::AstEdgeRole::LambdaBody
                                      : ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole
  classify(const clang::CXXTryStmt &Parent, const clang::Stmt &Child) {
    if (Parent.getTryBlock() == &Child)
      return ast::AstEdgeRole::TryBody;
    for (unsigned Index = 0; Index < Parent.getNumHandlers(); ++Index) {
      if (Parent.getHandler(Index) == &Child)
        return ast::AstEdgeRole::Handler;
    }
    return ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole
  classify(const clang::CXXCatchStmt &Parent, const clang::Stmt &Child) {
    return Parent.getHandlerBlock() == &Child ? ast::AstEdgeRole::CatchBody
                                              : ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole
  classify(const clang::CXXThrowExpr &Parent, const clang::Stmt &Child) {
    return Parent.getSubExpr() == &Child ? ast::AstEdgeRole::ThrownValue
                                         : ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole
  classify(const clang::CoreturnStmt &Parent, const clang::Stmt &Child) {
    return Parent.getOperand() == &Child ? ast::AstEdgeRole::ReturnValue
                                         : ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole
  classify(const clang::IndirectGotoStmt &Parent, const clang::Stmt &Child) {
    return Parent.getTarget() == &Child ? ast::AstEdgeRole::Operand
                                        : ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole
  classify(const clang::CoawaitExpr &Parent, const clang::Stmt &Child) {
    return Parent.getOperand() == &Child ? ast::AstEdgeRole::Operand
                                         : ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole
  classify(const clang::CoyieldExpr &Parent, const clang::Stmt &Child) {
    return Parent.getOperand() == &Child ? ast::AstEdgeRole::Operand
                                         : ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole
  classify(const clang::CoroutineBodyStmt &Parent, const clang::Stmt &Child) {
    return Parent.getBody() == &Child ? ast::AstEdgeRole::Body
                                      : ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole
  classify(const clang::BinaryOperator &Parent, const clang::Stmt &Child) {
    if (Parent.getLHS() == &Child)
      return ast::AstEdgeRole::LeftOperand;
    if (Parent.getRHS() == &Child)
      return ast::AstEdgeRole::RightOperand;
    return ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole
  classify(const clang::UnaryOperator &Parent, const clang::Stmt &Child) {
    return Parent.getSubExpr() == &Child ? ast::AstEdgeRole::Operand
                                         : ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole
  classify(const clang::ConditionalOperator &Parent, const clang::Stmt &Child) {
    if (Parent.getCond() == &Child)
      return ast::AstEdgeRole::Condition;
    if (Parent.getTrueExpr() == &Child)
      return ast::AstEdgeRole::ThenBranch;
    if (Parent.getFalseExpr() == &Child)
      return ast::AstEdgeRole::ElseBranch;
    return ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole classify(const clang::CallExpr &Parent,
                                                 const clang::Stmt &Child) {
    if (Parent.getCallee() == &Child)
      return ast::AstEdgeRole::Callee;
    for (const clang::Expr *Argument : Parent.arguments()) {
      if (Argument == &Child)
        return ast::AstEdgeRole::Argument;
    }
    return ast::AstEdgeRole::Child;
  }

  [[nodiscard]] static ast::AstEdgeRole
  classify(const clang::ReturnStmt &Parent, const clang::Stmt &Child) {
    return Parent.getRetValue() == &Child ? ast::AstEdgeRole::ReturnValue
                                          : ast::AstEdgeRole::Child;
  }
};

} // namespace

ast::AstNodeKind classifyAstNode(const clang::Decl &Declaration) {
  return AstNodeClassifier::classify(Declaration);
}

ast::AstNodeKind classifyAstNode(const clang::Stmt &Statement) {
  return AstNodeClassifier::classify(Statement);
}

ast::AstEdgeRole classifyAstEdge(const clang::DynTypedNode &Parent,
                                 const clang::Decl &Child) {
  return AstEdgeClassifier::classify(Parent, Child);
}

ast::AstEdgeRole classifyAstEdge(const clang::DynTypedNode &Parent,
                                 const clang::Stmt &Child) {
  return AstEdgeClassifier::classify(Parent, Child);
}

} // namespace mycpptools::clang_frontend
