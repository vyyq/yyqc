#include "parser.h"
#include <memory>

// Statements

/**
 *  statement ->
 *                labeled-statement
 *                compound-statement
 *                expression-statement
 *                selection-statement
 *                iteration-statement
 *                jump-statement
 */
std::unique_ptr<Stmt> Parser::Statement() { return nullptr; }

/**
 *  labeled-statement ->
 *                identifier : statement
 *                case constant-expression : statement
 *                default : statement
 */
std::unique_ptr<LabeledStmt> Parser::LabeledStatement() {
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::IDENTIFIER) {
    token = Match(TOKEN::IDENTIFIER);
    Match(TOKEN::COLON);
  } else if (tag == TOKEN::CASE) {
    Match(TOKEN::CASE);
    Match(TOKEN::COLON);

  } else {
    Match(TOKEN::DEFAULT);
    Match(TOKEN::COLON);
  }
  return nullptr;
}

/**
 * compound-statement ->
 *                { block-item-list_{opt} }
 */
std::unique_ptr<CompoundStmt> Parser::CompoundStatement() {
  Match(TOKEN::LBRACE);
  EnterNewSubScope();
  auto tag = PeekToken()->tag();
  CompoundStmt *compound_stmt = nullptr;
  if (tag != TOKEN::RBRACE) {
    std::list<ASTNode *> block_item_list = BlockItemList();
    compound_stmt = new CompoundStmt(block_item_list);
  } else {
    compound_stmt = new CompoundStmt();
  }
  ExitCurrentSubScope();
  Match(TOKEN::RBRACE);
  //return compound_stmt;
  return nullptr;
}

/**
 *  expression-statement  ->
 *                expression_{opt};
 */
std::unique_ptr<ExpressionStmt> Parser::ExpressionStatement() { return nullptr; }

/**
 *  selection-statement ->
 *                if ( expression ) statement
 *                if ( expression ) statement else statement
 *                switch ( expression ) statement
 */
std::unique_ptr<SelectionStmt> Parser::SelectionStatement() {
  auto token = PeekToken();
  if (token->tag() == TOKEN::IF) {
    Match(TOKEN::IF);
    Match(TOKEN::LPAR);
    // auto condition = Expression();
    Match(TOKEN::RPAR);
    // Stmt *if_stmt = Statement();
    // Stmt *else_stmt = nullptr;
    token = PeekToken();
    if (token->tag() == TOKEN::ELSE) {
      Match(TOKEN::ELSE);
      // else_stmt = Statement();
    }
    // return new IfStmt(condition, if_stmt, else_stmt);
    return nullptr;
  } else {
    Match(TOKEN::SWITCH);
    Match(TOKEN::LPAR);
    // auto choices = Expression();
    Match(TOKEN::RPAR);
    auto stmt = Statement();
    // return new SwitchStmt(choices);
    return nullptr;
  }
}

/**
 *  iteration-statement ->
 *      while ( expression ) statement
 *      do statement while ( expression ) ;
 *      for ( expression_{opt}; expression_{opt}; expression_{opt} ) statement
 *      for ( declaration expression_{opt} ; expression_{opt} ) statement
 */
std::unique_ptr<IterationStmt> Parser::IterationStatement() {
  auto tag = PeekToken()->tag();
  if (tag == TOKEN::WHILE) {
    Match(TOKEN::WHILE);
    Match(TOKEN::LPAR);
    // auto condition = Expression();
    Match(TOKEN::RPAR);
    auto body = Statement();
    // "next field" in while should be evaluate later.
    // return new WhileStmt(condition, body, nullptr);
    return nullptr;
  } else if (tag == TOKEN::DO) {
    Match(TOKEN::DO);
    auto body = Statement();
    Match(TOKEN::WHILE);
    Match(TOKEN::LPAR);
    // auto condition = Expression();
    Match(TOKEN::RPAR);
    Match(TOKEN::SEMI);
    // return new DoWhileStmt(condition, body, nullptr);
    return nullptr;
  } else if (tag == TOKEN::FOR) {
    Match(TOKEN::FOR);
    Match(TOKEN::LPAR);
    // TODO: Complete for loop recognition.
    // return new ForStmt(nullptr, nullptr, nullptr);
    return nullptr;
  } else {
    Error("iteration-statement should start with while or for.");
    return nullptr;
  }
}

/**
 *  jump-statement  ->
 *      goto identifier ;
 *      continue ;
 *      break ;
 *      return expression_{opt} ;
 */
std::unique_ptr<JumpStmt> Parser::JumpStatement() {
  auto tag = PeekToken()->tag();
  if (tag == TOKEN::GOTO) {
    Match(TOKEN::GOTO);
    auto ident_token = Match(TOKEN::IDENTIFIER);
    Match(TOKEN::SEMI);
    // return new GotoStmt(nullptr, ident_token, nullptr);
    return nullptr;
    // TODO: Reconsider those nullptr
  } else if (tag == TOKEN::CONTINUE) {
    Match(TOKEN::CONTINUE);
    // return new ContinueStmt(nullptr);
    return nullptr;
  } else if (tag == TOKEN::BREAK) {
    Match(TOKEN::BREAK);
    // return new BreakStmt(nullptr);
    return nullptr;
  } else if (tag == TOKEN::RETURN) {
    // If (expression)
    // auto returned = Expression();
    // return new ReturnStmt(nullptr, returned);
    return nullptr;
  } else {
    Error("error in jump-statement!");
    return nullptr;
  }
}

/**
 *  block-item-list ->
 *      block-item
 *      block-item-list block-item
 */
std::list<ASTNode *> Parser::BlockItemList() {
  std::list<ASTNode *> block_item_list;
  auto token = PeekToken();
  do {
    auto block_item = BlockItem();
    block_item_list.push_back(block_item);
  } while (token->tag() != TOKEN::RBRACE);
  return block_item_list;
}

/**
 *  block-item  ->
 *                  declaration
 *                  statement
 */
ASTNode *Parser::BlockItem() { return nullptr; }
