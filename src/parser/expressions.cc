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
    return expr;
  } else if (tag == TOKEN::GENERIC) {
    Error("Generic-selection is not supported now.");
    return nullptr;
  } else {
    return nullptr;
  }
}

std::unique_ptr<Expr> Parser::Expression() {
  auto snapshot = LexerSnapShot();
  auto expr = AssignmentExpr();
  if (!expr) {
#ifdef DEBUG
    std::cout << "Expression failed: in AssignmentExpr()" << std::endl;
#endif
    LexerPutBack(snapshot);
    return nullptr;
  }
#ifdef DEBUG
  print_line();
  std::cout << "Expression: succeeded: " << std::endl;
  std::cout << *expr << std::endl;
  print_line();
#endif
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
  auto snapshot = LexerSnapShot();
  auto expr = PrimaryExpression();
  if (!expr) {
#ifdef DEBUG
    std::cout << "Postfix Expression failed: in PrimaryExpression."
              << std::endl;
#endif
    LexerPutBack(snapshot);
    return nullptr;
  }
  auto postfix_expr_success = PostfixExprPrime(expr);
  if (postfix_expr_success) {
#ifdef DEBUG
    print_line();
    std::cout << "Postfix: succeeded: " << std::endl;
    std::cout << *expr << std::endl;
    print_line();
#endif
    return expr;
  } else {
#ifdef DEBUG
    std::cout << "Postfix Expression failed: in PostfixPrime(expr)."
              << std::endl;
#endif
    LexerPutBack(snapshot);
    return nullptr;
  }
  // TODO: support compound literal feature.
  // Error("Compound literals feature is not supported yet.");
}

bool Parser::PostfixExprPrime(std::unique_ptr<Expr> &expr) {
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
#ifdef DEBUG
    std::cout << "!!! Postfix Expression Prime failed!" << std::endl;
#endif
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
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  Match(TOKEN::LSQUBRKT);
  auto offset = Expression();
  if (!offset) {
#ifdef DEBUG
    std::cout << "!!! Array Subscripting failed!" << std::endl;
#endif
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
#ifdef DEBUG
      std::cout << "!!! Function Call failed!" << std::endl;
#endif
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
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::INCREMENT) {
    return PrefixIncrement();
  } else if (tag == TOKEN::DECREMENT) {
    auto expr = PrefixDecrement();
#ifdef DEBUG
    print_line();
    std::cout << "Unary Expr: succeeded: " << std::endl;
    std::cout << *expr << std::endl;
    print_line();
#endif
    return expr;
  } else if (tag == TOKEN::SIZEOF) {
    auto expr = Sizeof();
#ifdef DEBUG
    print_line();
    std::cout << "Unary Expr: succeeded: " << std::endl;
    std::cout << *expr << std::endl;
    print_line();
#endif
    return expr;
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
    auto expr = std::make_unique<UnaryOperatorExpr>(op, operand, token);
#ifdef DEBUG
    print_line();
    std::cout << "Unary Expr: succeeded: " << std::endl;
    std::cout << *expr << std::endl;
    print_line();
#endif
    return expr;
  } else {
    return PostfixExpr();
  }
}

std::unique_ptr<Expr> Parser::PrefixIncrement() {
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
  return expr;
}

std::unique_ptr<Expr> Parser::PrefixDecrement() {
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
  return expr;
}

std::unique_ptr<Expr> Parser::Sizeof() {
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
  return sizeof_expr;
}

// Expr *Parser::Alignof() { return nullptr; }

// Token *Parser::UnaryOperator() { return ConsumeToken(); }

std::unique_ptr<Expr> Parser::CastExpr() {
  if (PeekToken(TOKEN::LPAR) && false) {
    // TODO: Handle ( type-name ) cast-expresssion
    return nullptr;
  }
  auto snapshot = LexerSnapShot();
  auto expr = UnaryExpr();
  if (expr) {
    return expr;
  } else {
    LexerPutBack(snapshot);
    return nullptr;
  }
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
  auto snapshot = LexerSnapShot();
  auto expr1 = CastExpr();
  if (!expr1) {
#ifdef DEBUG
    std::cout << "MultiplicativeExpr: failed at CastExpr()." << std::endl;
#endif
    LexerPutBack(snapshot);
    return nullptr;
  }
#ifdef DEBUG
  print_line();
  std::cout << "MultiplicativeExpr: CastExpr succeeded: " << std::endl;
  std::cout << *expr1 << std::endl;
  print_line();
#endif
  if (MultiplicativeExprPrime(expr1)) {
#ifdef DEBUG
    print_line();
    std::cout << "MultiplicativeExpr: succeeded, and returned " << std::endl;
    std::cout << *expr1 << std::endl;
    print_line();
#endif
    return expr1;
  } else {
#ifdef DEBUG
    std::cout << "MultiplicativeExpr: failed at MultiplicativExprPrime(expr1)."
              << std::endl;
#endif
    LexerPutBack(snapshot);
    return nullptr;
  }
}

bool Parser::MultiplicativeExprPrime(std::unique_ptr<Expr> &operand1) {
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
  auto res = MultiplicativeExprPrime(operand1);
  if (res) {
    return true;
  } else {
    LexerPutBack(snapshot);
    return false;
  }
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
  auto snapshot = LexerSnapShot();
  auto operand1 = MultiplicativeExpr();
  if (!operand1) {
#ifdef DEBUG
    std::cout << "AdditiveExpr: failed at MultiplicativeExpr()." << std::endl;
#endif
    LexerPutBack(snapshot);
    return nullptr;
  }
#ifdef DEBUG
  print_line();
  std::cout << "AdditiveExpr: MultiplicativeExpr succeeded: " << std::endl;
  std::cout << *operand1 << std::endl;
  print_line();
#endif
  if (AdditiveExprPrime(operand1)) {
#ifdef DEBUG
    print_line();
    std::cout << "AdditiveExpr: succeeded: " << std::endl;
    std::cout << *operand1 << std::endl;
    print_line();
#endif
    return operand1;
  } else {
#ifdef DEBUG
    std::cout << "AdditiveExpr: failed at AdditiveExprPrime(operand1)."
              << std::endl;
#endif
    LexerPutBack(snapshot);
    return nullptr;
  }
}

bool Parser::AdditiveExprPrime(std::unique_ptr<Expr> &operand1) {
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
  auto res = AdditiveExprPrime(operand1);
  if (res) {
    return true;
  } else {
    LexerPutBack(snapshot);
    return false;
  }
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
  auto snapshot = LexerSnapShot();
  auto operand1 = AdditiveExpr();
  if (!operand1) {
#ifdef DEBUG
    std::cout << "ShiftExpr: failed at AdditiveExpr()." << std::endl;
#endif
    LexerPutBack(snapshot);
    return nullptr;
  }
  if (ShiftExprPrime(operand1)) {
#ifdef DEBUG
    print_line();
    std::cout << "ShiftExpr: succeeded: " << std::endl;
    std::cout << *operand1 << std::endl;
    print_line();
#endif
    return operand1;
  } else {
#ifdef DEBUG
    std::cout << "ShiftExpr: failed at ShiftExprPrime(operand1)." << std::endl;
#endif
    LexerPutBack(snapshot);
    return nullptr;
  }
}

bool Parser::ShiftExprPrime(std::unique_ptr<Expr> &operand1) {
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
  auto res = ShiftExprPrime(operand1);
  if (res) {
    return true;
  } else {
    LexerPutBack(snapshot);
    return false;
  }
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
  auto snapshot = LexerSnapShot();
  auto operand1 = ShiftExpr();
  if (!operand1) {
#ifdef DEBUG
    std::cout << "RelationalExpr: failed at ShiftExpr()." << std::endl;
#endif
    LexerPutBack(snapshot);
    return nullptr;
  }
#ifdef DEBUG
  print_line();
  std::cout << "RelationalExpr: succeeded: " << std::endl;
  std::cout << *operand1 << std::endl;
  print_line();
#endif
  if (RelationalExprPrime(operand1)) {
    return operand1;
  } else {
    LexerPutBack(snapshot);
    return nullptr;
  }
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
  auto res = RelationalExprPrime(operand1);
  if (res) {
    return true;
  } else {
    LexerPutBack(snapshot);
    return false;
  }
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
  auto snapshot = LexerSnapShot();
  auto operand1 = RelationalExpr();
  if (!operand1) {
    LexerPutBack(snapshot);
    return nullptr;
  }
  if (EqualityExprPrime(operand1)) {
    return operand1;
  } else {
    return nullptr;
  }
}

bool Parser::EqualityExprPrime(std::unique_ptr<Expr> &operand1) {
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
  auto res = EqualityExprPrime(operand1);
  if (res) {
    return true;
  } else {
    LexerPutBack(snapshot);
    return false;
  }
}

/**
 * AND-expression  ->
 *                            equality-expression
 *                          | AND-expression & equality-expression
 *
 * which is a left recursive formula and is needed to be re-written.
 */
std::unique_ptr<Expr> Parser::ANDExpr() {
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
    auto res = ANDExprPrime(operand1);
    if (res) {
      return true;
    } else {
      LexerPutBack(snapshot);
      return false;
    }
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
    auto res = ORExprPrime(operand1);
    if (res) {
      return true;
    } else {
      LexerPutBack(snapshot);
      return false;
    }
  } else {
    return true;
  }
}

std::unique_ptr<Expr> Parser::LogicalANDExpr() {
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
    auto res = LogicalANDExprPrime(operand1);
    if (res) {
      return true;
    } else {
      LexerPutBack(snapshot);
      return false;
    }
  } else {
    return true;
  }
}

std::unique_ptr<Expr> Parser::LogicalORExpr() {
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
    auto res = LogicalORExprPrime(operand1);
    if (res) {
      return true;
    } else {
      LexerPutBack(snapshot);
      return false;
    }
  } else {
    return true;
  }
}

std::unique_ptr<Expr> Parser::ConditionalExpr() {
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
  auto snapshot = LexerSnapShot();
  // assignment-expression
  //  -> unary-expression assignment-operator assignment-expression
  auto lhs = UnaryExpr();
  if (!lhs) {
    LexerPutBack(snapshot);
    auto conditional_expr = ConditionalExpr();
    if (!conditional_expr) {
      LexerPutBack(snapshot);
      return nullptr;
    } else {
      return conditional_expr;
    }
  }
#ifdef DEBUG
  print_line();
  std::cout << "AssignmentExpr: lhs succeeded: " << std::endl;
  std::cout << *lhs << std::endl;
  print_line();
#endif
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
  } else {
    LexerPutBack(snapshot);
    auto conditional_expr = ConditionalExpr();
    if (!conditional_expr) {
      LexerPutBack(snapshot);
      return nullptr;
    } else {
      return conditional_expr;
    }
  }
  ConsumeToken();
#ifdef DEBUG
  print_line();
  std::cout << "AssignmentExpr: OP succeeded: " << std::endl;
  std::cout << Expr::op_to_string[op] << std::endl;
  print_line();
#endif
  auto rhs = AssignmentExpr();
  if (!rhs) {
    auto conditional_expr = ConditionalExpr();
    if (!conditional_expr) {
      LexerPutBack(snapshot);
      return nullptr;
    } else {
#ifdef DEBUG
      print_line();
      std::cout << "AssignmentExpr: rhs failed." << std::endl;
      std::cout << "New conditional_expr: " << std::endl;
      std::cout << *conditional_expr << std::endl;
      print_line();
#endif
      return conditional_expr;
    }
  }
  auto expr = std::make_unique<BinaryOperatorExpr>(op, lhs, rhs, token);
#ifdef DEBUG
  print_line();
  std::cout
      << "AssignmentExpr: recognition succeeded. make BinaryOperatorExpr: "
      << std::endl;
  std::cout << *expr << std::endl;
  print_line();
#endif
  return expr;
}

std::unique_ptr<Expr> Parser::ConstantExpr() { return ConditionalExpr(); }
