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
#ifdef DEBUG
  std::cout << ">>> Primary Expression" << std::endl;
#endif
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
    return expr;
  } else if (tag == TOKEN::GENERIC) {
    Error("Generic-selection is not supported now.");
    return nullptr;
  } else {
    return nullptr;
  }
}

std::unique_ptr<Expr> Parser::Expression() {
#ifdef DEBUG
  std::cout << ">>> Expression" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto expr = AssignmentExpr();
  if (!expr) {
    LexerPutBack(snapshot);
    return nullptr;
  }
  return expr;
  // TODO: expression, assignment-expression
}

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
#ifdef DEBUG
  std::cout << ">>> Postfix Expression" << std::endl;
#endif
  auto expr = PrimaryExpression();
  auto postfix_expr_success = PostfixExprPrime(expr);
  if (postfix_expr_success) {
    return expr;
  } else {
    return nullptr;
  }
  // TODO: support compound literal feature.
  // Error("Compound literals feature is not supported yet.");
}

bool Parser::PostfixExprPrime(std::unique_ptr<Expr> &expr) {
#ifdef DEBUG
  std::cout << ">>> Postfix Expression Prime" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  bool scan_success = true;
  switch (token->tag()) {
  case TOKEN::LSQUBRKT:
    scan_success = ArraySubscripting(expr);
    break;
  case TOKEN::LPAR:
    scan_success = FunctionCall(expr);
    break;
  case TOKEN::DOT:
  case TOKEN::PTR_MEM_REF:
    scan_success = MemberReference(expr);
    break;
  case TOKEN::INCREMENT:
    scan_success = PostfixIncrement(expr);
    break;
  case TOKEN::DECREMENT:
    scan_success = PostfixDecrement(expr);
    break;
  default:
    return scan_success;
  }
  if (scan_success) {
    return PostfixExprPrime(expr);
  } else {
    LexerPutBack(snapshot);
    return false;
  }
}

/**
 * ptr [offset]:
 * syntax sugar for equivalent expression:
 * *(ptr + offset)
 */
bool Parser::ArraySubscripting(std::unique_ptr<Expr> &expr) {
#ifdef DEBUG
  std::cout << "Array Subscripting" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  Match(TOKEN::LSQUBRKT);
  auto offset = Expression();
  if (!offset) {
    LexerPutBack(snapshot);
    return false;
  }
  Match(TOKEN::RSQUBRKT);
  // Handle ptr_to = ptr + offset
  std::unique_ptr<Expr> point_to =
      std::make_unique<BinaryOperatorExpr>(OP::PLUS, expr, offset);
  // return dereference(ptr_to)
  auto dereference =
      std::make_unique<UnaryOperatorExpr>(OP::DEREFERENCE, point_to, token);
  expr = std::move(dereference);
  return true;
}

bool Parser::FunctionCall(std::unique_ptr<Expr> &designator) {
#ifdef DEBUG
  std::cout << ">>> Function Call" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  Match(TOKEN::LPAR);
  auto function_call = std::make_unique<FunctionCallExpr>(designator, token);
  if (PeekToken(TOKEN::RPAR)) {
    Match(TOKEN::RPAR);
    designator = std::move(function_call);
  } else {
    auto argument_expression_list = ArgumentExpressionList();
    if (argument_expression_list.size() == 0) {
      LexerPutBack(snapshot);
      return false;
    }
    function_call->AddParameters(argument_expression_list);
    Match(TOKEN::RPAR);
    designator = std::move(function_call);
  }
  return true;
}

std::vector<std::unique_ptr<Expr>> Parser::ArgumentExpressionList() {
  std::vector<std::unique_ptr<Expr>> argument_expressions;
  return argument_expressions;
}

bool Parser::MemberReference(std::unique_ptr<Expr> &ptr) {
#ifdef DEBUG
  std::cout << ">>> Member Reference" << std::endl;
#endif

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
  return true;
}

bool Parser::PostfixIncrement(std::unique_ptr<Expr> &operand) {
#ifdef DEBUG
  std::cout << ">>> Postfix Increment" << std::endl;
#endif

  auto token = Match(TOKEN::INCREMENT);
  auto expr =
      std::make_unique<UnaryOperatorExpr>(OP::POSTFIX_INC, operand, token);
  operand = std::move(expr);
  return true;
}

bool Parser::PostfixDecrement(std::unique_ptr<Expr> &operand) {
  auto token = Match(TOKEN::DECREMENT);
  auto expr =
      std::make_unique<UnaryOperatorExpr>(OP::POSTFIX_DEC, operand, token);
  operand = std::move(expr);
  return true;
}

// Expr *Parser::CompoundLiterals(Expr *ptr) {
//   // TODO: implement compound literals feature.
//   return nullptr;
// }

std::unique_ptr<Expr> Parser::UnaryExpr() {
#ifdef DEBUG
  std::cout << ">>> Unary Expression" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::INCREMENT) {
    return PrefixIncrement();
  } else if (tag == TOKEN::DECREMENT) {
    return PrefixDecrement();
  } else if (tag == TOKEN::SIZEOF) {
    return Sizeof();
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
    if (!operand) {
      LexerPutBack(snapshot);
      return nullptr;
    }
    return std::make_unique<UnaryOperatorExpr>(op, operand, token);
  } else {
    return PostfixExpr();
  }
}

std::unique_ptr<Expr> Parser::PrefixIncrement() {
#ifdef DEBUG
  std::cout << ">>> Prefix Increment" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  Match(TOKEN::INCREMENT);
  auto operand = UnaryExpr();
  if (!operand) {
    LexerPutBack(snapshot);
    return nullptr;
  }
  auto expr =
      std::make_unique<UnaryOperatorExpr>(OP::PREFIX_INC, operand, token);
  return std::move(expr);
}

std::unique_ptr<Expr> Parser::PrefixDecrement() {
#ifdef DEBUG
  std::cout << ">>> Prefix Decrement" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  Match(TOKEN::DECREMENT);
  auto operand = UnaryExpr();
  if (!operand) {
    LexerPutBack(snapshot);
    return nullptr;
  }
  auto expr =
      std::make_unique<UnaryOperatorExpr>(OP::PREFIX_DEC, operand, token);
  return std::move(expr);
}

std::unique_ptr<Expr> Parser::Sizeof() {
#ifdef DEBUG
  std::cout << ">>> sizeof" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto token = Match(TOKEN::SIZEOF);
  auto expr = UnaryExpr();
  if (!expr) {
    LexerPutBack(snapshot);
    return nullptr;
  }
  auto sizeof_expr =
      std::make_unique<UnaryOperatorExpr>(OP::SIZEOF, expr, token);
  // TODO: sizeof (type-name)
  return std::move(sizeof_expr);
}

// Expr *Parser::Alignof() { return nullptr; }

// Token *Parser::UnaryOperator() { return ConsumeToken(); }

std::unique_ptr<Expr> Parser::CastExpr() {
#ifdef DEBUG
  std::cout << ">>> Cast Expression" << std::endl;
#endif
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
#ifdef DEBUG
  std::cout << ">>> Multiplicative Expression" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto expr1 = CastExpr();
  if (MultiplicativeExprPrime(expr1)) {
    return expr1;
  } else {
    LexerPutBack(snapshot);
    return nullptr;
  }
}

bool Parser::MultiplicativeExprPrime(std::unique_ptr<Expr> &operand1) {
#ifdef DEBUG
  std::cout << ">>> Multiplicative Expression Prime" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::STAR) {
    Match(TOKEN::STAR);
    auto operand2 = CastExpr();
    if (!operand2) {
      LexerPutBack(snapshot);
      return false;
    }
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::MULTIPLY, operand1,
                                                    operand2, token);
  } else if (tag == TOKEN::DIV) {
    Match(TOKEN::DIV);
    auto operand2 = CastExpr();
    if (!operand2) {
      LexerPutBack(snapshot);
      return false;
    }
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::DIVIDE, operand1,
                                                    operand2, token);
  } else if (tag == TOKEN::MOD) {
    Match(TOKEN::MOD);
    auto operand2 = CastExpr();
    if (!operand2) {
      LexerPutBack(snapshot);
      return false;
    }
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::MOD, operand1, operand2,
                                                    token);
  } else {
    return true;
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
#ifdef DEBUG
  std::cout << ">>> Additive Expression" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto operand1 = MultiplicativeExpr();
  if (!operand1) {
    LexerPutBack(snapshot);
    return nullptr;
  }
  if (AdditiveExprPrime(operand1)) {
    return operand1;
  } else {
    LexerPutBack(snapshot);
    return nullptr;
  }
}

bool Parser::AdditiveExprPrime(std::unique_ptr<Expr> &operand1) {
#ifdef DEBUG
  std::cout << ">>> Additive Expression Prime" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::ADD) {
    Match(TOKEN::ADD);
    auto operand2 = MultiplicativeExpr();
    if (!operand2) {
      LexerPutBack(snapshot);
      return false;
    }
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::PLUS, operand1,
                                                    operand2, token);
  } else if (tag == TOKEN::SUB) {
    Match(TOKEN::SUB);
    auto operand2 = MultiplicativeExpr();
    if (!operand2) {
      LexerPutBack(snapshot);
      return false;
    }
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::MINUS, operand1,
                                                    operand2, token);
  } else {
    return true;
  }
  return AdditiveExprPrime(operand1);
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
#ifdef DEBUG
  std::cout << ">>> Shift Expression" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto operand1 = AdditiveExpr();
  if (!operand1) {
    LexerPutBack(snapshot);
    return nullptr;
  }
  if (ShiftExprPrime(operand1)) {
    return operand1;
  } else {
    LexerPutBack(snapshot);
    return nullptr;
  }
}

bool Parser::ShiftExprPrime(std::unique_ptr<Expr> &operand1) {
#ifdef DEBUG
  std::cout << ">>> Shift Expression Prime" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::LEFT_SHIFT) {
    Match(TOKEN::LEFT_SHIFT);
    auto operand2 = AdditiveExpr();
    if (!operand2) {
      LexerPutBack(snapshot);
      return false;
    }
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::LEFT_SHIFT, operand1,
                                                    operand2, token);
  } else if (tag == TOKEN::RIGHT_SHIFT) {
    Match(TOKEN::RIGHT_SHIFT);
    auto operand2 = AdditiveExpr();
    if (!operand2) {
      LexerPutBack(snapshot);
      return false;
    }
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::RIGHT_SHIFT, operand1,
                                                    operand2, token);
  } else {
    return true;
  }
  return ShiftExprPrime(operand1);
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
#ifdef DEBUG
  std::cout << ">>> Relational Expression" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto operand1 = ShiftExpr();
  if (!operand1) {
    LexerPutBack(snapshot);
    return nullptr;
  }
  RelationalExprPrime(operand1);
  return operand1;
}

bool Parser::RelationalExprPrime(std::unique_ptr<Expr> &operand1) {
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  auto tag = token->tag();

  if (tag == TOKEN::LESS) {
    Match(TOKEN::LESS);
    auto operand2 = ShiftExpr();
    if (!operand2) {
      LexerPutBack(snapshot);
      return false;
    }
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::LESS, operand1,
                                                    operand2, token);
  } else if (tag == TOKEN::GREATER) {
    Match(TOKEN::GREATER);
    auto operand2 = ShiftExpr();
    if (!operand2) {
      LexerPutBack(snapshot);
      return false;
    }
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::GREATER, operand1,
                                                    operand2, token);
  } else if (tag == TOKEN::LE) {
    Match(TOKEN::LE);
    auto operand2 = ShiftExpr();
    if (!operand2) {
      LexerPutBack(snapshot);
      return false;
    }
    operand1 =
        std::make_unique<BinaryOperatorExpr>(OP::LE, operand1, operand2, token);
  } else if (tag == TOKEN::GE) {
    Match(TOKEN::GE);
    auto operand2 = ShiftExpr();
    if (!operand2) {
      LexerPutBack(snapshot);
      return false;
    }
    operand1 =
        std::make_unique<BinaryOperatorExpr>(OP::GE, operand1, operand2, token);
  } else {
    return true;
  }
  return RelationalExprPrime(operand1);
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
#ifdef DEBUG
  std::cout << ">>> Equality Expression" << std::endl;
#endif
  auto operand1 = RelationalExpr();
  if (EqualityExprPrime(operand1)) {
    return operand1;
  } else {
    return nullptr;
  }
}

bool Parser::EqualityExprPrime(std::unique_ptr<Expr> &operand1) {
#ifdef DEBUG
  std::cout << ">>> Equality Expression Prime" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::EQ) {
    Match(TOKEN::EQ);
    auto operand2 = RelationalExpr();
    if (!operand2) {
      LexerPutBack(snapshot);
      return false;
    }
    operand1 =
        std::make_unique<BinaryOperatorExpr>(OP::EQ, operand1, operand2, token);
  } else if (tag == TOKEN::NE) {
    Match(TOKEN::NE);
    auto operand2 = RelationalExpr();
    if (!operand2) {
      LexerPutBack(snapshot);
      return false;
    }
    operand1 =
        std::make_unique<BinaryOperatorExpr>(OP::NE, operand1, operand2, token);
  } else {
    return true;
  }
  return EqualityExprPrime(operand1);
}

/**
 * AND-expression  ->
 *                            equality-expression
 *                          | AND-expression & equality-expression
 *
 * which is a left recursive formula and is needed to be re-written.
 */
std::unique_ptr<Expr> Parser::ANDExpr() {
#ifdef DEBUG
  std::cout << ">>> AND Expression" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto operand1 = EqualityExpr();
  if (!operand1) {
    LexerPutBack(snapshot);
    return nullptr;
  }
  if (ANDExprPrime(operand1)) {
    return operand1;
  } else {
    LexerPutBack(snapshot);
    return nullptr;
  }
}

bool Parser::ANDExprPrime(std::unique_ptr<Expr> &operand1) {
#ifdef DEBUG
  std::cout << ">>> AND Expression Prime" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::AND) {
    Match(TOKEN::AND);
    auto operand2 = EqualityExpr();
    if (!operand2) {
      LexerPutBack(snapshot);
      return false;
    }
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::AND, operand1, operand2,
                                                    token);
    return ANDExprPrime(operand1);
  } else {
    return true;
  }
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
#ifdef DEBUG
  std::cout << ">>> XOR Expression" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto operand1 = ANDExpr();
  if (!operand1) {
    LexerPutBack(snapshot);
    return nullptr;
  }
  if (XORExprPrime(operand1)) {
    return operand1;
  } else {
    LexerPutBack(snapshot);
    return nullptr;
  }
}

bool Parser::XORExprPrime(std::unique_ptr<Expr> &operand1) {
#ifdef DEBUG
  std::cout << ">>> XOR Expression Prime" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::XOR) {
    Match(TOKEN::XOR);
    auto operand2 = ANDExpr();
    if (!operand2) {
      LexerPutBack(snapshot);
      return false;
    }
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::XOR, operand1, operand2,
                                                    token);
    return XORExprPrime(operand1);
  } else {
    return true;
  }
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
#ifdef DEBUG
  std::cout << ">>> OR Expression" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto operand1 = XORExpr();
  if (!operand1) {
    LexerPutBack(snapshot);
    return nullptr;
  }
  if (ORExprPrime(operand1)) {
    return operand1;
  } else {
    LexerPutBack(snapshot);
    return nullptr;
  }
}

bool Parser::ORExprPrime(std::unique_ptr<Expr> &operand1) {
#ifdef DEBUG
  std::cout << ">>> OR Expression Prime" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::OR) {
    Match(TOKEN::OR);
    auto operand2 = XORExpr();
    if (!operand2) {
      LexerPutBack(snapshot);
      return false;
    }
    operand1 =
        std::make_unique<BinaryOperatorExpr>(OP::OR, operand1, operand2, token);
    return ORExprPrime(operand1);
  } else {
    return true;
  }
}

std::unique_ptr<Expr> Parser::LogicalANDExpr() {
#ifdef DEBUG
  std::cout << ">>> Logical AND Expression" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto operand1 = ORExpr();
  if (!operand1) {
    LexerPutBack(snapshot);
    return nullptr;
  }
  if (LogicalANDExprPrime(operand1)) {
    return operand1;
  } else {
    LexerPutBack(snapshot);
    return nullptr;
  }
}

bool Parser::LogicalANDExprPrime(std::unique_ptr<Expr> &operand1) {
#ifdef DEBUG
  std::cout << ">>> Logical AND Expression Prime" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::LOGICAL_AND) {
    Match(TOKEN::LOGICAL_AND);
    auto operand2 = ORExpr();
    if (!operand2) {
      LexerPutBack(snapshot);
      return false;
    }
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::LOGICAL_AND, operand1,
                                                    operand2, token);
    return LogicalANDExprPrime(operand1);
  } else {
    return true;
  }
}

std::unique_ptr<Expr> Parser::LogicalORExpr() {
#ifdef DEBUG
  std::cout << ">>> Logical OR Expression" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto operand1 = LogicalANDExpr();
  if (!operand1) {
    LexerPutBack(snapshot);
    return nullptr;
  }
  if (LogicalORExprPrime(operand1)) {
    return operand1;
  } else {
    LexerPutBack(snapshot);
    return nullptr;
  }
}

bool Parser::LogicalORExprPrime(std::unique_ptr<Expr> &operand1) {
#ifdef DEBUG
  std::cout << ">>> Logical OR Expression Prime" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::LOGICAL_OR) {
    Match(TOKEN::LOGICAL_OR);
    auto operand2 = LogicalANDExpr();
    if (!operand2) {
      LexerPutBack(snapshot);
      return false;
    }
    operand1 = std::make_unique<BinaryOperatorExpr>(OP::LOGICAL_OR, operand1,
                                                    operand2, token);
    return LogicalORExprPrime(operand1);
  } else {
    return true;
  }
}

std::unique_ptr<Expr> Parser::ConditionalExpr() {
#ifdef DEBUG
  std::cout << ">>> Conditianal Expression" << std::endl;
#endif
  auto snapshot = LexerSnapShot();
  auto cond = LogicalORExpr();
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::COND) {
    Match(TOKEN::COND);
    auto true_operand = Expression();
    if (!true_operand) {
      LexerPutBack(snapshot);
      return nullptr;
    }
    Match(TOKEN::COLON);
    auto false_operand = ConditionalExpr();
    if (!false_operand) {
      LexerPutBack(snapshot);
      return nullptr;
    }
    auto expr = std::make_unique<TenaryOperatorExpr>(
        OP::COND, OP::COLON, cond, true_operand, false_operand, token);
    return expr;
  } else {
    return cond;
  }
}

std::unique_ptr<Expr> Parser::AssignmentExpr() {
#ifdef DEBUG
  std::cout << ">>> Assignment Expression" << std::endl;
#endif
  auto conditional_expr = ConditionalExpr();
  if (conditional_expr) {
    return conditional_expr;
  } else {
    // assignment-expression
    //  -> unary-expression assignment-operator assignment-expression
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
    return expr;
  }
}

std::unique_ptr<Expr> Parser::ConstantExpr() { return ConditionalExpr(); }
