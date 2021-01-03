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
#include <set>
#include <string>
#include <tuple>

#define DEBUG

bool IsSpecifier(TOKEN tag);
const char stoc(const std::string &);

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
    std::cout << "Match: " << Token::tag_to_string[tag] << std::endl;
#endif // DEBUG

    assert(PeekToken(tag));
    return ConsumeToken();
  }
  unsigned LexerScreenShot() { return _lexer->ScreenShot(); }
  void LexerPutBack(unsigned screenshot) { return _lexer->PutBack(screenshot); }
  std::shared_ptr<Scope> CurrentScope() { return _current_scope; }
  bool isRootScope() { return _current_scope->parent() == nullptr; }

public:
  void EnterNewSubScope() {
    auto &curr = _current_scope->AddSubScope();
    _current_scope = curr;
  }
  void ExitCurrentSubScope() {}
  explicit Parser(const std::string &filename)
      : _lexer(std::make_unique<Lexer>(filename)),
        _root_scope(std::make_shared<Scope>()), _current_scope(_root_scope) {}
  ~Parser() = default;
  bool Scan() { return TranslationUnit(); }

  // Expressions
  // Expr *Expression();
  // std::unique_ptr<Expr> PrimaryExpr();
  // Constant *EnumerationConstant();
  // Constant *CharacterConstant();
  // StringLiteral *StrLiteral();
  // Expr *PostfixExpr();
  // Expr *UnaryExpr();
  // Expr *Sizeof();
  // Expr *Alignof();
  // Expr *CastExpr();
  // Expr *MultiplicativeExpr();
  // Expr *AdditiveExpr();
  // Expr *ShiftExpr();
  // Expr *RelationalExpr();
  // Expr *EqualityExpr();
  // Expr *ANDExpr();
  // Expr *XORExpr();
  // Expr *ORExpr();
  // Expr *LogicalANDExpr();
  // Expr *LogicalORExpr();
  // Expr *ConditionalExpr();
  // Expr *AssignmentExpr();
  // Expr *ConstantExpr();

  // Declarators
  bool TryDeclaration();
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
  // Type *StaticAssertDeclaration(Type *);                                // in cc
  // Type *EnumeratorList(Type *);                                         // in cc
  // Type *Enumerator(Type *);                                             // in cc
  // Type *EnumerationConstant(Type *);                                    // in cc
  void ParameterTypeList(std::unique_ptr<FunctionType> &);              // in cc
  std::vector<std::unique_ptr<Symbol>> ParameterList();                 // in cc
  std::unique_ptr<Symbol> ParameterDeclaration();                       // in cc
  std::unique_ptr<Symbol>
  AbstractDeclarator(const std::unique_ptr<Type> &); // in cc
  std::unique_ptr<Symbol>
  DirectAbstractDeclarator(std::unique_ptr<Type> &); // in cc
  std::unique_ptr<Type> DeclarationSpecifier();
  //  void TypedefName();

  // Statements
  Stmt *Statement();
  LabeledStmt *LabeledStatement();
  CompoundStmt *CompoundStatement();
  ExpressionStmt *ExpressionStatement();
  SelectionStmt *SelectionStatement();
  IterationStmt *IterationStatement();
  JumpStmt *JumpStatement();
  std::list<ASTNode *> BlockItemList();
  ASTNode *BlockItem();

  // External Definitions
  bool TranslationUnit();
  bool ExternalDeclaration();
  bool TryFunctionDeclaration();
  void DeclarationList();

private:
  //  Expr *PostfixExprPrime(Expr *);
  //  Expr *ArraySubscripting(Expr *);
  //  Expr *FunctionCall(Expr *);
  //  Expr *MemberReference(Expr *);
  //  Expr *PostfixIncrement(Expr *);
  //  Expr *PostfixDecrement(Expr *);
  //  Expr *CompoundLiterals(Expr *);
  //  Expr *UnaryExpr(Expr *);
  //  Token *UnaryOperator();
  //  Token *PrefixIncrement();
  //  Token *PrefixDecrement();
  //  Expr *MultiplicativeExprPrime(Expr *operand1);
  //  Expr *AdditiveExprPrime(Expr *operand1);
  //  Expr *ShiftExprPrime(Expr *operand1);
  //  Expr *RelationalExprPrime(Expr *operand1);
  //  Expr *EqualityExprPrime(Expr *operand1);
  //  Expr *ANDExprPrime(Expr *operand1);
  //  Expr *XORExprPrime(Expr *operand1);
  //  Expr *ORExsprPrime(Expr *operand1);
  //  Expr *LogicalANDExprPrime(Expr *operand1);
  //  Expr *LogicalORExprPrime(Expr *operand1);

  // Declarations
  void StructDeclarationListPrime(Type *);                   // in cc
  void StructDeclaratorListPrime(Type *, StructUnionType *); // in cc
  std::unique_ptr<ArrayType>
  ArrayDeclarator(std::unique_ptr<Type> &); // 6.7.6.2      // in cc
  int ArrayDeclaratorInBracket();           // in cc
  std::unique_ptr<FunctionType>
  FunctionDeclarator(std::unique_ptr<Type> &); // 6.7.6.3      // in cc
  void FunctionDeclaratorInParanthesis(std::unique_ptr<FunctionType> &);
  void DirectAbstractDeclaratorPrime(std::unique_ptr<Type> &);

  std::unique_ptr<Symbol> GeneralDeclarator(const std::unique_ptr<Type> &);
  std::unique_ptr<Symbol> GeneralDirectDeclarator(std::unique_ptr<Type> &);
  std::unique_ptr<Type> GeneralDirectDeclaratorPrime(std::unique_ptr<Type> &);

private:
  // Lexer *_lexer;
  std::unique_ptr<Lexer> _lexer;
  // Scope *_scope;
  std::shared_ptr<Scope> _root_scope;
  std::shared_ptr<Scope> _current_scope;

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