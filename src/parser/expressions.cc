#include "parser.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <list>
#include <pthread.h>
#include <set>
#include <sstream>
#include <string>

/**
 *  primary-expression  ->
 *                          identifier
 *                        | constant
 *                        | string-literal
 *                        | ( expression )
 *                        | generic-selection
 */
std::unique_ptr<Expr> Parser::PrimaryExpression() {
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::IDENTIFIER) {
    Match(TOKEN::IDENTIFIER);
    return std::make_unique<Identifier>(token);
  } else if (tag == TOKEN::INTEGER_CONTANT || tag == TOKEN::FLOATING_CONSTANT ||
             tag == TOKEN::ENUMERATION_CONSTANT ||
             tag == TOKEN::CHARACTER_CONSTANT) {
    ConsumeToken();
    return std::make_unique<Constant>(token);
  } else if (tag == TOKEN::STRING_LITERAL) {
    Match(TOKEN::STRING_LITERAL);
    return std::make_unique<Constant>(token);
  } else if (tag == TOKEN::LPAR) {
    Match(TOKEN::LPAR);
    auto expr = Expression();
    Match(TOKEN::RPAR);
    return std::move(expr);
  } else if (tag == TOKEN::GENERIC) {
    Error("Generic-selection is not supported now.");
    return nullptr;
  } else {
    return nullptr;
  }
}

std::unique_ptr<Expr> Parser::Expression() {
  auto expr = AssignmentExpr();
  return std::move(expr);
  // TODO: expression, assignment-expression
}

// const char stoc(const std::string &str) {
//   // TODO: Parse case starting with 'u', 'L', 'U'
//   assert(str.size() <= 2);
//   if (str.size() == 2) {
//     assert(str[0] == '\\');
//   }
//   if (str.size() == 2) {
//     switch (str.at(1)) {
//     case 'n':
//       return '\n';
//     case '"':
//       return '\"';
//     default:
//       return '\\';
//       break;
//     }
//   } else {
//     return str.at(0);
//   }
// }

/**
 * The left recursions are re-written as:
 * postfix-expression
 *   -> ( type-name ) { initialize-list } postfix-expression'
 *    | ( type-name ) { initialize-list , } postfix-expression'
 *    | primary-expression postfix-expression'
 *
 * postfix-expression'
 *   -> [ expression ] postfix-expression'
 *    | ( argument-expression-list_{opt} ) postfix-expression'
 *    | .  identifier postfix-expression'
 *    | -> identifier postfix-expression'
 *    | ++ postfix-expression'
 *    | -- postfix-expression'
 *    | $epsilon$
 */
std::unique_ptr<Expr> Parser::PostfixExpr() {
  auto expr = PrimaryExpression();
  PostfixExprPrime(expr);
  return std::move(expr);
  // TODO: support compound literal feature.
  // Error("Compound literals feature is not supported yet.");
}

void Parser::PostfixExprPrime(std::unique_ptr<Expr> &expr) {
  auto token = PeekToken();
  switch (token->tag()) {
  case TOKEN::LSQUBRKT:
    ArraySubscripting(expr);
    break;
  case TOKEN::LPAR:
    FunctionCall(expr);
    break;
  case TOKEN::DOT:
  case TOKEN::PTR_MEM_REF:
    MemberReference(expr);
    break;
  case TOKEN::INCREMENT:
    PostfixIncrement(expr);
    break;
  case TOKEN::DECREMENT:
    PostfixDecrement(expr);
    break;
  default:
    return;
  }
  PostfixExprPrime(expr);
}

/**
 * ptr [offset]:
 * syntax sugar for equivalent expression:
 * *(ptr + offset)
 */
void Parser::ArraySubscripting(std::unique_ptr<Expr> &expr) {
  auto token = PeekToken();
  Match(TOKEN::LSQUBRKT);
  auto offset = Expression();
  Match(TOKEN::RSQUBRKT);
  // Handle ptr_to = ptr + offset
  std::unique_ptr<Expr> point_to = std::make_unique<BinaryOperatorExpr>(OP::PLUS, expr, offset);
  // return dereference(ptr_to)
  auto dereference =
      std::make_unique<UnaryOperatorExpr>(OP::DEREFERENCE, point_to, token);
  expr = std::move(dereference);
}

void Parser::FunctionCall(std::unique_ptr<Expr> &designator) {
  auto token = PeekToken();
  Match(TOKEN::LPAR);
  auto function_call = std::make_unique<FunctionCallExpr>(designator, token);
  if (PeekToken(TOKEN::RPAR)) {
    Match(TOKEN::RPAR);
    designator = std::move(function_call);
  } else {
    auto argument_expression_list = ArgumentExpressionList();
    function_call->AddParameters(argument_expression_list);
    Match(TOKEN::RPAR);
    designator = std::move(function_call);
  }
}

std::vector<std::unique_ptr<Expr>> Parser::ArgumentExpressionList() {
  std::vector<std::unique_ptr<Expr>> argument_expressions;
  return std::move(argument_expressions);
}

void Parser::MemberReference(std::unique_ptr<Expr> &ptr) {
  OP op;
  if (PeekToken(TOKEN::PTR_MEM_REF)) {
    Match(TOKEN::PTR_MEM_REF);
    op = OP::ARROW_REFERENCE;
  } else if (PeekToken(TOKEN::DOT)) {
    Match(TOKEN::DOT);
    op = OP::POINT_REFERENCE;
  } else {
    Error("MemberReference error.");
  }
  auto token = PeekToken();
  Match(TOKEN::IDENTIFIER);
  std::unique_ptr<Expr> identifier = std::make_unique<Identifier>(
      token, IdentifierNameSpace::STRUCT_UNION_MEM);
  auto expr = std::make_unique<BinaryOperatorExpr>(op, ptr, identifier, token);
  ptr = std::move(expr);
}

void Parser::PostfixIncrement(std::unique_ptr<Expr> &operand) {
  auto token = Match(TOKEN::INCREMENT);
  auto expr =
      std::make_unique<UnaryOperatorExpr>(OP::POSTFIX_INC, operand, token);
  operand = std::move(expr);
}

void Parser::PostfixDecrement(std::unique_ptr<Expr> &operand) {
  auto token = Match(TOKEN::DECREMENT);
  auto expr =
      std::make_unique<UnaryOperatorExpr>(OP::POSTFIX_DEC, operand, token);
  operand = std::move(expr);
}

// Expr *Parser::CompoundLiterals(Expr *ptr) {
//   // TODO: implement compound literals feature.
//   return nullptr;
// }

std::unique_ptr<Expr> Parser::UnaryExpr() {
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::INCREMENT) {
    return std::move(PrefixIncrement());
  } else if (tag == TOKEN::DECREMENT) {
    return std::move(PrefixDecrement());
  } else if (tag == TOKEN::SIZEOF) {
    return std::move(Sizeof());
  } else if (tag == TOKEN::ALIGNOF) {
    return nullptr; // TODO
  } else if (tag == TOKEN::AND || tag == TOKEN::STAR || tag == TOKEN::ADD ||
             tag == TOKEN::SUB || tag == TOKEN::NOT ||
             tag == TOKEN::LOGICAL_NOT) {
    OP op;
    if (tag == TOKEN::AND)
      op = OP::GET_ADDRESS;
    else if (tag == TOKEN::STAR)
      op = OP::DEREFERENCE;
    else if (tag == TOKEN::ADD)
      op = OP::POSITIVE;
    else if (tag == TOKEN::SUB)
      op = OP::NEGATIVE;
    // else if (tag == TOKEN::NOT)
    // op =
    else if (tag == TOKEN::LOGICAL_NOT)
      op = OP::NEGATION;
    auto token = ConsumeToken();
    auto operand = CastExpr();
    return std::make_unique<UnaryOperatorExpr>(op, operand, token);
  } else {
    return std::move(PostfixExpr());
  }
}

std::unique_ptr<Expr> Parser::PrefixIncrement() {
  auto token = PeekToken();
  Match(TOKEN::INCREMENT);
  auto operand = UnaryExpr();
  auto expr =
      std::make_unique<UnaryOperatorExpr>(OP::PREFIX_INC, operand, token);
  return std::move(expr);
}

std::unique_ptr<Expr> Parser::PrefixDecrement() {
  auto token = PeekToken();
  Match(TOKEN::DECREMENT);
  auto operand = UnaryExpr();
  auto expr =
      std::make_unique<UnaryOperatorExpr>(OP::PREFIX_DEC, operand, token);
  return std::move(expr);
}

std::unique_ptr<Expr> Parser::Sizeof() {
  auto token = Match(TOKEN::SIZEOF);
  auto expr = UnaryExpr();
  auto sizeof_expr =
      std::make_unique<UnaryOperatorExpr>(OP::SIZEOF, expr, token);
  // TODO: sizeof (type-name)
  return std::move(sizeof_expr);
}

// Expr *Parser::Alignof() { return nullptr; }

// Token *Parser::UnaryOperator() { return ConsumeToken(); }

std::unique_ptr<Expr> Parser::CastExpr() {
  if (PeekToken(TOKEN::LPAR) && false) {
    // TODO: Handle ( type-name ) cast-expresssion
    return nullptr;
  }
  return UnaryExpr();
}

/**
 * multiplicative-expression  ->
 *                             cast-expression
 *                           | multiplicative-expression * cast-expression
 *                           | multiplicative-expression / cast-expression
 *                           | multiplicative-expression % cast-expression
 *
 * which is a left recursive formula and can be re-written as:
 * multiplicative-expression  ->
 *                             cast-expression multiplicative-expression'
 * multiplicative-expression' ->
 *                             * cast-expression multiplicative-expression'
 *                           | / cast-expression multiplicative-expression'
 *                           | % cast-expression multiplicative-expression'
 *                           | $\epsilon$
 */
std::unique_ptr<Expr> Parser::MultiplicativeExpr() {
  auto expr1 = CastExpr();
  MultiplicativeExprPrime(expr1);
  return std::move(expr1);
}

void Parser::MultiplicativeExprPrime(std::unique_ptr<Expr> &operand1) {
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::STAR) {
    Match(TOKEN::STAR);
    auto operand2 = CastExpr();
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::MULTIPLY, operand1,
                                                    operand2, token);
  } else if (tag == TOKEN::DIV) {
    Match(TOKEN::DIV);
    auto operand2 = CastExpr();
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::DIVIDE, operand1,
                                                    operand2, token);
  } else if (tag == TOKEN::MOD) {
    Match(TOKEN::MOD);
    auto operand2 = CastExpr();
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::MOD, operand1, operand2,
                                                    token);
  } else {
    return;
  }
  return MultiplicativeExprPrime(operand1);
}

/**
 * additive-expression  ->
 *                          multiplicative-expression
 *                          | additive-expression + multiplicative-expression
 *                          | additive-expression - multiplicative-expression
 *
 * which is a left recursive formula and can be re-written as:
 * additive-expression  ->
 *                             multiplicative additive-expression'
 * additive-expression' ->
 *                             + multiplicative-expression
 additive-expression'
 *                           | - multiplicative-expression
 additive-expression'
 *                           | $\epsilon$
 */
std::unique_ptr<Expr> Parser::AdditiveExpr() {
  auto operand1 = MultiplicativeExpr();
  AdditiveExprPrime(operand1);
  return std::move(operand1);
}

void Parser::AdditiveExprPrime(std::unique_ptr<Expr> &operand1) {
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::ADD) {
    Match(TOKEN::ADD);
    auto operand2 = MultiplicativeExpr();
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::PLUS, operand1, operand2, token);
  } else if (tag == TOKEN::SUB) {
    Match(TOKEN::SUB);
    auto operand2 = MultiplicativeExpr();
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::MINUS, operand1, operand2, token);
  } else {
    return;
  }
  AdditiveExprPrime(operand1);
}

/**
 * shift-expression  ->
 *                      additive-expression
 *                    | shift-expression << additive-expression
 *                    | shift-expression >> additive-expression
 * which is a left recursive formula and can be re-written as:
 *
 * shift-expression  ->
 *                      additive-expression shift-expression'
 * shift-expression' ->
 *                      << additive-expression shift-expression'
 *                    | >> additive-expression shift-expression'
 *                    | ${epsilon}
 */
std::unique_ptr<Expr> Parser::ShiftExpr() {
  auto operand1 = AdditiveExpr();
  ShiftExprPrime(operand1);
  return std::move(operand1);
}

void Parser::ShiftExprPrime(std::unique_ptr<Expr> &operand1) {
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::LEFT_SHIFT) {
    Match(TOKEN::LEFT_SHIFT);
    auto operand2 = AdditiveExpr();
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::LEFT_SHIFT, operand1,
                                                    operand2, token);
  } else if (tag == TOKEN::RIGHT_SHIFT) {
    Match(TOKEN::RIGHT_SHIFT);
    auto operand2 = AdditiveExpr();
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::RIGHT_SHIFT, operand1,
                                                    operand2, token);
  } else {
    return;
  }
  ShiftExprPrime(operand1);
}

/**
 * relational-expression  ->
 *                            shift-expression
 *                          | relational-expression < shift-expression
 *                          | relational-expression > shift-expression
 *                          | relational-expression <= shift-expression
 *                          | relational-expression >= shift-expression
 *
 * which is a left recursive formula and can be re-written as:
 *
 * relational-expression  ->
 *                            shift-expression relational-expression'
 * relational-expression' ->  < shift-expression relational-expression'
 *                          | > shift-expression relational-expression'
 *                          | >= shift-expression relational-expression'
 *                          | <= shift-expression relational-expression'
 *                          | ${epsilon}
 */
std::unique_ptr<Expr> Parser::RelationalExpr() {
  auto operand1 = ShiftExpr();
  RelationalExprPrime(operand1);
  return std::move(operand1);
}

void Parser::RelationalExprPrime(std::unique_ptr<Expr> &operand1) {
  auto token = PeekToken();
  auto tag = token->tag();

  if (tag == TOKEN::LESS) {
    Match(TOKEN::LESS);
    auto operand2 = ShiftExpr();
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::LESS, operand1,
                                                    operand2, token);
  } else if (tag == TOKEN::GREATER) {
    Match(TOKEN::GREATER);
    auto operand2 = ShiftExpr();
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::GREATER, operand1,
                                                    operand2, token);
  } else if (tag == TOKEN::LE) {
    Match(TOKEN::LE);
    auto operand2 = ShiftExpr();
    operand1 =
        std::make_unique<BinaryOperatorExpr>(OP::LE, operand1, operand2, token);
  } else if (tag == TOKEN::GE) {
    Match(TOKEN::GE);
    auto operand2 = ShiftExpr();
    operand1 =
        std::make_unique<BinaryOperatorExpr>(OP::GE, operand1, operand2, token);
  } else {
    return;
  }
  RelationalExprPrime(operand1);
}

/**
 * equality-expression  ->
 *                            relational-expression
 *                          | equality-expression == relational-expression
 *                          | equality-expression != relational-expression
 *
 * which is a left recursive formula and can be re-written as:
 *
 * equality-expression  ->
 *                            relational-expression equality-expression'
 * equality-expression' ->
 *                            == relational-expression equality-expression'
 *                          | != relational-expression equality-expression'
 *                          | ${epsilon}
 */
std::unique_ptr<Expr> Parser::EqualityExpr() {
  auto operand1 = RelationalExpr();
  EqualityExprPrime(operand1);
  return std::move(operand1);
}

void Parser::EqualityExprPrime(std::unique_ptr<Expr> &operand1) {
  auto token = PeekToken();
  auto tag = token->tag();

  if (tag == TOKEN::EQ) {
    Match(TOKEN::EQ);
    auto operand2 = RelationalExpr();
    operand1 =
        std::make_unique<BinaryOperatorExpr>(OP::EQ, operand1, operand2, token);
  } else if (tag == TOKEN::NE) {
    Match(TOKEN::NE);
    auto operand2 = RelationalExpr();
    operand1 =
        std::make_unique<BinaryOperatorExpr>(OP::NE, operand1, operand2, token);
  } else {
    return;
  }
  EqualityExprPrime(operand1);
}

/**
 * AND-expression  ->
 *                            equality-expression
 *                          | AND-expression & equality-expression
 *
 * which is a left recursive formula and is needed to be re-written.
 */
std::unique_ptr<Expr> Parser::ANDExpr() {
  auto operand1 = EqualityExpr();
  ANDExprPrime(operand1);
  return std::move(operand1);
}

void Parser::ANDExprPrime(std::unique_ptr<Expr> &operand1) {
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::AND) {
    Match(TOKEN::AND);
    auto operand2 = EqualityExpr();
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::AND, operand1, operand2,
                                                    token);
  } else {
    return;
  }
  ANDExprPrime(operand1);
}

/**
 * XOR-expression   ->
 *                        AND-expression
 *                      | XOR-expression ^ AND-expression
 *
 * which is a left-recursive expression, and needed to be rewritten as:
 *
 * XOR-expression   ->    AND-expression XOR-expression'
 * XOR-expression'  ->    ^ AND-expression XOR-expression'
 *                      | ${epsilon}
 */
std::unique_ptr<Expr> Parser::XORExpr() {
  auto operand1 = ANDExpr();
  XORExprPrime(operand1);
  return std::move(operand1);
}

void Parser::XORExprPrime(std::unique_ptr<Expr> &operand1) {
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::XOR) {
    Match(TOKEN::XOR);
    auto operand2 = ANDExpr();
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::XOR, operand1, operand2,
                                                    token);
  } else {
    return;
  }
  XORExprPrime(operand1);
}

/**
 * OR-expression    ->
 *                      XOR-expression
 *                      OR-expression | XOR-expression
 * which is a left recursive expression, and needed to be rewritten as:
 * OR-expression    ->
 *                      XOR-expression OR-expression'
 * OR-expression'   ->  | XOR-expression OR-expression'
 *                  ->  ${epsilon}
 */
std::unique_ptr<Expr> Parser::ORExpr() {
  auto operand1 = XORExpr();
  ORExprPrime(operand1);
  return std::move(operand1);
}

void Parser::ORExprPrime(std::unique_ptr<Expr> &operand1) {
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::OR) {
    Match(TOKEN::OR);
    auto operand2 = XORExpr();
    operand1 =
        std::make_unique<BinaryOperatorExpr>(OP::OR, operand1, operand2, token);
  } else {
    return;
  }
  ORExprPrime(operand1);
}

std::unique_ptr<Expr> Parser::LogicalANDExpr() {
  auto operand1 = ORExpr();
  LogicalANDExprPrime(operand1);
  return std::move(operand1);
}

void Parser::LogicalANDExprPrime(std::unique_ptr<Expr> &operand1) {
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::LOGICAL_AND) {
    Match(TOKEN::LOGICAL_AND);
    auto operand2 = ORExpr();
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::LOGICAL_AND, operand1,
                                                    operand2, token);
  } else {
    return;
  }
  LogicalANDExprPrime(operand1);
}

std::unique_ptr<Expr> Parser::LogicalORExpr() {
  auto operand1 = LogicalANDExpr();
  LogicalORExprPrime(operand1);
  return std::move(operand1);
}

void Parser::LogicalORExprPrime(std::unique_ptr<Expr> &operand1) {
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::LOGICAL_OR) {
    Match(TOKEN::LOGICAL_OR);
    auto operand2 = LogicalANDExpr();
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::LOGICAL_OR, operand1,
                                                    operand2, token);
  } else {
    return;
  }
  LogicalORExprPrime(operand1);
}

std::unique_ptr<Expr> Parser::ConditionalExpr() {
  auto cond = ConditionalExpr();
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::COND) {
    Match(TOKEN::COND);
    auto true_operand = Expression();
    Match(TOKEN::COLON);
    auto false_operand = ConditionalExpr();
    auto expr = std::make_unique<TenaryOperatorExpr>(
        OP::COND, OP::COLON, cond, true_operand, false_operand, token);
    return std::move(expr);
  }
  return std::move(cond);
}

std::unique_ptr<Expr> Parser::AssignmentExpr() {
  auto lhs = UnaryExpr();
  auto token = PeekToken();
  auto tag = token->tag();
  OP op;
  if (tag == TOKEN::ASSIGN) {
    op = OP::ASSIGN;
  } else if (tag == TOKEN::MUL_ASSIGN) {
    op = OP::MULTIPLY_ASSIGN;
  } else if (tag == TOKEN::DIV_ASSIGN) {
    op = OP::DIVIDE_ASSIGN;
  } else if (tag == TOKEN::MOD_ASSIGN) {
    op = OP::MOD_ASSIGN;
  } else if (tag == TOKEN::ADD_ASSIGN) {
    op = OP::PLUS_ASSIGN;
  } else if (tag == TOKEN::SUB_ASSIGN) {
    op = OP::MINUS_ASSIGN;
  } else if (tag == TOKEN::LEFT_ASSIGN) {
    op = OP::LEFT_SHIFT_ASSIGN;
  } else if (tag == TOKEN::RIGHT_ASSIGN) {
    op = OP::RIGHT_SHIFT_ASSIGN;
  } else if (tag == TOKEN::AND_ASSIGN) {
    op = OP::AND_ASSIGN;
  } else if (tag == TOKEN::NOT_ASSIGN) {
    op = OP::NOT_ASSIGN;
  } else if (tag == TOKEN::OR_ASSIGN) {
    op = OP::OR_ASSIGN;
  }
  ConsumeToken();
  auto rhs = AssignmentExpr();
  auto expr = std::make_unique<BinaryOperatorExpr>(op, lhs, rhs, token);
  return std::move(expr);
}

std::unique_ptr<Expr> Parser::ConstantExpr() {
  return std::move(ConditionalExpr());
}
