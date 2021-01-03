#ifndef _STMT_H_
#define _STMT_H_
#include "ast_base.h"
#include "../type/type_base.h"
#include <atomic>
#include <string>
#include <vector>
#include <utility>
#include <list>

class Type;

class LabeledStmt : public Stmt {
private:
  std::string _label;

public:
  const std::string &label() const { return _label; }
};

class CompoundStmt : public Stmt {
private:
  std::list<ASTNode *> _stmts;

public:
  CompoundStmt() = default;
  CompoundStmt(std::list<ASTNode *> stmts)
    : _stmts(stmts) {}
  const std::list<ASTNode *> stmts() const { return _stmts; }
};

class SelectionStmt : public Stmt {

};

class IfStmt : public SelectionStmt {
public:
  IfStmt(Expr *condition, Stmt *if_stmt, Stmt *else_stmt) 
    : _condition_expr(condition), _if_stmt(if_stmt), _else_stmt(else_stmt) {}
private:
  Expr *_condition_expr;
  Stmt *_if_stmt;
  Stmt *_else_stmt;
};

class SwitchStmt : public SelectionStmt {
public:
  SwitchStmt(Expr *selection)
    : _selection_expr(selection) {}
private:
  Expr *_selection_expr;
  std::pair<Constant *, Stmt *> _value_stmt_pair;
};

class ExpressionStmt : public Stmt {
};

class IterationStmt : public Stmt {
private:
  Expr *_condition;
  Stmt *_start;
  Stmt *_next;
  bool _execute_first = false;

public:
  IterationStmt(Expr *condition, Stmt *start, Stmt *next, bool execute_first)
      : _condition(condition), _start(start), _next(next), _execute_first(execute_first) {}
};

class WhileStmt : public IterationStmt {
  public:
    WhileStmt(Expr *condition, Stmt *start, Stmt *next)
      : IterationStmt(condition, start, next, false) {}
};

class DoWhileStmt : public IterationStmt {
public:
  DoWhileStmt(Expr *condition, Stmt *start, Stmt *next)
    : IterationStmt(condition, start, next, true) {}
};

class ForStmt : public IterationStmt {
public:
  ForStmt(Expr *condition, Stmt *body, Stmt *next)
    : IterationStmt(condition, body, next, false) {}
};


class JumpStmt : public Stmt {
private:
  Stmt *_jump_to;

public:
  JumpStmt(Stmt *jump_to)
      : _jump_to(jump_to) {}
};

class GotoStmt : public JumpStmt {
public:
  GotoStmt(Stmt *jump_to, Token *ident_token, Identifier *identifier)
    : JumpStmt(jump_to), _ident_token(ident_token), _identifier(identifier) {}
private:
  Token *_ident_token = nullptr;
  Identifier *_identifier = nullptr;
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
  ReturnStmt(Stmt *jump_to, Expr *returned) : JumpStmt(jump_to), _returned(returned) {}
private:
  Expr *_returned;
};

#endif
