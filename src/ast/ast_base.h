#ifndef _AST_BASE_H
#define _AST_BASE_H
#include "../lexer/token.h"
#include "../lexer/value.h"

enum class IdentifierNameSpace;

class ASTNode;
class Stmt;
class Expr;
class Decl;
class LabeledStmt;
class CompoundStmt;
class SelectionStmt;
class IfStmt;
class SwitchStmt;
class ExpressionStmt;
class WhileStmt;
class DoWhileStmt;
class ForStmt;
class JumpStmt;
class GotoStmt;
class ContinueStmt;
class BreakStmt;
class ReturnStmt;
class PrimaryExpr;
class Identifier;
class LabelName;
class StructUnionEnumTag;
class StructUnionMember;
class OrdinaryIdentifier;
class Constant;
class StringLiteral;
class UnaryOperatorExpr;
class BinaryOperatorExpr;
class TenaryOperatorExpr;
class FunctionCallExpr;

class Constant;
class Identifier;

enum class EXPR {
  PRIMARY_EXPR = 1,
  POSTFIX_EXPR,
  UNARY_EXPR,
  CAST_EXPR,
  MULTIPLICATIVE_EXPR,
  ADDITIVE_EXPR,
  SHIFT_EXPR,
  RELATIONAL_EXPR,
  EQUALITY_EXPR,
  AND_EXPR,
  EXCLUSIVE_OR_EXPR,
  INCLUSIVE_OR_EXPR,
  LOGICAL_AND_EXPR,
  LOGICAL_OR_EXPR,
  CONDITIONAL_EXPR,
  ASSIGNMENT_EXPR
};

class ASTNode {};

class Stmt : public ASTNode {};

class Expr : public ASTNode {
public:
  void set_token(std::unique_ptr<Token> token) { _token = std::move(token); }
    virtual ~Expr() {}
protected:
  Expr(std::unique_ptr<Token> token) : _token(std::move(token)) {}

  virtual bool IsLValue() const = 0;

private:
  std::shared_ptr<Token> _token;
};

class Decl : public ASTNode {};

#endif

// int _precedence;
// Expr(Token *token, int precedence, Type *type)
//     : _token(token), _precedence(precedence), _type(type) {}
// int precedence() const { return _precedence; }
// Type *type() const { return _type; }
