#ifndef _PARSER_H_
#define _PARSER_H_

#include "../ast/expr.h"
#include "../ast/stmt.h"
#include "../error/error.h"
#include "../lexer/lexer.h"
#include "../symbol/scope.h"
#include "../type/type_arithmetic.h"
#include "../type/type_base.h"
#include "../type/type_derived.h"
#include "../util/print_info.h"
#include <cassert>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>

#define DEBUG

bool IsSpecifier(TOKEN tag);
char stoc(const std::string &);

class Parser {
private:
  std::shared_ptr<Token> PeekToken() const {
    return _lexer->PeekCurrentToken();
  }
  bool PeekToken(TOKEN tag) const { return tag == PeekToken()->tag(); }
  std::shared_ptr<Token> PeekNextToken() const {
    return _lexer->PeekNextToken();
  }
  bool PeekNextToken(TOKEN tag) const { return tag == PeekNextToken()->tag(); }
  std::shared_ptr<Token> ConsumeToken() { return _lexer->ConsumeToken(); }
  std::shared_ptr<Token> Match(TOKEN tag) {
#ifdef DEBUG
    std::cout << "Match: " << Token::tag_to_string[tag] << " ----> "
              << PeekToken()->position() << std::endl;
    std::cout << "Current Token: " << Token::tag_to_string[PeekToken()->tag()] << std::endl;
    std::cout << "Next Token: " << Token::tag_to_string[PeekNextToken()->tag()] << std::endl;
#endif // DEBUG
    assert(PeekToken(tag));
    return ConsumeToken();
  }
  unsigned LexerSnapShot() { return _lexer->ScreenShot(); }
  void LexerPutBack(unsigned screenshot) { return _lexer->PutBack(screenshot); }
  std::weak_ptr<Scope> &CurrentScope() { return _current_scope; }
  bool isRootScope() { return _current_scope.lock()->parent().lock() == nullptr; }

public:
  void EnterNewSubScope() {
    auto &curr = _current_scope.lock()->AddSubScope();
    _current_scope = curr;
  }
  void ExitCurrentSubScope() {
    _current_scope = _current_scope.lock()->parent();
  }
  explicit Parser(const std::string &filename)
      : _lexer(std::make_unique<Lexer>(filename)),
        _root_scope(std::make_shared<Scope>()), _current_scope(_root_scope) {}
  ~Parser() = default;
  bool Scan() {
    auto result = TranslationUnit();
    if (result) {
      std::cout << "Parser::Scan finished! Found no errors." << std::endl;
      return true;
    } else {
      std::cout << "Parser::Scan found error(s)." << std::endl;
      return false;
    }
  }

  // Expressions
  std::unique_ptr<Expr> Expression();
  std::unique_ptr<Expr> PrimaryExpression();
  std::unique_ptr<Expr> PostfixExpr();
  std::unique_ptr<Expr> UnaryExpr();
  // Expr *Alignof();
  std::unique_ptr<Expr> CastExpr();
  std::unique_ptr<Expr> MultiplicativeExpr();
  std::unique_ptr<Expr> AdditiveExpr();
  std::unique_ptr<Expr> ShiftExpr();
  std::unique_ptr<Expr> RelationalExpr();
  std::unique_ptr<Expr> EqualityExpr();
  std::unique_ptr<Expr> ANDExpr();
  std::unique_ptr<Expr> XORExpr();
  std::unique_ptr<Expr> ORExpr();
  std::unique_ptr<Expr> LogicalANDExpr();
  std::unique_ptr<Expr> LogicalORExpr();
  std::unique_ptr<Expr> ConditionalExpr();
  std::unique_ptr<Expr> AssignmentExpr();
  std::unique_ptr<Expr> ConstantExpr();

  // Declarators
  std::vector<std::unique_ptr<Symbol>> Declaration();
  // Type *TypeName();                    // 6.7.7         // in cc
  uint32_t TryStorageClassSpecifier(); // in cc
  std::unique_ptr<Type> TryTypeSpecifier(uint32_t, uint32_t &, uint32_t,
                                         uint32_t);
  uint32_t TryTypeQualifier();
  uint32_t TryFunctionSpecifier();
  uint32_t TryAlignmentSpecifier();
  std::tuple<Token *, Type *> SpecifierQualifierList(); // 6.7.2.1    // in cc
  Type *AtomicTypeSpecifier(Type *);                    // in cc
  std::tuple<Token *, Type *> StructOrUnionSpecifier(); // in cc
  void StructDeclarationList(Type *);                   // in cc
  void StructDeclaration(Type *);                       // in cc
  void StructDeclaratorList(Type *, StructUnionType *); // in cc
  std::tuple<Token *, Type *> StructDeclarator(Type *); // in cc
  Type *EnumSpecifier(Type *);                          // in cc
  std::unique_ptr<Symbol> Declarator(const std::unique_ptr<Type> &);    // in cc
  std::unique_ptr<Symbol> DirectDeclarator(std::unique_ptr<Type> &);    // in cc
  std::unique_ptr<Type> DirectDeclaratorPrime(std::unique_ptr<Type> &); // in cc
  std::unique_ptr<Type> Pointer(const std::unique_ptr<Type> &);         // in cc
  void TypeQualifierList(std::unique_ptr<Type> &);                      // in cc
  // Type *StaticAssertDeclaration(Type *);                                // in
  // cc Type *EnumeratorList(Type *);                                         //
  // in cc Type *Enumerator(Type *); // in cc Type *EnumerationConstant(Type *);
  // // in cc
  void ParameterTypeList(std::unique_ptr<FunctionType> &); // in cc
  std::vector<std::unique_ptr<Symbol>> ParameterList();    // in cc
  std::unique_ptr<Symbol> ParameterDeclaration();          // in cc
  std::unique_ptr<Symbol>
  AbstractDeclarator(const std::unique_ptr<Type> &); // in cc
  std::unique_ptr<Symbol>
  DirectAbstractDeclarator(std::unique_ptr<Type> &); // in cc
  std::unique_ptr<Type> DeclarationSpecifier();
  //  void TypedefName();

  // Statements
  std::unique_ptr<Stmt> Statement();
  std::unique_ptr<LabeledStmt> LabeledStatement();
  std::unique_ptr<CompoundStmt> CompoundStatement();
  std::pair<bool, std::unique_ptr<ExpressionStmt>> ExpressionStatement();
  std::unique_ptr<SelectionStmt> SelectionStatement();
  std::unique_ptr<IterationStmt> IterationStatement();
  std::unique_ptr<JumpStmt> JumpStatement();
  std::pair<bool, std::vector<std::unique_ptr<Stmt>>> BlockItemList();

  // External Definitions
  bool TranslationUnit();
  bool ExternalDeclaration();
  bool FunctionDeclaration();
  void DeclarationList();

private:
  bool PostfixExprPrime(std::unique_ptr<Expr> &);
  bool ArraySubscripting(std::unique_ptr<Expr> &);
  bool FunctionCall(std::unique_ptr<Expr> &);
  bool MemberReference(std::unique_ptr<Expr> &);
  bool PostfixIncrement(std::unique_ptr<Expr> &);
  bool PostfixDecrement(std::unique_ptr<Expr> &);
  std::vector<std::unique_ptr<Expr>> ArgumentExpressionList();
  //  Expr *CompoundLiterals(Expr *);
  //  Expr *UnaryExpr(Expr *);
  //  Token *UnaryOperator();
  std::unique_ptr<Expr> PrefixIncrement();
  std::unique_ptr<Expr> PrefixDecrement();
  std::unique_ptr<Expr> Sizeof();
  bool AdditiveExprPrime(std::unique_ptr<Expr> &operand1);
  bool ShiftExprPrime(std::unique_ptr<Expr> &operand1);
  bool RelationalExprPrime(std::unique_ptr<Expr> &operand1);
  bool EqualityExprPrime(std::unique_ptr<Expr> &operand1);
  bool ANDExprPrime(std::unique_ptr<Expr> &operand1);
  bool XORExprPrime(std::unique_ptr<Expr> &operand1);
  bool ORExprPrime(std::unique_ptr<Expr> &operand1);
  bool LogicalANDExprPrime(std::unique_ptr<Expr> &operand1);
  bool LogicalORExprPrime(std::unique_ptr<Expr> &operand1);

  bool MultiplicativeExprPrime(std::unique_ptr<Expr> &);
  // Declarations
  void StructDeclarationListPrime(Type *);                   // in cc
  void StructDeclaratorListPrime(Type *, StructUnionType *); // in cc
  std::unique_ptr<ArrayType>
  ArrayDeclarator(std::unique_ptr<Type> &); // 6.7.6.2      // in cc
  long long ArrayDeclaratorInBracket();           // in cc
  std::unique_ptr<FunctionType>
  FunctionDeclarator(std::unique_ptr<Type> &); // 6.7.6.3      // in cc
  void FunctionDeclaratorInParanthesis(std::unique_ptr<FunctionType> &);
  void DirectAbstractDeclaratorPrime(std::unique_ptr<Type> &);

  std::unique_ptr<Symbol> GeneralDeclarator(const std::unique_ptr<Type> &);
  std::unique_ptr<Symbol> GeneralDirectDeclarator(std::unique_ptr<Type> &);
  std::unique_ptr<Type> GeneralDirectDeclaratorPrime(std::unique_ptr<Type> &);

private:
  std::unique_ptr<Lexer> _lexer;
  std::shared_ptr<Scope> _root_scope;
  std::weak_ptr<Scope> _current_scope;

private:
  std::set<TOKEN> scs{TOKEN::TYPEDEF,      TOKEN::EXTERN, TOKEN::STATIC,
                      TOKEN::THREAD_LOCAL, TOKEN::AUTO,   TOKEN::REGISTER};
  std::set<TOKEN> ts{
      TOKEN::VOID,     TOKEN::CHAR,  TOKEN::SHORT,   TOKEN::INT,
      TOKEN::LONG,     TOKEN::FLOAT, TOKEN::DOUBLE,  TOKEN::SIGNED,
      TOKEN::UNSIGNED, TOKEN::BOOL,  TOKEN::COMPLEX, TOKEN::ATOMIC,
      TOKEN::STRUCT,   TOKEN::UNION, TOKEN::ENUM,    TOKEN::TYPEDEF};
  std::set<TOKEN> sus{TOKEN::STRUCT, TOKEN::UNION};
  std::set<TOKEN> tq{TOKEN::CONST, TOKEN::RESTRICT, TOKEN::VOLATILE,
                     TOKEN::ATOMIC};

  std::set<TOKEN> fs{TOKEN::INLINE, TOKEN::NORETURN};

  std::set<TOKEN> as{TOKEN::ALIGNAS};
  bool InFirstSetOfDeclarationSpecifier(TOKEN tag) {
    if (scs.find(tag) != scs.end()) {
      return true;
    }
    if (ts.find(tag) != ts.end()) {
      return true;
    }
    if (tq.find(tag) != tq.end()) {
      return true;
    }
    if (fs.find(tag) != fs.end()) {
      return true;
    }
    if (as.find(tag) != as.end()) {
      return true;
    }
    return false;
  }
};

#endif
