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
// std::unique_ptr<Expr> Parser::PrimaryExpr() {
//   auto token = PeekToken();
//   switch (token->tag()) {
//   case TOKEN::IDENTIFIER:
//     Match(TOKEN::IDENTIFIER);
//     return;
//   case TOKEN::CONSTANT_START... TOKEN::CONSTANT_END:
//     ConsumeToken();
//     return Const();
//   case TOKEN::STRING_LITERAL:
//     Match(TOKEN::STRING_LITERAL);
//     return StrLiteral();
//   case TOKEN::LPAR:
//     Match(TOKEN::LPAR);
//     expr = Expression();
//     Match(TOKEN::RPAR);
//     break; // TODO:
//   case TOKEN::GENERIC:
//     // TODO
//     Error("Generic-selection is not supported now.");
//   default:
//     break;
//   }
//   return nullptr;
// }

// Identifier *Parser::Identf() {
//   auto token = Match(TOKEN::IDENTIFIER);
//   return new Identifier(token);
// }

// /**
//  *  constant  ->
//  *                integer-constant
//  *                floating-constant
//  *                enumeration-constant
//  *                character-constant
//  */
// Constant *Parser::Const() {
//   auto token = PeekToken();
//   assert(token->IsConstant());
//   // Must be one of the three, by the assertion.
//   switch (token->tag()) {
//   case TOKEN::INTEGER_CONTANT:
//     return IntegerConstant();
//   case TOKEN::FLOATING_CONSTANT:
//     return FloatingConstant();
//   case TOKEN::ENUMERATION_CONSTANT:
//     return EnumerationConstant();
//   case TOKEN::CHARACTER_CONSTANT:
//     return CharacterConstant();
//   default:
//     break;
//   }
//   return nullptr; // placeholder.
// }

// Constant *Parser::IntegerConstant() {
//   auto token = Match(TOKEN::INTEGER_CONTANT);
//   const long value = std::stol(token->value());
//   return new Constant(token, value);
// }

// Constant *Parser::FloatingConstant() {
//   auto token = Match(TOKEN::FLOATING_CONSTANT);
//   const double value = std::stod(token->value());
//   return new Constant(token, value);
// }

// Constant *Parser::EnumerationConstant() {
//   // TODO: Think about this case.
//   return nullptr;
// }

// Constant *Parser::CharacterConstant() {
//   auto token = ConsumeToken();
//   const char value = stoc(token->value());
//   return new Constant(token, value);
// }

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

// StringLiteral *Parser::StrLiteral() {
//   auto token = ConsumeToken();
//   const auto &str_value = token->value();
//   return new StringLiteral(token, str_value);
// }

// /**
//  * The left recursions are re-written as:
//  * postfix-expression  -> ( type-name ) { initialize-list }
//  postfix-expression'
//  *                      | ( type-name ) { initialize-list , }
//  * postfix-expression' | primary-expression postfix-expression'
//  * postfix-expression' -> [ expression ] postfix-expression'
//  *                      | ( argument-expression-list_{opt} )
//  postfix-expression'
//  *                      | .  identifier postfix-expression'
//  *                      | -> identifier postfix-expression'
//  *                      | ++ postfix-expression'
//  *                      | -- postfix-expression'
//  *                      | $epsilon$
//  */
// Expr *Parser::PostfixExpr() {
//   Expr *primary_expr = PrimaryExpr();
//   return PostfixExprPrime(primary_expr);
//   // TODO: support compound literal feature.
//   // Error("Compound literals feature is not supported yet.");
// }

// Expr *Parser::PostfixExprPrime(Expr *ptr) {
//   auto token = PeekToken();
//   switch (token->tag()) {
//   case TOKEN::LSQUBRKT:
//     ptr = ArraySubscripting(ptr);
//     break;
//   case TOKEN::LPAR:
//     ptr = FunctionCall(ptr);
//     break;
//   case TOKEN::DOT:
//   case TOKEN::PTR_MEM_REF:
//     ptr = MemberReference(ptr);
//     break;
//   case TOKEN::INCREMENT:
//     ptr = PostfixIncrement(ptr);
//     break;
//   case TOKEN::DECREMENT:
//     ptr = PostfixDecrement(ptr);
//     break;
//   default:
//     // TODO: Double-check the implementation here.
//     return ptr;
//   }
//   return PostfixExprPrime(ptr);
// }

// /**
//  * ptr [offset]:
//  * syntax sugar for equivalent expression:
//  * *(ptr + offset)
//  */
// Expr *Parser::ArraySubscripting(Expr *ptr) {
//   auto token = Match(TOKEN::LSQUBRKT);
//   auto offset = Expression();
//   Match(TOKEN::RSQUBRKT);
//   // Handle ptr_to = ptr + offset
//   auto ptr_to = new BinaryOperatorExpr(TOKEN::ADD, ptr, offset);
//   // return dereference(ptr_to)
//   return new UnaryOperatorExpr(TOKEN::STAR, ptr_to, token);
// }

// Expr *Parser::FunctionCall(Expr *designator) {
//   Match(TOKEN::LPAR);
//   std::list<Expr *> argument_list;
//   if (PeekToken(TOKEN::RPAR)) {
//     Match(TOKEN::RPAR);
//   } else {
//     // TODO: Handle argument-expression-list_{opt}
//     Match(TOKEN::RPAR);
//   }
//   // TODO: return FunctionCall node.
//   return new FunctionCallExpr(designator, argument_list);
// }

// Expr *Parser::MemberReference(Expr *ptr) {
//   if (PeekToken(TOKEN::PTR_MEM_REF)) {
//     Match(TOKEN::PTR_MEM_REF);
//   } else if (PeekToken(TOKEN::DOT)) {
//     Match(TOKEN::DOT);
//   } else {
//     Error("MemberReference error.");
//   }
//   auto token = PeekToken();
//   Match(TOKEN::IDENTIFIER);
//   // TODO: return pointer of MemberReference node.
//   return nullptr;
// }

// Expr *Parser::PostfixIncrement(Expr *operand) {
//   auto token = Match(TOKEN::INCREMENT);
//   // Return pointer of Increment node.
//   return new UnaryOperatorExpr(TOKEN::INCREMENT, operand, token);
// }

// Expr *Parser::PostfixDecrement(Expr *operand) {
//   auto token = Match(TOKEN::DECREMENT);
//   // Return pointer of Decrement node.
//   return new UnaryOperatorExpr(TOKEN::DECREMENT, operand, token);
// }

// Expr *Parser::CompoundLiterals(Expr *ptr) {
//   // TODO: implement compound literals feature.
//   return nullptr;
// }

// Expr *Parser::UnaryExpr() {
//   auto token = PeekToken();
//   Expr *operand = nullptr;
//   switch (token->tag()) {
//   case TOKEN::INCREMENT:
//     token = PrefixIncrement();
//     operand = UnaryExpr();
//     return new UnaryOperatorExpr(TOKEN::INCREMENT, operand, token);
//   case TOKEN::DECREMENT:
//     token = PrefixDecrement();
//     operand = UnaryExpr();
//     return new UnaryOperatorExpr(TOKEN::INCREMENT, operand, token);
//   case TOKEN::SIZEOF:
//     break;
//   case TOKEN::ALIGNOF:
//     break;
//   case TOKEN::AND:
//     token = UnaryOperator();
//     operand = CastExpr();
//     return new UnaryOperatorExpr(TOKEN::AND, operand, token);
//   case TOKEN::STAR:
//     token = UnaryOperator();
//     operand = CastExpr();
//     return new UnaryOperatorExpr(TOKEN::STAR, operand, token);
//   case TOKEN::ADD:
//     token = UnaryOperator();
//     operand = CastExpr();
//     return new UnaryOperatorExpr(TOKEN::ADD, operand, token);
//   case TOKEN::SUB:
//     token = UnaryOperator();
//     operand = CastExpr();
//     return new UnaryOperatorExpr(TOKEN::SUB, operand, token);
//   case TOKEN::NOT:
//     token = UnaryOperator();
//     operand = CastExpr();
//     return new UnaryOperatorExpr(TOKEN::NOT, operand, token);
//   case TOKEN::LOGICAL_NOT:
//     token = UnaryOperator();
//     operand = CastExpr();
//     return new UnaryOperatorExpr(TOKEN::NOT, operand, token);
//   default:
//     return PostfixExpr();
//   }
//   return nullptr; // placeholer
// }

// inline Token *Parser::PrefixIncrement() { return Match(TOKEN::INCREMENT); }

// inline Token *Parser::PrefixDecrement() { return Match(TOKEN::DECREMENT); }

// Expr *Parser::Sizeof() { return nullptr; }

// Expr *Parser::Alignof() { return nullptr; }

// Token *Parser::UnaryOperator() { return ConsumeToken(); }

// Expr *Parser::CastExpr() {
//   if (PeekToken(TOKEN::LPAR) && false) {
//     // TODO: Handle ( type-name ) cast-expresssion
//     return nullptr;
//   }
//   return UnaryExpr();
// }

// /**
//  * multiplicative-expression  ->
//  *                             cast-expression
//  *                           | multiplicative-expression * cast-expression
//  *                           | multiplicative-expression / cast-expression
//  *                           | multiplicative-expression % cast-expression
//  *
//  * which is a left recursive formula and can be re-written as:
//  * multiplicative-expression  ->
//  *                             cast-expression multiplicative-expression'
//  * multiplicative-expression' ->
//  *                             * cast-expression multiplicative-expression'
//  *                           | / cast-expression multiplicative-expression'
//  *                           | % cast-expression multiplicative-expression'
//  *                           | $\epsilon$
//  */
// Expr *Parser::MultiplicativeExpr() {
//   auto expr1 = CastExpr();
//   return MultiplicativeExprPrime(expr1);
// }

// Expr *Parser::MultiplicativeExprPrime(Expr *operand1) {
//   Token *token = PeekToken();
//   Expr *operand2 = nullptr;
//   Expr *expr_ptr = nullptr;
//   switch (token->tag()) {
//   case TOKEN::STAR:
//     Match(TOKEN::STAR);
//     operand2 = CastExpr();
//     expr_ptr = new BinaryOperatorExpr(TOKEN::STAR, operand1, operand2,
//     token); break;
//   case TOKEN::DIV:
//     Match(TOKEN::DIV);
//     operand2 = CastExpr();
//     expr_ptr = new BinaryOperatorExpr(TOKEN::DIV, operand1, operand2, token);
//     break;
//   case TOKEN::MOD:
//     Match(TOKEN::MOD);
//     operand2 = CastExpr();
//     expr_ptr = new BinaryOperatorExpr(TOKEN::MOD, operand1, operand2, token);
//     break;
//   default:
//     return operand1;
//   }
//   return MultiplicativeExprPrime(expr_ptr);
// }

// /**
//  * additive-expression  ->
//  *                          multiplicative-expression
//  *                          | additive-expression + multiplicative-expression
//  *                          | additive-expression - multiplicative-expression
//  *
//  * which is a left recursive formula and can be re-written as:
//  * additive-expression  ->
//  *                             multiplicative additive-expression'
//  * additive-expression' ->
//  *                             + multiplicative-expression
//  additive-expression'
//  *                           | - multiplicative-expression
//  additive-expression'
//  *                           | $\epsilon$
//  */
// Expr *Parser::AdditiveExpr() {
//   auto operand1 = MultiplicativeExpr();
//   return AdditiveExprPrime(operand1);
// }

// Expr *Parser::AdditiveExprPrime(Expr *operand1) {
//   Token *token = PeekToken();
//   Expr *operand2 = nullptr;
//   Expr *expr_ptr = nullptr;
//   switch (token->tag()) {
//   case TOKEN::ADD:
//     Match(TOKEN::ADD);
//     operand2 = MultiplicativeExpr();
//     expr_ptr = new BinaryOperatorExpr(TOKEN::ADD, operand1, operand2, token);
//     break;
//   case TOKEN::SUB:
//     Match(TOKEN::SUB);
//     operand2 = MultiplicativeExpr();
//     expr_ptr = new BinaryOperatorExpr(TOKEN::SUB, operand1, operand2, token);
//     break;
//   default:
//     return operand1;
//   }
//   return AdditiveExprPrime(expr_ptr);
// }

// /**
//  * shift-expression  ->
//  *                      additive-expression
//  *                    | shift-expression << additive-expression
//  *                    | shift-expression >> additive-expression
//  * which is a left recursive formula and can be re-written as:
//  *
//  * shift-expression  ->
//  *                      additive-expression shift-expression'
//  * shift-expression' ->
//  *                      << additive-expression shift-expression'
//  *                    | >> additive-expression shift-expression'
//  *                    | ${epsilon}
//  */
// Expr *Parser::ShiftExpr() {
//   auto operand1 = AdditiveExpr();
//   return ShiftExprPrime(operand1);
// }

// Expr *Parser::ShiftExprPrime(Expr *operand1) {
//   Token *token = PeekToken();
//   Expr *operand2 = nullptr;
//   Expr *expr_ptr = nullptr;
//   switch (token->tag()) {
//   case TOKEN::LEFT_SHIFT:
//     Match(TOKEN::LEFT_SHIFT);
//     operand2 = AdditiveExpr();
//     expr_ptr =
//         new BinaryOperatorExpr(TOKEN::LEFT_SHIFT, operand1, operand2, token);
//     break;
//   case TOKEN::RIGHT_SHIFT:
//     Match(TOKEN::RIGHT_SHIFT);
//     operand2 = AdditiveExpr();
//     expr_ptr =
//         new BinaryOperatorExpr(TOKEN::RIGHT_SHIFT, operand1, operand2,
//         token);
//     break;
//   default:
//     return operand1;
//   }
//   return ShiftExprPrime(expr_ptr);
// }

// /**
//  * relational-expression  ->
//  *                            shift-expression
//  *                          | relational-expression < shift-expression
//  *                          | relational-expression > shift-expression
//  *                          | relational-expression <= shift-expression
//  *                          | relational-expression >= shift-expression
//  *
//  * which is a left recursive formula and can be re-written as:
//  *
//  * relational-expression  ->
//  *                            shift-expression relational-expression'
//  * relational-expression' ->  < shift-expression relational-expression'
//  *                          | > shift-expression relational-expression'
//  *                          | >= shift-expression relational-expression'
//  *                          | <= shift-expression relational-expression'
//  *                          | ${epsilon}
//  */
// Expr *Parser::RelationalExpr() {
//   auto operand1 = ShiftExpr();
//   return RelationalExprPrime(operand1);
// }

// Expr *Parser::RelationalExprPrime(Expr *operand1) {
//   Token *token = PeekToken();
//   Expr *operand2 = nullptr;
//   Expr *expr_ptr = nullptr;
//   switch (token->tag()) {
//   case TOKEN::LESS:
//     Match(TOKEN::LESS);
//     operand2 = ShiftExpr();
//     expr_ptr = new BinaryOperatorExpr(TOKEN::LESS, operand1, operand2,
//     token); break;
//   case TOKEN::GREATER:
//     Match(TOKEN::GREATER);
//     operand2 = ShiftExpr();
//     expr_ptr =
//         new BinaryOperatorExpr(TOKEN::GREATER, operand1, operand2, token);
//     break;
//   case TOKEN::LE:
//     Match(TOKEN::LE);
//     operand2 = ShiftExpr();
//     expr_ptr = new BinaryOperatorExpr(TOKEN::LE, operand1, operand2, token);
//     break;
//   case TOKEN::GE:
//     Match(TOKEN::GE);
//     operand2 = ShiftExpr();
//     expr_ptr = new BinaryOperatorExpr(TOKEN::GE, operand1, operand2, token);
//     break;
//   default:
//     return operand1;
//   }
//   return RelationalExprPrime(expr_ptr);
// }

// /**
//  * equality-expression  ->
//  *                            relational-expression
//  *                          | equality-expression == relational-expression
//  *                          | equality-expression != relational-expression
//  *
//  * which is a left recursive formula and can be re-written as:
//  *
//  * equality-expression  ->
//  *                            relational-expression equality-expression'
//  * equality-expression' ->
//  *                            == relational-expression equality-expression'
//  *                          | != relational-expression equality-expression'
//  *                          | ${epsilon}
//  */
// Expr *Parser::EqualityExpr() {
//   Expr *operand1 = RelationalExpr();
//   return EqualityExprPrime(operand1);
// }

// Expr *Parser::EqualityExprPrime(Expr *operand1) {
//   Token *token = PeekToken();
//   Expr *operand2 = nullptr;
//   Expr *expr_ptr = nullptr;
//   switch (token->tag()) {
//   case TOKEN::EQ:
//     Match(TOKEN::EQ);
//     operand2 = RelationalExpr();
//     expr_ptr = new BinaryOperatorExpr(TOKEN::EQ, operand1, operand2, token);
//     break;
//   case TOKEN::NE:
//     Match(TOKEN::NE);
//     operand2 = RelationalExpr();
//     expr_ptr = new BinaryOperatorExpr(TOKEN::NE, operand1, operand2, token);
//     break;
//   default:
//     return operand1;
//   }
//   return EqualityExprPrime(expr_ptr);
// }

// /**
//  * AND-expression  ->
//  *                            equality-expression
//  *                          | AND-expression & equality-expression
//  *
//  * which is a left recursive formula and is needed to be re-written.
//  */
// Expr *Parser::ANDExpr() {
//   auto operand1 = EqualityExpr();
//   return ANDExprPrime(operand1);
// }

// Expr *Parser::ANDExprPrime(Expr *operand1) {
//   Token *token = PeekToken();
//   Expr *operand2 = nullptr;
//   Expr *expr_ptr = nullptr;
//   switch (token->tag()) {
//   case TOKEN::AND:
//     Match(TOKEN::AND);
//     operand2 = EqualityExpr();
//     expr_ptr = new BinaryOperatorExpr(TOKEN::AND, operand1, operand2, token);
//     break;
//   default:
//     return operand1;
//   }
//   return ANDExprPrime(expr_ptr);
// }

// /**
//  * XOR-expression   ->
//  *                        AND-expression
//  *                      | XOR-expression ^ AND-expression
//  *
//  * which is a left-recursive expression, and needed to be rewritten as:
//  *
//  * XOR-expression   ->    AND-expression XOR-expression'
//  * XOR-expression'  ->    ^ AND-expression XOR-expression'
//  *                      | ${epsilon}
//  */
// Expr *Parser::XORExpr() {
//   auto operand1 = ANDExpr();
//   return XORExprPrime(operand1);
// }

// Expr *Parser::XORExprPrime(Expr *operand1) {
//   Token *token = PeekToken();
//   Expr *operand2 = nullptr;
//   Expr *expr_ptr = nullptr;
//   switch (token->tag()) {
//   case TOKEN::XOR:
//     Match(TOKEN::XOR);
//     operand2 = ANDExpr();
//     expr_ptr = new BinaryOperatorExpr(TOKEN::XOR, operand1, operand2, token);
//     break;
//   default:
//     return operand1;
//   }
//   return XORExprPrime(expr_ptr);
// }

// /**
//  * OR-expression    ->
//  *                      XOR-expression
//  *                      OR-expression | XOR-expression
//  * which is a left recursive expression, and needed to be rewritten as:
//  * OR-expression    ->
//  *                      XOR-expression OR-expression'
//  * OR-expression'   ->  | XOR-expression OR-expression'
//  *                  ->  ${epsilon}
//  */
// Expr *Parser::ORExpr() {
//   auto operand1 = XORExpr();
//   return ORExprPrime(operand1);
// }

// Expr *Parser::ORExprPrime(Expr *operand1) {
//   Token *token = PeekToken();
//   Expr *operand2 = nullptr;
//   Expr *expr_ptr = nullptr;
//   switch (token->tag()) {
//   case TOKEN::OR:
//     Match(TOKEN::OR);
//     operand2 = XORExpr();
//     expr_ptr = new BinaryOperatorExpr(TOKEN::OR, operand1, operand2, token);
//     break;
//   default:
//     return operand1;
//   }
//   return ORExprPrime(expr_ptr);
// }

// Expr *Parser::LogicalANDExpr() {
//   auto operand1 = ORExpr();
//   return LogicalANDExprPrime(operand1);
// }

// Expr *Parser::LogicalANDExprPrime(Expr *operand1) {
//   Token *token = PeekToken();
//   Expr *operand2 = nullptr;
//   Expr *expr_ptr = nullptr;
//   switch (token->tag()) {
//   case TOKEN::LOGICAL_AND:
//     Match(TOKEN::LOGICAL_AND);
//     operand2 = ORExpr();
//     expr_ptr =
//         new BinaryOperatorExpr(TOKEN::LOGICAL_AND, operand1, operand2,
//         token);
//     break;
//   default:
//     return operand1;
//   }
//   return LogicalANDExprPrime(expr_ptr);
// }

// Expr *Parser::LogicalORExpr() {
//   auto operand1 = LogicalANDExpr();
//   return LogicalORExprPrime(operand1);
// }

// Expr *Parser::LogicalORExprPrime(Expr *operand1) {
//   Token *token = PeekToken();
//   Expr *operand2 = nullptr;
//   Expr *expr_ptr = nullptr;
//   switch (token->tag()) {
//   case TOKEN::LOGICAL_OR:
//     Match(TOKEN::LOGICAL_OR);
//     operand2 = LogicalANDExpr();
//     expr_ptr =
//         new BinaryOperatorExpr(TOKEN::LOGICAL_OR, operand1, operand2, token);
//     break;
//   default:
//     return operand1;
//   }
//   return LogicalORExprPrime(expr_ptr);
// }

// Expr *Parser::ConditionalExpr() {
//   auto cond = ConditionalExpr();
//   auto token = PeekToken();
//   if (token->tag() == TOKEN::COND) {
//     Match(TOKEN::COND);
//     auto true_operand = Expression();
//     Match(TOKEN::COLON);
//     auto false_operand = ConditionalExpr();
//     return new TenaryOperatorExpr(TOKEN::COND, TOKEN::COLON, cond,
//     true_operand,
//                                   false_operand, token);
//   }
//   return cond;
// }

// Expr *Parser::AssignmentExpr() { return nullptr; }

// Expr *Parser::ConstantExpr() { return ConditionalExpr(); }
