#ifndef _EXPR_H_
#define _EXPR_H_

#include "../lexer/lexer.h"
#include "ast_base.h"
#include "stmt.h"
#include "../type/type_base.h"

#include <list>
#include <string>

class PrimaryExpr : public Expr {
public:
protected:
  PrimaryExpr(std::unique_ptr<Token> token) : Expr(std::move(token)) {}
};

enum class IdentifierNameSpace {
  UNKNOWN,
  LABEL_NAME,
  STRUCT_UNION_ENUM_TAG,
  STRUCT_UNION_MEM,
  ORDINARY_IDENTIFIER,
};

class Identifier : public PrimaryExpr {
public:
  void set_name_space(IdentifierNameSpace name_space) {
    _name_space = name_space;
  }
  Identifier(std::unique_ptr<Token> token, std::unique_ptr<Type> type = nullptr)
      : PrimaryExpr(std::move(token)), _type(std::move(type)) {}
  Identifier(std::unique_ptr<Token> token, IdentifierNameSpace name_space,
             std::unique_ptr<Type> type = nullptr)
      : PrimaryExpr(std::move(token)), _name_space(name_space),
        _type(std::move(type)) {}
  virtual bool IsLValue() const { return true; }
  Type *type() const { return _type.get(); }

private:
  IdentifierNameSpace _name_space = IdentifierNameSpace::UNKNOWN;
  std::unique_ptr<Type> _type;
};

class Constant : public PrimaryExpr {
public:
  virtual bool IsLValue() const { return false; }
  Constant(std::unique_ptr<Token> token, std::unique_ptr<Value> val)
      : PrimaryExpr(std::move(token)), _value(std::move(val)) {}

private:
  std::unique_ptr<Value> _value;
};

class UnaryOperatorExpr : public Expr {
public:
  virtual bool IsLValue() const;
  UnaryOperatorExpr(TOKEN op, std::unique_ptr<Expr> &operand, std::unique_ptr<Token> &token)
      : Expr(std::move(token)), _operator(op), _operand(std::move(operand)) {}
  UnaryOperatorExpr(TOKEN op, std::unique_ptr<Expr> operand)
      : Expr(nullptr), _operator(op), _operand(std::move(operand)) {}
  void set_operator(TOKEN op) { _operator = op; }
  void set_operand(std::unique_ptr<Expr> operand) { _operand = std::move(operand); }

private:
  TOKEN _operator;
  std::unique_ptr<Expr> _operand;
};

class BinaryOperatorExpr : public Expr {
public:
  virtual bool IsLValue() const { return false; };
  BinaryOperatorExpr(TOKEN op, std::unique_ptr<Expr> operand1, std::unique_ptr<Expr> operand2,
                     std::unique_ptr<Token> token = nullptr)
      : Expr(std::move(token)), _operator(op), _operand1(std::move(operand1)), _operand2(std::move(operand2)) {}
  void set_operator(TOKEN op) { _operator = op; }
  void set_operand1(std::unique_ptr<Expr> operand1) { _operand1 = std::move(operand1); }
  void set_operand2(std::unique_ptr<Expr> operand2) { _operand2 = std::move(operand2); }

private:
  TOKEN _operator;
  std::unique_ptr<Expr> _operand1;
  std::unique_ptr<Expr> _operand2;
};

class TenaryOperatorExpr : public Expr {
public:
  virtual bool IsLValue() const { return false; };
  TenaryOperatorExpr(TOKEN op1, TOKEN op2, Expr *operand1, Expr *operand2,
                     Expr *operand3, std::unique_ptr<Token> token = nullptr)
      : Expr(std::move(token)), _operator1(op1), _operator2(op2), _operand1(operand1),
        _operand2(operand2), _operand3(operand3) {}
  void set_operator1(TOKEN op1) { _operator1 = op1; }
  void set_operator2(TOKEN op2) { _operator2 = op2; }
  void set_operand1(Expr *operand1) { _operand1 = operand1; }
  void set_operand2(Expr *operand2) { _operand2 = operand2; }
  void set_operand3(Expr *operand3) { _operand3 = operand3; }

private:
  TOKEN _operator1;
  TOKEN _operator2;
  Expr *_operand1;
  Expr *_operand2;
  Expr *_operand3;
};

class FunctionCallExpr : public Expr {
public:
  virtual bool IsLValue() const { return false; }
  FunctionCallExpr(Expr *designator, const std::list<Expr *> &param_list,
                   std::unique_ptr<Token> token = nullptr)
      : Expr(std::move(token)), _designator(designator), _parameter_list(param_list) {}

private:
  Expr *_designator;
  std::list<Expr *> _parameter_list;
};

#endif
