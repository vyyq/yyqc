#ifndef _STMT_H_
#define _STMT_H_
#include "../symbol/scope.h"
#include "../type/type_base.h"
#include "ast_base.h"
#include <atomic>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class Type;

class LabeledStmt : public Stmt {
private:
  std::string _label;

public:
  const std::string &label() const { return _label; }
};

class CompoundStmt : public Stmt {
private:
  std::vector<std::unique_ptr<Stmt>> _stmts;
  std::shared_ptr<Scope> _self_scope = nullptr;

public:
  CompoundStmt() = default;
  std::vector<std::unique_ptr<Stmt>> &stmts() { return _stmts; }
  void AddStmts(std::vector<std::unique_ptr<Stmt>> &stmts) {
    _stmts.insert(_stmts.end(), std::make_move_iterator(stmts.begin()),
                  std::make_move_iterator(stmts.end()));
  }
  void set_scope(std::shared_ptr<Scope> &scope) { _self_scope = scope; }
};

class SelectionStmt : public Stmt {};

class IfStmt : public SelectionStmt {
public:
  IfStmt(std::unique_ptr<Expr> &condition, std::unique_ptr<Stmt> &if_stmt,
         std::unique_ptr<Stmt> &else_stmt)
      : _condition_expr(std::move(condition)), _if_stmt(std::move(if_stmt)),
        _else_stmt(std::move(else_stmt)) {}

private:
  std::unique_ptr<Expr> _condition_expr;
  std::unique_ptr<Stmt> _if_stmt;
  std::unique_ptr<Stmt> _else_stmt;
};

class SwitchStmt : public SelectionStmt {
public:
  SwitchStmt(Expr *selection) : _selection_expr(selection) {}

private:
  std::unique_ptr<Expr> _selection_expr;
  std::pair<Constant *, Stmt *> _value_stmt_pair;
};

class ExpressionStmt : public Stmt {
private:
  std::unique_ptr<Expr> _expression;

public:
  ExpressionStmt(std::unique_ptr<Expr> &expression)
      : _expression(std::move(expression)) {}
  std::unique_ptr<Expr> &expression() { return _expression; }
  void set_expression(std::unique_ptr<Expr> &expression) {
    _expression = std::move(expression);
  }
};

class IterationStmt : public Stmt {
private:
  std::unique_ptr<Expr> _condition;
  std::unique_ptr<Stmt> _loop_body;
  bool _execute_first = false;

public:
  IterationStmt(std::unique_ptr<Expr> &condition,
                std::unique_ptr<Stmt> &loop_body, bool execute_first)
      : _condition(std::move(condition)), _loop_body(std::move(loop_body)),
        _execute_first(execute_first) {}
  bool execute_before_condition() { return _execute_first; }
};

class WhileStmt : public IterationStmt {
public:
  WhileStmt(std::unique_ptr<Expr> &condition, std::unique_ptr<Stmt> &loop_body)
      : IterationStmt(condition, loop_body, false) {}
};

class DoWhileStmt : public IterationStmt {
public:
  DoWhileStmt(std::unique_ptr<Expr> &condition,
              std::unique_ptr<Stmt> &loop_body)
      : IterationStmt(condition, loop_body, true) {}
};

class ForStmt : public IterationStmt {
public:
  ForStmt(std::unique_ptr<Expr> &condition, std::unique_ptr<Stmt> &body)
      : IterationStmt(condition, body, false) {}
};

class JumpStmt : public Stmt {
private:
  std::unique_ptr<Stmt> _jump_to;

public:
  JumpStmt(Stmt *jump_to) : _jump_to(jump_to) {}
};

class GotoStmt : public JumpStmt {
public:
  GotoStmt(Stmt *jump_to, Token *ident_token)
      : JumpStmt(jump_to), _ident_token(ident_token) {}

private:
  std::shared_ptr<Token> _ident_token = nullptr;
};

class ContinueStmt : public JumpStmt {
public:
  ContinueStmt(Stmt *jump_to) : JumpStmt(jump_to) {}
};

class BreakStmt : public JumpStmt {
public:
  BreakStmt(Stmt *jump_to) : JumpStmt(jump_to) {}
};

class ReturnStmt : public JumpStmt {
public:
  ReturnStmt(Stmt *jump_to, Expr *returned)
      : JumpStmt(jump_to), _returned(returned) {}

private:
  std::unique_ptr<Expr> _returned;
};

#endif
