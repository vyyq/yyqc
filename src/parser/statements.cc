#include "parser.h"
#include <memory>
#include <utility>

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
std::unique_ptr<Stmt> Parser::Statement() {
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::WHILE || tag == TOKEN::DO || tag == TOKEN::FOR) {
    auto iteration_stmt = IterationStatement();
    if (!iteration_stmt) {
      LexerPutBack(snapshot);
      return nullptr;
    }
    return iteration_stmt;
  } else if (tag == TOKEN::GOTO || tag == TOKEN::CONTINUE ||
             tag == TOKEN::BREAK || tag == TOKEN::RETURN) {
    auto jump_stmt = JumpStatement();
    if (!jump_stmt) {
      LexerPutBack(snapshot);
      return nullptr;
    }
    return jump_stmt;
  } else if (tag == TOKEN::IF || tag == TOKEN::SWITCH) {
    auto selection_stmt = SelectionStatement();
    if (!selection_stmt) {
      LexerPutBack(snapshot);
      return nullptr;
    }
    return selection_stmt;
  } else if (tag == TOKEN::LBRACE) {
    auto compound_stmt = CompoundStatement();
    if (!compound_stmt) {
      LexerPutBack(snapshot);
      return nullptr;
    }
    return compound_stmt;
  } else if (tag == TOKEN::CASE || tag == TOKEN::DEFAULT ||
             (tag == TOKEN::IDENTIFIER &&
              PeekNextToken()->tag() == TOKEN::COLON)) {
    auto labeled_stmt = LabeledStatement();
    if (!labeled_stmt) {
      LexerPutBack(snapshot);
      return nullptr;
    }
    return labeled_stmt;
  } else {
    auto expression_pair = ExpressionStatement();
    if (!expression_pair.first) {
      LexerPutBack(snapshot);
      return nullptr;
    }
    auto expression_stmt = std::move(expression_pair.second);
    return expression_stmt;
  }
}

/**
 *  labeled-statement ->
 *                identifier : statement
 *                case constant-expression : statement
 *                default : statement
 */
std::unique_ptr<LabeledStmt> Parser::LabeledStatement() {
  // TODO
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
  auto snapshot = LexerSnapShot();
  Match(TOKEN::LBRACE);
  EnterNewSubScope();
  auto tag = PeekToken()->tag();
  auto compound_stmt = std::make_unique<CompoundStmt>();
  compound_stmt->set_scope(_current_scope);
  if (tag != TOKEN::RBRACE) {
    auto pair = BlockItemList();
    if (pair.first) {
      compound_stmt->AddStmts(pair.second);
    } else {
      LexerPutBack(snapshot);
      return nullptr;
    }
  }
  ExitCurrentSubScope();
  Match(TOKEN::RBRACE);
  return compound_stmt;
}

/**
 *  expression-statement  ->
 *                expression_{opt};
 */
std::pair<bool, std::unique_ptr<ExpressionStmt>> Parser::ExpressionStatement() {
  auto snapshot = LexerSnapShot();
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::SEMI) {
    Match(TOKEN::SEMI);
    return std::make_pair(true, nullptr);
  } else {
    auto expression = Expression();
    if (!expression) {
      LexerPutBack(snapshot);
      return std::make_pair(false, nullptr);
    }
    auto expression_stmt = std::make_unique<ExpressionStmt>(expression);
    Match(TOKEN::SEMI);
    return std::make_pair(true, std::move(expression_stmt));
  }
}

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
    auto condition = Expression();
    Match(TOKEN::RPAR);
    auto true_stmt = Statement();
    std::unique_ptr<Stmt> false_stmt = nullptr;
    token = PeekToken();
    if (token->tag() == TOKEN::ELSE) {
      Match(TOKEN::ELSE);
      false_stmt = Statement();
    }
    return std::make_unique<IfStmt>(condition, true_stmt, false_stmt);
  } else {
    // Match(TOKEN::SWITCH);
    // Match(TOKEN::LPAR);
    // Match(TOKEN::RPAR);
    // auto stmt = Statement();
    // TODO: support switch statement.
    std::cerr << "switch statement is not supported yet." << std::endl;
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
    auto condition = Expression();
    Match(TOKEN::RPAR);
    auto body = Statement();
    // "next field" in while should be evaluate later.
    // return new WhileStmt(condition, body, nullptr);
    auto while_stmt = std::make_unique<WhileStmt>(condition, body);
    return while_stmt;
  } else if (tag == TOKEN::DO) {
    Match(TOKEN::DO);
    auto body = Statement();
    Match(TOKEN::WHILE);
    Match(TOKEN::LPAR);
    auto condition = Expression();
    Match(TOKEN::RPAR);
    Match(TOKEN::SEMI);
    auto do_while_stmt = std::make_unique<DoWhileStmt>(condition, body);
    return do_while_stmt;
  } else if (tag == TOKEN::FOR) {
    Match(TOKEN::FOR);
    Match(TOKEN::LPAR);
    // TODO: Complete for loop recognition.
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
  // TODO
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
std::pair<bool, std::vector<std::unique_ptr<Stmt>>> Parser::BlockItemList() {
  auto snapshot = LexerSnapShot();
  std::vector<std::unique_ptr<Stmt>> stmt_items;
  auto token = PeekToken();
  while (PeekToken()->tag() != TOKEN::RBRACE) {
    auto declaration = Declaration();
    if (declaration.size() == 0) {
      auto statement = Statement();
      if (statement) {
        stmt_items.push_back(std::move(statement));
      } else {
        LexerPutBack(snapshot);
        return std::make_pair(false, std::vector<std::unique_ptr<Stmt>>());
      }
    } else {
      _current_scope.lock()->AddSymbols(declaration);
    }
  }
  return std::make_pair(true, std::move(stmt_items));
}

