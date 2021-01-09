#include "parser.h"
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
  return scan;
}

/**
 *  external-declaration  ->
 *      function-definition
 *      declaration
 */
bool Parser::ExternalDeclaration() {
  if (TryDeclaration()) {
    return true;
  }
  if (TryFunctionDeclaration()) {
    return true;
  }
  return false;
}

/**
 * function-declaration ->
 *      declaration-specifier declarator
 *          declaration-list_{opt} compound-statement
 */
bool Parser::TryFunctionDeclaration() {
  auto type_base = DeclarationSpecifier();
  auto delegator = Declarator(type_base);
  /* TODO: declaration_list_{opt} */
  auto compound_statement = CompoundStatement();
  (void)compound_statement;

  return false;
}

/**
 *  declaration-list  ->
 *      declaration
 *      declaration-list declaration
 */
void Parser::DeclarationList() {}
