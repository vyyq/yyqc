#include "../type/type_derived.h"
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
 * declarator  ->  pointer_{opt} direct-declarator
 * Here type base should not be a unique_ptr with ownership since this function
 * just uses it to construct type for some symbols.
 *
 * type_base is only used for constructing new type. For example,
 * int a, b, *c;
 * When a, b and c are all constructed, type_base will be deconstructed.
 */
std::unique_ptr<Symbol>
Parser::Declarator(const std::unique_ptr<Type> &type_base) {
  auto token = PeekToken();
  auto tag = token->tag();
  std::unique_ptr<Type> cloned_type_base = type_base->clone();
  if (tag == TOKEN::STAR) {
    cloned_type_base = Pointer(cloned_type_base);
  }
  return DirectDeclarator(cloned_type_base);
}

/**
 * direct-declarator ->
 * identifier
 * ( declarator )
 * direct-declarator [ type-qualifier-list_{opt} assignment-expression_{opt} ]
 * direct-declarator [ static type-qualifier-list_{opt} assignment-expression ]
 * direct-declarator [ type-qualifier-list static assignment-expression ]
 * direct-declarator [ type-qualifier-list_{opt} * ]
 * direct-declarator ( parameter-type-list )
 * direct-declarator ( identifier-list_{opt} )
 *
 *  which is left-recursive and is needed to be rewritten as:
 *
 * direct-declarator   ->
 *                          identifier direct-declarator'
 *                          ( declarator ) direct-declarator'
 * direct-declarator'  ->
 * [ type-qualifier-list_{opt} assignment-expression_{opt} ]direct-declarator'
 * [ static type-qualifier-list_{opt} assignment-expression ] direct-declarator'
 * [ type-qualifier-list static assignment-expression ] direct-declarator'
 * [ type-qualifier-list_{opt} * ] direct-declarator'
 * ( parameter-type-list ) direct-declarator'
 * ( identifier-list_{opt} ) direct-declarator'
 * ${epsilon}
 */
std::unique_ptr<Symbol>
Parser::DirectDeclarator(std::unique_ptr<Type> &cloned_type_base) {
  auto token = PeekToken();
  if (token->tag() == TOKEN::IDENTIFIER) {
    /**
     * If, in the declaration "T D1", D1 has the form identifier then the
     type
    * specified for ident is T .
    */
    auto identifier_token = Match(TOKEN::IDENTIFIER);
#ifdef DEBUG
    std::cout << "See an Identifier. Its name is [ "
              << *identifier_token->value() << " ]." << std::endl;
#endif // DEBUG
    auto type = DirectDeclaratorPrime(cloned_type_base);
    auto symbol = std::make_unique<Symbol>(identifier_token, type);
    return symbol;
  } else if (token->tag() == TOKEN::LPAR) {
    /**
     * If, in the declaration "T D1", D1 has the form (D) then ident has the
     * type specified by the declaration "T D". Thus, a declarator in
     * parentheses is identical to the unparenthesized declarator, but the
     * binding of complicated declarators may be altered by parentheses.
     */
    Match(TOKEN::LPAR);
    auto symbol = Declarator(cloned_type_base);
    Match(TOKEN::RPAR);
    auto new_type = DirectDeclaratorPrime(symbol->type());
    symbol->set_type(new_type);
    return symbol;
  } else {
    return nullptr;
  }
}

std::unique_ptr<Type>
Parser::DirectDeclaratorPrime(std::unique_ptr<Type> &cloned_type_base) {
  auto token = PeekToken();
  std::unique_ptr<Type> derived_type = nullptr;
  if (token->tag() == TOKEN::LSQUBRKT) {
    derived_type = ArrayDeclarator(cloned_type_base);
  } else if (token->tag() == TOKEN::LPAR) {
    derived_type = FunctionDeclarator(cloned_type_base);
  } else {
    return std::move(cloned_type_base);
  }
  // TODO: DirectDeclaratorPrime(cloned_type_base);
  return derived_type;
}

/**
 * [ type-qualifier-list_{opt} assignment-expression_{opt} ]
 * [ static type-qualifier-list_{opt} assignment-expression ]
 * [ type-qualifier-list static assignment-expression ]
 * [ type-qualifier-list_{opt} * ]
 */
std::unique_ptr<ArrayType>
Parser::ArrayDeclarator(std::unique_ptr<Type> &cloned_array_base) {
  assert(cloned_array_base != nullptr);
  Match(TOKEN::LSQUBRKT);
  int array_length = ArrayDeclaratorInBracket();
  auto array_type =
      std::make_unique<ArrayType>(cloned_array_base, array_length);
  Match(TOKEN::RSQUBRKT);
  return array_type;
}

// VLA support?
/**
 * In C99, the length parameter must come before the variable-length array
 * parameter in function calls.
 * Linus Torvalds has expressed his displeasure in the past over VLA usage for
 * arrays with predetermined small sizes, with comments like "USING VLA'S IS
 * ACTIVELY STUPID! It generates much more code, and much slower code (and more
 * fragile code), than just using a fixed key size would have done." With the
 * Linux 4.20 kernel, Linux kernel is effectively VLA-free.
 * The GNU C Compiler allocates memory for VLAs with automatic storage duration
 * on the stack. VLAs, like all objects in C, are limited to SIZE_MAX bytes
 * VLAs can also be allocated on the heap and accessed using a pointer to VLA.
 *
 * Maybe it'll be supported later.
 */
int Parser::ArrayDeclaratorInBracket() {
  auto token = PeekToken();
  if (PeekToken()->tag() == TOKEN::INTEGER_CONTANT) {
    // int value = token->value()->
    Match(TOKEN::INTEGER_CONTANT);
    // return std::stoi(token->value());
    return -1;
  } else if (PeekToken()->tag() == TOKEN::RSQUBRKT) {
    return -1; // The length has not been determined.
  } else {
    return -1;
  }
}

/**
 * ( parameter-type-list )
 * ( identifier-list_{opt} )
 */
std::unique_ptr<FunctionType>
Parser::FunctionDeclarator(std::unique_ptr<Type> &cloned_function_base) {
#ifdef DEBUG
  std::cout << ">>> Function Declarator" << std::endl;
#endif // DEBUG
  Match(TOKEN::LPAR);
  auto function_type = std::make_unique<FunctionType>(cloned_function_base);
  if (PeekToken()->tag() != TOKEN::RPAR) {
    FunctionDeclaratorInParanthesis(function_type);
  }
  Match(TOKEN::RPAR);
#ifdef DEBUG
  std::cout << "<<< Function Declarator" << std::endl;
#endif // DEBUG
  return function_type;
}

void Parser::FunctionDeclaratorInParanthesis(
    std::unique_ptr<FunctionType> &function_type) {
  ParameterTypeList(function_type);
}

/**
 *  abstract-declarator ->
 *                          pointer
 *                          pointer direct-abstract-declarator
 *                          direct-abstract-declarator
 *  Compared to declarator, besides the "abstract", it can ends with pointer.
 */
std::unique_ptr<Symbol>
Parser::AbstractDeclarator(const std::unique_ptr<Type> &type_base) {
  auto token = PeekToken();
  auto tag = token->tag();
  std::unique_ptr<Type> cloned_type_base = type_base->clone();
  if (tag == TOKEN::STAR) {
    cloned_type_base = Pointer(cloned_type_base);
    auto symbol = DirectAbstractDeclarator(cloned_type_base);
    return symbol;
  } else if (tag == TOKEN::LPAR || tag == TOKEN::LSQUBRKT) {
    auto symbol = DirectAbstractDeclarator(cloned_type_base);
    return symbol;
  } else {
    // abstract-declarator starts with (, [ or *.
    return nullptr;
  }
}

/**
 * direct-abstract-declarator  ->
 * ( abstract-declarator )
 * direct-abstract-declarator_{opt} [ type-qualifier-list_{opt}
 *                                    assignment-expression_{opt} ]
 * direct-abstract-declarator_{opt} [ static type-qualifier-list_{opt}
 *                                    assignment-expression ]
 * direct-abstract-declarator_{opt} [ type-qualifier-list static
 *                                    assignment-expression ]
 * direct-abstract-declarator_{opt} [ * ]
 * direct-abstract-declarator_{opt} ( parameter-type-list_{opt} )
 *
 * It is actually a left recursion and can be rewritten into:
 *
 * direct-abstract-declarator ->
 * ( abstract-declarator ) direct-abstract-declarator'
 * [ type-qualifier-list_{opt} assignment-expression_{opt} ]
 *                                    direct-abstract-declarator'
 * [ static type-qualifier-list_{opt} assignment-expression ]
 *                                    direct-abstract-declarator'
 * [ type-qualifier-list static assignment-expression ]
 *                      declarator'
 * ( parameter-type-list_{opt} )      derect-abstract-declarator'
 *
 * direct-abstract-declarator'  ->
 * [ type-qualifier-list_{opt} assignment-expression_{opt} ]
 *                                              direct-abstract-declarator'
 * [ * ] direct-abstract-    direct-abstract-declarator'
 * [ static type-qualifier-list_{opt} assignment-expression ]
 *                                    direct-abstract-declarator'
 * [ type-qualifier-list static assignment-expression ]
 *                                    direct-abstract-declarator'
 * [ * ] direct-abstract-declarator'
 * ( parameter-type-list_{opt} )      derect-abstract-declarator'
 * ${epsilon}
 *
 */
std::unique_ptr<Symbol>
Parser::DirectAbstractDeclarator(std::unique_ptr<Type> &cloned_type_base) {
  auto token = PeekToken();
  auto tag = token->tag();
  if (tag == TOKEN::LPAR || tag == TOKEN::LSQUBRKT) {
    DirectAbstractDeclarator(cloned_type_base);
    return std::make_unique<Symbol>(nullptr, cloned_type_base);
  } else {
    return std::make_unique<Symbol>(nullptr, cloned_type_base);
  }
}

/**
 * direct-abstract-declarator'  ->
 * [ type-qualifier-list_{opt} assignment-expression_{opt} ]
 *                                              direct-abstract-declarator'
 * [ * ] direct-abstract-    direct-abstract-declarator'
 * [ static type-qualifier-list_{opt} assignment-expression ]
 *                                    direct-abstract-declarator'
 * [ type-qualifier-list static assignment-expression ]
 *                                    direct-abstract-declarator'
 * [ * ] direct-abstract-declarator'
 * ( parameter-type-list_{opt} )      derect-abstract-declarator'
 * ${epsilon}
 */
void Parser::DirectAbstractDeclaratorPrime(
    std::unique_ptr<Type> &cloned_type_base) {
  auto token = PeekToken();
  if (token->tag() == TOKEN::LSQUBRKT) {
    Match(TOKEN::LSQUBRKT);
    cloned_type_base = ArrayDeclarator(cloned_type_base);
    DirectAbstractDeclaratorPrime(cloned_type_base);
  } else if (token->tag() == TOKEN::LPAR) {
    Match(TOKEN::LPAR);
    token = PeekToken();
    if (token->tag() == TOKEN::RPAR) {
      Match(TOKEN::RPAR);
      // return DirectAbstractDeclaratorPrime(type_base);
      cloned_type_base = std::make_unique<FunctionType>(cloned_type_base);
    } else {
      // This is the case of parameter-type-list
      // bool is_variadic = false;
      // TODO
      return;
    }
  }
}

std::unique_ptr<Symbol>
Parser::GeneralDeclarator(const std::unique_ptr<Type> &type_base) {
  auto token = PeekToken();
  auto tag = token->tag();
  std::unique_ptr<Type> cloned_type_base = type_base->clone();
  if (tag == TOKEN::STAR) {
    cloned_type_base = Pointer(cloned_type_base);
  }
  return GeneralDirectDeclarator(cloned_type_base);
}

std::unique_ptr<Symbol>
Parser::GeneralDirectDeclarator(std::unique_ptr<Type> &cloned_type_base) {
  auto token = PeekToken();
  if (token->tag() == TOKEN::IDENTIFIER) {
    auto identifier_token = Match(TOKEN::IDENTIFIER);
#ifdef DEBUG
    std::cout << "See an Identifier. Its name is [ "
              << *identifier_token->value() << " ]." << std::endl;
#endif // DEBUG
    auto type = GeneralDirectDeclaratorPrime(cloned_type_base);
    auto symbol = std::make_unique<Symbol>(identifier_token, type);
    return symbol;
  } else if (token->tag() == TOKEN::LPAR) {
    Match(TOKEN::LPAR);
    auto symbol = GeneralDeclarator(cloned_type_base);
    Match(TOKEN::RPAR);
    auto new_type = GeneralDirectDeclaratorPrime(symbol->type());
    symbol->set_type(new_type);
    return symbol;
  } else {
    return nullptr;
  }
}

std::unique_ptr<Type>
Parser::GeneralDirectDeclaratorPrime(std::unique_ptr<Type> &cloned_type_base) {
  auto token = PeekToken();
  std::unique_ptr<Type> derived_type = nullptr;
  if (token->tag() == TOKEN::LSQUBRKT) {
    derived_type = ArrayDeclarator(cloned_type_base);
  } else if (token->tag() == TOKEN::LPAR) {
    derived_type = FunctionDeclarator(cloned_type_base);
  } else {
    return std::move(cloned_type_base);
  }
  return derived_type;
}

/**
 *  struct-declarator-list  ->
 *                              struct-declarator
 *                              struct-declarator-list , struct-declarator
 *
 *  this left recursion can be rewritten as:
 *
 *  struct-declarator-list  ->
 *                              struct-declarator struct-declarator-list'
 *  struct-declarator-list' ->
 *                              , struct-declarator struct-declarator-list'
 *                              ${epsilon}
 */
void Parser::StructDeclaratorList(Type *declarator_base, StructUnionType *su) {
  (void)declarator_base;
  (void)su;
}

void Parser::StructDeclaratorListPrime(Type *declarator_base,
                                       StructUnionType *su) {
  (void)declarator_base;
  (void)su;
}

/**
 *  struct-declarator   ->
 *                          declarator
 *                          declarator_{opt} : constant-expression
 */
std::tuple<Token *, Type *> Parser::StructDeclarator(Type *type) {
  (void)type;
  return std::make_tuple(nullptr, nullptr);
}
