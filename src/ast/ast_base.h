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

enum class OP {
  PLUS,
  MINUS,
  MULTIPLY,
  DIVIDE,
  MOD,

  ARROW_REFERENCE,
  POINT_REFERENCE,
  DEREFERENCE,
  POSTFIX_INC,
  POSTFIX_DEC,
  PREFIX_INC,
  PREFIX_DEC,
  SIZEOF,
  GET_ADDRESS,
  POSITIVE,
  NEGATIVE,
  NEGATION,

  RIGHT_SHIFT,
  LEFT_SHIFT,
  LESS,
  GREATER,
  LE,
  GE,
  EQ,
  NE,

  AND,
  XOR,
  OR,

  LOGICAL_AND,
  LOGICAL_OR,

  COND,
  COLON,

  ASSIGN,
  MULTIPLY_ASSIGN,
  DIVIDE_ASSIGN,
  MOD_ASSIGN,
  PLUS_ASSIGN,
  MINUS_ASSIGN,
  LEFT_SHIFT_ASSIGN,
  RIGHT_SHIFT_ASSIGN,
  AND_ASSIGN,
  NOT_ASSIGN,
  OR_ASSIGN,
};

class ASTNode {};

class Stmt : public ASTNode {};

class Expr : public ASTNode {
public:
  void set_token(std::unique_ptr<Token> token) { _token = std::move(token); }
  virtual ~Expr() {}

protected:
  Expr(std::shared_ptr<Token> token) : _token(token) {}
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
