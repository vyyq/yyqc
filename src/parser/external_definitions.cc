#include "parser.h"
#include <cassert>
#include <iostream>

// External Definitions

/**
 *  translation-unit  ->
 *      external-declaration
 *      translation-unit external-declaration
 */
bool Parser::TranslationUnit() {
  bool scan = true;
  while (scan && _lexer->PeekCurrentToken()->tag() != TOKEN::FILE_EOF) {
    scan = ExternalDeclaration();
  }
  return scan || PeekToken()->tag() == TOKEN::FILE_EOF;
}

/**
 *  external-declaration  ->
 *      function-definition
 *      declaration
 */
bool Parser::ExternalDeclaration() {
  auto declarations = Declaration();
  if (declarations.size() >= 1) {
#ifdef DEBUG
      print_line();
      auto &last_element = declarations.back();
      std::cout << "Symbol added: " << *last_element << std::endl;
      std::cout << *(last_element->type());
      print_line();
#endif // DEBUG
    _current_scope.lock()->AddSymbols(declarations);
    return true;
  }
  if (FunctionDeclaration()) {
    return true;
  }
  return false;
}

/**
 * function-declaration ->
 *      declaration-specifier declarator
 *          declaration-list_{opt} compound-statement
 */
bool Parser::FunctionDeclaration() {
  auto snapshot = LexerSnapShot();
#ifdef DEBUG
  std::cout << ">>> Function Declaration" << std::endl;
  std::cout << ">>> Declaration Specifier" << std::endl;
#endif
  auto type_base = DeclarationSpecifier();
  if (!type_base) {
    LexerPutBack(snapshot);
    return false;
  }
#ifdef DEBUG
  std::cout << "<<< Declaration Specifier" << std::endl;
  std::cout << ">>> Declarator" << std::endl;
#endif
  auto delegator = Declarator(type_base);
  if (!delegator) {
    LexerPutBack(snapshot);
    return false;
  }
#ifdef DEBUG
  std::cout << "<<< Declarator" << std::endl;
  std::cout << ">>> Compound Statement" << std::endl;
#endif
  /* TODO: declaration_list_{opt} */
  auto compound_statement = CompoundStatement();
  if (!compound_statement) {
    LexerPutBack(snapshot);
    return false;
  }
  ((FunctionType *)((delegator->type()).get()))->set_compound_stmt(compound_statement);
  // _current_scope.lock()->AddSymbol(delegator);
#ifdef DEBUG
  std::cout << "<<< CompoundStatement" << std::endl;
      print_line();
      std::cout << "Symbol added: " << *delegator << std::endl;
      std::cout << *(delegator->type());
      print_line();
#endif // DEBUG
  return true;
}

/**
 *  declaration-list  ->
 *      declaration
 *      declaration-list declaration
 */
void Parser::DeclarationList() {}
