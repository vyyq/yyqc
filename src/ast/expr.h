#ifndef _EXPR_H_
#define _EXPR_H_

#include "../lexer/lexer.h"
#include "../type/type_base.h"
#include "ast_base.h"
#include "stmt.h"

#include <list>
#include <string>

class PrimaryExpr : public Expr {
public:
  PrimaryExpr(std::shared_ptr<Token> token) : Expr(token) {}
protected:
  virtual void print(std::ostream &os) const override {
    os << "PrimaryExpr: " << *_token;
  }
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
  Identifier(std::shared_ptr<Token> token) : PrimaryExpr(std::move(token)) {}
  Identifier(std::shared_ptr<Token> token, IdentifierNameSpace name_space)
      : PrimaryExpr(std::move(token)), _name_space(name_space) {}
  virtual bool IsLValue() const override { return true; }
protected:
  virtual void print(std::ostream &os) const override {
    os << "Identifier: " << *_token;
  }
private:
  IdentifierNameSpace _name_space = IdentifierNameSpace::UNKNOWN;
};

class Constant : public PrimaryExpr {
public:
  virtual bool IsLValue() const override { return false; }
  Constant(std::shared_ptr<Token> token, std::unique_ptr<Value> &val)
      : PrimaryExpr(token), _value(std::move(val)) {}
  Constant(std::shared_ptr<Token> token) : PrimaryExpr(token) {}
protected:
  virtual void print(std::ostream &os) const override {
    os << "Constant: " << *_token << " || " << *_value;
  }
private:
  std::unique_ptr<Value> _value;
};

class UnaryOperatorExpr : public Expr {
public:
  virtual bool IsLValue() const override { return true; }
  UnaryOperatorExpr(OP op, std::unique_ptr<Expr> &operand,
                    std::shared_ptr<Token> &token)
      : Expr(token), _operator(op), _operand(std::move(operand)) {}
  UnaryOperatorExpr(OP op, std::unique_ptr<Expr> &operand)
      : Expr(nullptr), _operator(op), _operand(std::move(operand)) {}
  void set_operator(OP op) { _operator = op; }
  void set_operand(std::unique_ptr<Expr> &operand) {
    _operand = std::move(operand);
  }
protected:
  virtual void print(std::ostream &os) const override {
    os << "Unary Operator Expression: " << std::endl;
    os << "OP: " << op_to_string.at(_operator)  << std::endl;
    _operand->print(os);
  }
private:
  OP _operator;
  std::unique_ptr<Expr> _operand;
};

class BinaryOperatorExpr : public Expr {
public:
  virtual bool IsLValue() const override { return false; };
  BinaryOperatorExpr(OP op, std::unique_ptr<Expr> &operand1,
                              std::unique_ptr<Expr> &operand2,
                              std::shared_ptr<Token> token = nullptr)
      : Expr(token), _operator(op), _operand1(std::move(operand1)),
        _operand2(std::move(operand2)) {}
  void set_operator(OP op) { _operator = op; }
  void set_operand1(std::unique_ptr<Expr> operand1) {
    _operand1 = std::move(operand1);
  }
  void set_operand2(std::unique_ptr<Expr> operand2) {
    _operand2 = std::move(operand2);
  }
protected:
  virtual void print(std::ostream &os) const override {
    os << "Binary Operator Expression: " << std::endl;
    _operand1->print(os);
    os << std::endl << "OP: " << op_to_string.at(_operator)  << std::endl;
    _operand2->print(os);
  }
private:
  OP _operator;
  std::unique_ptr<Expr> _operand1;
  std::unique_ptr<Expr> _operand2;
};

class TenaryOperatorExpr : public Expr {
public:
  virtual bool IsLValue() const { return false; };
  TenaryOperatorExpr(OP op1, OP op2, std::unique_ptr<Expr> &operand1,
                     std::unique_ptr<Expr> &operand2,
                     std::unique_ptr<Expr> &operand3,
                     std::shared_ptr<Token> token = nullptr)
      : Expr(token), _operator1(op1), _operator2(op2),
        _operand1(std::move(operand1)), _operand2(std::move(operand2)),
        _operand3(std::move(operand3)) {}
  void set_operator1(OP op1) { _operator1 = op1; }
  void set_operator2(OP op2) { _operator2 = op2; }

private:
  OP _operator1;
  OP _operator2;
  std::unique_ptr<Expr> _operand1;
  std::unique_ptr<Expr> _operand2;
  std::unique_ptr<Expr> _operand3;
};

class FunctionCallExpr : public Expr {
public:
  virtual bool IsLValue() const { return false; }
  FunctionCallExpr(std::unique_ptr<Expr> &designator,
                   std::vector<std::unique_ptr<Expr>> &param_list,
                   std::shared_ptr<Token> token = nullptr)
      : Expr(token), _designator(std::move(designator)),
        _parameter_list(std::move(param_list)) {}
  FunctionCallExpr(std::unique_ptr<Expr> &designator,
                   std::shared_ptr<Token> token = nullptr)
      : Expr(token), _designator(std::move(designator)) {}
  void AddParameters(std::vector<std::unique_ptr<Expr>> &src) {
    _parameter_list.insert(_parameter_list.end(),
                           std::make_move_iterator(src.begin()),
                           std::make_move_iterator(src.end()));
  }

private:
  std::unique_ptr<Expr> _designator;
  std::vector<std::unique_ptr<Expr>> _parameter_list;
};

#endif
