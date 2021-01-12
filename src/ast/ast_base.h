#ifndef _AST_BASE_H
#define _AST_BASE_H
#include "../lexer/token.h"
#include "../lexer/value.h"
#include <unordered_map>

enum class IdentifierNameSpace;

class ASTNode;
class Stmt;
class Expr;
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

class ASTNode {
  friend std::ostream &operator<<(std::ostream &os, const ASTNode &node) {
    node.print(os);
    return os;
  }

public:
  virtual ~ASTNode() {}
  virtual void print(std::ostream &os) const { os << "ASTNode"; }
};

class Stmt : public ASTNode {
  friend std::ostream &operator<<(std::ostream &os, const Stmt &stmt) {
    stmt.print(os);
    return os;
  }

public:
  virtual ~Stmt() {}
  virtual void print(std::ostream &os) const { os << "Stmt"; }
};

class Expr : public ASTNode {
  friend std::ostream &operator<<(std::ostream &os, const Expr &expr) {
    expr.print(os);
    return os;
  }
public:
  void set_token(std::unique_ptr<Token> token) { _token = std::move(token); }
  virtual ~Expr() {}
  virtual void print(std::ostream &os) const { os << "Expr: " << *_token; }
  static inline std::unordered_map<OP, std::string> op_to_string{
      {OP::AND, "&"},
      {OP::AND_ASSIGN, "&="},
      {OP::ARROW_REFERENCE, "->"},
      {OP::ASSIGN, "="},
      {OP::COLON, ":"},
      {OP::COND, "?"},
      {OP::DEREFERENCE, "* (derefrence)"},
      {OP::DIVIDE, "/ (divide)"},
      {OP::DIVIDE_ASSIGN, "/="},
      {OP::EQ, "=="},
      {OP::GE, ">="},
      {OP::GET_ADDRESS, "& (address)"},
      {OP::GREATER, ">"},
      {OP::LE, "<="},
      {OP::LEFT_SHIFT, "<<"},
      {OP::LEFT_SHIFT_ASSIGN, "<<="},
      {OP::LESS, "<"},
      {OP::LOGICAL_AND, "&&"},
      {OP::LOGICAL_OR, "||"},
      {OP::MINUS, "-"},
      {OP::MINUS_ASSIGN, "-="},
      {OP::MOD, "% (mod)"},
      {OP::MOD_ASSIGN, "%="},
      {OP::MULTIPLY, "* (multiply)"},
      {OP::MULTIPLY_ASSIGN, "*="},
      {OP::NE, "!="},
      {OP::NEGATION, "!"},
      {OP::NEGATIVE, "- (negative)"},
      {OP::NOT_ASSIGN, "^="},
      {OP::OR, "|"},
      {OP::OR_ASSIGN, "|="},
      {OP::PLUS, "+"},
      {OP::PLUS_ASSIGN, "+="},
      {OP::POINT_REFERENCE, ". (reference)"},
      {OP::POSITIVE, "+ (positive)"},
      {OP::POSTFIX_DEC, "-- (postfix)"},
      {OP::POSTFIX_INC, "++ (postfix)"},
      {OP::PREFIX_DEC, "-- (prefix)"},
      {OP::PREFIX_INC, "++ (prefix)"},
      {OP::RIGHT_SHIFT, ">>"},
      {OP::RIGHT_SHIFT_ASSIGN, ">>="},
      {OP::SIZEOF, "sizeof"},
      {OP::XOR, "xor"},
  };

protected:
  Expr(std::shared_ptr<Token> token) : _token(token) {}
  virtual bool IsLValue() const { return false; }
  std::shared_ptr<Token> _token;
};

#endif

