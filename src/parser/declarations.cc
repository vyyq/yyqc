#include "../type/type_derived.h"
#include "parser.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <list>
#include <sstream>
#include <string>

// 6.7.7
// type-name -> specifier-qualifier-list abstract-declarator_{opt}
// Type *Parser::TypeName() {
//   // Token *type_token = nullptr;
//   Type *type_base = nullptr;
//   // std::tie(type_token, type_base) = SpecifierQualifierList();
//   // auto tag = PeekToken()->tag();
//   // if (tag == TOKEN::STAR || tag == TOKEN::LPAR || tag == TOKEN::LSQUBRKT)
//   {
//   //   type_base = AbstractDeclarator(type_base);
//   // }
//   // return type_base;
//   return nullptr;
// }

std::vector<std::unique_ptr<Symbol>> Parser::Declaration() {
  std::vector<std::unique_ptr<Symbol>> declarations;
  auto snapshot = LexerSnapShot();
  std::unique_ptr<Type> type_base = DeclarationSpecifier();
  if (type_base == nullptr) {
    LexerPutBack(snapshot);
    return {};
  } else {
    bool first_declarator = true;
    do {
      if (first_declarator) {
        first_declarator = false;
      } else {
        Match(TOKEN::COMMA);
      }
      auto declarator = Declarator(type_base);
      /* TODO: assign value in declarator. */
#ifdef DEBUG
      std::cout << "Prepare to add declarator to vector." << std::endl;
#endif
      std::unique_ptr<Symbol> symbol = std::move(declarator);
      declarations.push_back(std::move(symbol));
#ifdef DEBUG
      print_line();
      auto &last_element = declarations.back();
      std::cout << "Symbol added: " << *last_element << std::endl;
      std::cout << *(last_element->type());
      print_line();
#endif // DEBUG

      // _current_scope->AddSymbol(symbol);
    } while (PeekToken()->tag() == TOKEN::COMMA);
    if (PeekToken()->tag() == TOKEN::SEMI) {
      Match(TOKEN::SEMI);
      return declarations;
    } else {
      LexerPutBack(snapshot);
      return {};
    }
  }
}

/**
 *  declaration-specifiers  ->
 *        storage-class-specifier declaration-specifiers_{opt}
 *        type-specifier declaration-specifiers_{opt}
 *        type-qualifier declaration-specifiers_{opt}
 *        function-specifier declaration-specifiers_{opt}
 *        alignment-specifier declaration-specifiers_{opt}
 */
std::unique_ptr<Type> Parser::DeclarationSpecifier() {
  uint32_t storage_class_specifier_flag = 0;
  uint32_t type_specifier_flag = 0;
  uint32_t type_qualifier_flag = 0;
  uint32_t function_specifier_flag = 0;
  std::unique_ptr<Type> type = nullptr;
  while (true) {
    uint32_t temp_flag = 0x0;
    std::unique_ptr<Type> temp_type = nullptr;
    temp_flag = TryStorageClassSpecifier();
    if (temp_flag != 0) {
      storage_class_specifier_flag |= temp_flag;
      continue;
    }

    temp_type =
        TryTypeSpecifier(storage_class_specifier_flag, type_specifier_flag,
                         type_qualifier_flag, function_specifier_flag);
    if (temp_type != nullptr) {
      assert(type == nullptr);
      type = std::move(temp_type);
      type_specifier_flag |= temp_flag;
      continue;
    }

    temp_flag = TryTypeQualifier();
    if (temp_flag != 0) {
      type_qualifier_flag |= temp_flag;
      continue;
    }

    temp_flag = TryFunctionSpecifier();
    if (temp_flag != 0) {
      function_specifier_flag |= temp_flag;
      continue;
    }

    // TODO: TryAlignmentSpecifier();

    // None of them matches, break.
    break;
  }
  return type;
}

// Try to match storage-class-specifier. If succeeds, match, else pass.
uint32_t Parser::TryStorageClassSpecifier() {
  uint32_t flag = 0;
  auto tag = PeekToken()->tag();
  switch (tag) {
  case TOKEN::TYPEDEF:
    Match(TOKEN::TYPEDEF);
    flag |= SCS_TYPEDEF;
    break;
  case TOKEN::EXTERN:
    Match(TOKEN::EXTERN);
    flag |= SCS_EXTERN;
    break;
  case TOKEN::STATIC:
    Match(TOKEN::STATIC);
    flag |= SCS_STATIC;
    break;
  case TOKEN::THREAD_LOCAL:
    Match(TOKEN::THREAD_LOCAL);
    flag |= SCS_THREAD_LOCAL;
    break;
  case TOKEN::AUTO:
    Match(TOKEN::AUTO);
    flag |= SCS_AUTO;
    break;
  case TOKEN::REGISTER:
    Match(TOKEN::REGISTER);
    flag |= SCS_REGISTER;
    break;
  default:
    break;
  }
  return flag;
}

std::unique_ptr<Type> Parser::TryTypeSpecifier(
    uint32_t storage_class_specifier_flag, uint32_t &type_specifier_flag,
    uint32_t type_qualifier_flag, uint32_t function_specifier_flag) {
  std::unique_ptr<Type> type = nullptr;
  auto token = PeekToken();
  auto tag = token->tag();
  switch (tag) {
  case TOKEN::VOID:
    Match(TOKEN::VOID);
    type_specifier_flag |= TS_VOID;
    type = std::make_unique<VoidType>();
    break;
  case TOKEN::CHAR:
    Match(TOKEN::CHAR);
    type_specifier_flag |= TS_CHAR;
    type = std::make_unique<CharType>();
    break;
  case TOKEN::SHORT:
    Match(TOKEN::SHORT);
    type_specifier_flag |= TS_SHORT;
    type = std::make_unique<IntType>(storage_class_specifier_flag,
                                     type_specifier_flag, type_qualifier_flag,
                                     function_specifier_flag);
    break;
  case TOKEN::INT:
    Match(TOKEN::INT);
    type_specifier_flag |= TS_INT;
    if (type == nullptr) {
      type = std::make_unique<IntType>(storage_class_specifier_flag,
                                       type_specifier_flag, type_qualifier_flag,
                                       function_specifier_flag);
    }
    break;
  case TOKEN::LONG:
    Match(TOKEN::INT);
    type_specifier_flag |= TS_LONG;
    type = std::make_unique<IntType>(storage_class_specifier_flag,
                                     type_specifier_flag, type_qualifier_flag,
                                     function_specifier_flag);
    break;
  case TOKEN::FLOAT:
    Match(TOKEN::FLOAT);
    type_specifier_flag |= TS_FLOAT;
    type = std::make_unique<FloatType>();
    break;
  case TOKEN::DOUBLE:
    Match(TOKEN::DOUBLE);
    type_specifier_flag |= TS_DOUBLE;
    type = std::make_unique<FloatType>();
    break;
  case TOKEN::SIGNED:
    Match(TOKEN::SIGNED);
    type_specifier_flag |= TS_SIGNED;
    if (type == nullptr) {
      type = std::make_unique<IntType>();
    }
    break;
  case TOKEN::UNSIGNED:
    Match(TOKEN::UNSIGNED);
    type_specifier_flag |= TS_UNSIGNED;
    if (type == nullptr) {
      type = std::make_unique<IntType>(storage_class_specifier_flag,
                                       type_specifier_flag, type_qualifier_flag,
                                       function_specifier_flag);
    }
    break;
  case TOKEN::BOOL:
    Match(TOKEN::BOOL);
    type_specifier_flag |= TS_BOOL;
    type = std::make_unique<BoolType>();
    break;
  case TOKEN::COMPLEX:
    Match(TOKEN::COMPLEX);
    type_specifier_flag |= TS_COMPLEX;
    // TODO: complex
    break;
  case TOKEN::ATOMIC:
  case TOKEN::STRUCT:
  case TOKEN::UNION:
  case TOKEN::ENUM:
  case TOKEN::TYPEDEF:
  default:
    break;
  };
  return type;
}

// Try to match type-specifier. If succeeds, match, else pass.
uint32_t Parser::TryTypeQualifier() {
  uint32_t flag = 0;
  auto tag = PeekToken()->tag();
  switch (tag) {
  case TOKEN::CONST:
    Match(TOKEN::CONST);
    flag |= TQ_CONST;
    break;
  case TOKEN::RESTRICT:
    Match(TOKEN::RESTRICT);
    flag |= TQ_RESTRICT;
    break;
  case TOKEN::VOLATILE:
    Match(TOKEN::VOLATILE);
    flag |= TQ_RESTRICT;
    break;
  case TOKEN::ATOMIC:
    Match(TOKEN::VOLATILE);
    flag |= TQ_ATOMIC;
    break;
  default:
    break;
  }
  return flag;
}

uint32_t Parser::TryFunctionSpecifier() {
  uint32_t flag = 0;
  auto tag = PeekToken()->tag();
  switch (tag) {
  case TOKEN::INLINE:
    Match(TOKEN::INLINE);
    flag |= FS_INLINE;
    break;
  case TOKEN::NORETURN:
    Match(TOKEN::NORETURN);
    flag |= FS_NOTRETURN;
    break;
  default:
    break;
    ;
  }
  return flag;
}

uint32_t Parser::TryAlignmentSpecifier() { return 0; }

std::tuple<Token *, Type *> Parser::SpecifierQualifierList() {
  // auto token = PeekToken();
  // Type *type = nullptr;
  // Token *returned_token = nullptr;
  // while (true) {
  //   switch (token->tag()) {
  //   case TOKEN::VOID:
  //     assert(type == nullptr);
  //     type = new VoidType();
  //     type->AddFlag(TS_VOID);
  //     Match(TOKEN::VOID);
  //     break;
  //   case TOKEN::CHAR:
  //     assert(type == nullptr);
  //     type = new CharType();
  //     type->AddFlag(TS_CHAR);
  //     Match(TOKEN::VOID);
  //     break;
  //   case TOKEN::SHORT:
  //     assert(type == nullptr);
  //     type = new IntType();
  //     type->AddFlag(TS_SHORT);
  //     Match(TOKEN::SHORT);
  //     break;
  //   case TOKEN::INT:
  //     assert(type == nullptr);
  //     type = new IntType();
  //     type->AddFlag(TS_INT);
  //     Match(TOKEN::INT);
  //     break;
  //   case TOKEN::LONG:
  //     assert(type == nullptr);
  //     type = new IntType();
  //     type->AddFlag(TS_LONG);
  //     Match(TOKEN::LONG);
  //     break;
  //   case TOKEN::FLOAT:
  //     assert(type == nullptr);
  //     type = new FloatType();
  //     type->AddFlag(TS_LONG);
  //     Match(TOKEN::FLOAT);
  //     break;
  //   case TOKEN::DOUBLE:
  //     assert(type == nullptr);
  //     type = new FloatType();
  //     type->AddFlag(TS_DOUBLE);
  //     Match(TOKEN::DOUBLE);
  //     break;
  //   case TOKEN::SIGNED:
  //     assert(type == nullptr || type->IsIntType());
  //     if (type == nullptr) {
  //       type = new IntType();
  //     }
  //     type->AddFlag(TS_SIGNED);
  //     Match(TOKEN::SIGNED);
  //     break;
  //   case TOKEN::UNSIGNED:
  //     assert(type == nullptr || type->IsIntType());
  //     if (type == nullptr) {
  //       type = new IntType();
  //     }
  //     type->AddFlag(TS_UNSIGNED);
  //     Match(TOKEN::UNSIGNED);
  //     break;
  //   case TOKEN::BOOL:
  //     assert(type == nullptr);
  //     type = new BoolType();
  //     type->AddFlag(TS_BOOL);
  //     Match(TOKEN::BOOL);
  //     break;
  //   case TOKEN::COMPLEX:
  //     // TODO: How does complex work?
  //     type->AddFlag(TS_COMPLEX);
  //     Match(TOKEN::COMPLEX);
  //     break;
  //   case TOKEN::ATOMIC:
  //     if (PeekNextToken()->tag() == TOKEN::LPAR) {
  //       // type = new AtomicType(); // TODO
  //       type = AtomicTypeSpecifier(type);
  //     } else {
  //       type->AddFlag(TQ_ATOMIC);
  //     }
  //     break;
  //   case TOKEN::STRUCT:
  //   case TOKEN::UNION:
  //     assert(type == nullptr);
  //     if (token->tag() == TOKEN::STRUCT) {
  //       type = new StructType();
  //     } else {
  //       type = new UnionType();
  //     }
  //     std::tie(returned_token, type) = StructOrUnionSpecifier();
  //     break;
  //   case TOKEN::ENUM:
  //     // TODO: Support enum feature;
  //     type = EnumSpecifier(type);
  //     break;
  //   case TOKEN::IDENTIFIER:
  //     TypedefName();
  //     break;
  //   case TOKEN::CONST:
  //     type->AddFlag(TQ_CONST);
  //     Match(TOKEN::CONST);
  //     break;
  //   case TOKEN::RESTRICT:
  //     type->AddFlag(TQ_RESTRICT);
  //     Match(TOKEN::RESTRICT);
  //     break;
  //   case TOKEN::VOLATILE:
  //     type->AddFlag(TQ_VOLATILE);
  //     Match(TOKEN::VOLATILE);
  //     break;
  //   default:
  //     return std::make_tuple(returned_token, type);
  //   }
  // }
  return std::make_tuple(nullptr, nullptr);
}

/**
 * atomic-type-specifier structure:
 *
 *  Type* = nullptr  type-name -> Type*
 *  point-to         -------->    point-to = nullptr
 *  flags1                        flags2
 *
 *  where flags1 must include TS_ATOMIC bit.
 */
Type *Parser::AtomicTypeSpecifier(Type *type) {
  (void)type;
  // Match(TOKEN::ATOMIC);
  // type->AddFlag(TS_ATOMIC);
  // Match(TOKEN::LPAR);
  // auto type_head = TypeName();
  // type->set_base(type_head);
  // Match(TOKEN::RPAR);
  // return type;
  return nullptr;
}

/**
 *  struct-or-union-specifier ->
 *    struct-or-union identifier_{opt} { struct-declaration-list }
 *    struct-or-union identifier
 *
 *  struct-or-union ->
 *    struct
 *    union
 */
std::tuple<Token *, Type *> Parser::StructOrUnionSpecifier() {
  // Type *s_type = nullptr;
  // uint32_t type_specifier_flag = 0x0;
  // auto token = PeekToken();
  // if (token->tag() == TOKEN::STRUCT) {
  //   Match(TOKEN::STRUCT);
  //   // s_type->AddFlag(TS_STRUCT_UNION);
  //   type_specifier_flag |= TS_STRUCT_UNION;
  //   auto type = new StructType();
  // } else if (token->tag() == TOKEN::UNION) {
  //   Match(TOKEN::UNION);
  //   type_specifier_flag |= TS_STRUCT_UNION;
  //   // s_type->AddFlag(TS_STRUCT_UNION);
  //   auto type = new UnionType();
  // } else {
  //   // Handle error.
  //   Error("Error in struct-or-union-specifier, which should start with \" "
  //         "struct \" or \" union \".\n");
  //   return std::make_tuple(nullptr, nullptr);
  // }
  // token = PeekToken();
  // if (token->tag() == TOKEN::IDENTIFIER) { // We have seen an identifier.
  //   auto struct_union_tag = Match(TOKEN::IDENTIFIER);
  //   // Check the existence of this struct identifier in this scope.
  //   Identifier *identifier = nullptr; // TODO: check symbol table of this
  //   scope. if (identifier != nullptr) {      // If existed.
  //     // Check: if the struct/union of this identifier is complete, then it
  //     is
  //     // an error of redefinition.
  //     if (token->tag() == TOKEN::LBRACE) {
  //       if (identifier->type()->completed()) {
  //         // Invalid, since try to redefine an existed completed struct.
  //         Error("Redefinition of an existed struct/union.");
  //         return std::make_tuple(nullptr, nullptr);
  //       } else {
  //         // The struct/union is not completed yet, so it is valid.
  //         // Now it defines the struct and set it to completed.
  //         Match(TOKEN::LBRACE);
  //         StructDeclarationList(s_type);
  //         Match(TOKEN::RBRACE);
  //         identifier->type()->set_completed();
  //         // Return the token of the defining position.
  //         return std::make_tuple(token, s_type);
  //       }
  //     } else {
  //       // Just a declarator.
  //       // Return the token of this declaring position.
  //       return std::make_tuple(token, s_type);
  //     }
  //   } else { // Identifier does not exist.
  //     // Identifier *identifier = new StructUnionEnumTag(struct_union_tag,
  //     s_type);
  //     // TODO: New identifier. Add this identifier to symbol table of this
  //     // scope.
  //     if (token->tag() != TOKEN::LBRACE) {
  //       // Only a declarator.
  //       return std::make_tuple(struct_union_tag, s_type);
  //     } else {
  //       Match(TOKEN::LBRACE);
  //       StructDeclarationList(s_type);
  //       Match(TOKEN::RBRACE);
  //       return std::make_tuple(struct_union_tag, s_type);
  //     }
  //   }
  // } else { // Without an identifier.
  //   // Not seen an identifier, which indicates it is an anonymous
  //   struct/union.
  //   // Thus, { struct-declaration-list } must exist.
  //   token = PeekToken();
  //   if (token->tag() != TOKEN::LBRACE) {
  //     Error("In struct-or-union: anonymous struct/union must have { "
  //           "struct-declaration-list }");
  //     return std::make_tuple(nullptr, nullptr);
  //   } else {
  //     Match(TOKEN::LBRACE);
  //     StructDeclarationList(s_type);
  //     Match(TOKEN::RBRACE);
  //     return std::make_tuple(nullptr, s_type);
  //   }
  // }
  return std::make_tuple(nullptr, nullptr);
}

/**
 *  struct-declaration-list   ->
 *                                struct-declaration
 *                                struct-declaration-list struct-declaration
 *
 *  left recursion should be rewritten as:
 *
 *  struct-declaration-list   ->
 *                                struct-declaration struct-declaration-list'
 *  struct-declaration-list'  ->
 *                                struct-declaration struct-declaration-list'
 *                                ${epsilon}
 */
void Parser::StructDeclarationList(Type *s_type) {
  (void)s_type;
  // StructDeclaration(s_type);
  // StructDeclarationListPrime(s_type);
}

void Parser::StructDeclarationListPrime(Type *s_type) {
  (void)s_type;
  // StructDeclaration(s_type);
  // StructDeclarationListPrime(s_type);
}

/**
 *  struct-declaration  ->
 *          specifier-qualifier-list struct-declarator-list_{opt} ;
 *          static_assert-declaration
 */
void Parser::StructDeclaration(Type *s_type) {
  (void)s_type;
  // auto token = PeekToken();
  // auto tag = token->tag();
  // if (tag == TOKEN::STATIC_ASSERT) {
  //   StaticAssertDeclaration(s_type);
  // } else {
  //   Token *sq_token = nullptr;
  //   Type *base = nullptr;
  //   std::tie(sq_token, base) = SpecifierQualifierList();
  //   if (tag == TOKEN::COLON || tag == TOKEN::STAR || tag == TOKEN::IDENTIFIER
  //   ||
  //       tag == TOKEN::LPAR) {
  //     // Questionable, since not sure about the FIRST set.
  //     // TODO: double check the first set.
  //     StructDeclaratorList(base, s_type->CastToStructureOrUnionType());
  //     return;
  //   } else {
  //     /**
  //      *  The struct-declarator-list is not required if the base type is
  //      *  struct/union .....
  //      *  TODO: Think about what other type can be.
  //      */
  //     assert(base->IsStructOrUnionType());
  //     // s_type->CastToStructureOrUnionType()->AddMemberObject(
  //     //     new StructUnionMember(nullptr, base));
  //   }
  //   return;
  // }
}

/**
 *  pointer   ->
 *                  * type-qualifier-list_{opt}
 *                  * type-qualifier-list_{opt} pointer
 */
std::unique_ptr<Type> Parser::Pointer(const std::unique_ptr<Type> &type_base) {
  auto token = PeekToken();
  // std::unique_ptr<Type> pointer_type = std::make_unique<Type>(type_base);
  auto pointer_type = type_base->clone();
  while (token->tag() == TOKEN::STAR) {
    Match(TOKEN::STAR);
    pointer_type = std::make_unique<PointerType>(pointer_type);
    // TODO: double-check which node is qualified by type-qualifier-list
    TypeQualifierList(pointer_type);
    token = PeekToken();
  }
  return pointer_type;
}

/**
 *  parameter-type-list ->
 *                          parameter-list
 *                          parameter-list , ...
 */
void Parser::ParameterTypeList(std::unique_ptr<FunctionType> &function_type) {
  auto parameter_list = ParameterList();
  function_type->AddParameters(parameter_list);
  if (PeekToken()->tag() == TOKEN::COMMA) {
    Match(TOKEN::COMMA);
    Match(TOKEN::ELLIPSIS);
    function_type->set_variadic(true);
  } else {
    function_type->set_variadic(false);
  }
}

/**
 *  parameter-list  ->
 *                      parameter-declaration
 *                      parameter-list , parameter-declaration
 */
std::vector<std::unique_ptr<Symbol>> Parser::ParameterList() {
  std::vector<std::unique_ptr<Symbol>> parameter_list;
  parameter_list.push_back(ParameterDeclaration());
  while (PeekToken()->tag() == TOKEN::COMMA &&
         PeekNextToken()->tag() != TOKEN::ELLIPSIS) {
    Match(TOKEN::COMMA);
    parameter_list.push_back(ParameterDeclaration());
  }
  return parameter_list;
}

/**
 *  parameter-declaration ->
 *                            declaration-specifier declarator
 *                            declaration-specifier abstract-declarator_{opt}
 *
 */
std::unique_ptr<Symbol> Parser::ParameterDeclaration() {
  std::unique_ptr<Type> type_base = DeclarationSpecifier();
  return GeneralDeclarator(type_base);
}

/**
 *  type-qualifier-list   ->
 *                              type-qualifier
 *                              type-qualifier-list type-qualifier
 */
void Parser::TypeQualifierList(std::unique_ptr<Type> &type) {
  auto token = PeekToken();
  while (true) {
    switch (token->tag()) {
    case TOKEN::CONST:
      Match(TOKEN::CONST);
      type->add_type_qualifier(TQ_CONST);
      break;
    case TOKEN::RESTRICT:
      Match(TOKEN::RESTRICT);
      type->add_type_qualifier(TQ_RESTRICT);
      break;
    case TOKEN::VOLATILE:
      Match(TOKEN::VOLATILE);
      type->add_type_qualifier(TQ_VOLATILE);
      break;
    case TOKEN::ATOMIC:
      Match(TOKEN::ATOMIC);
      type->add_type_qualifier(TQ_ATOMIC);
      break;
    default:
      return;
    }
    token = PeekToken();
  }
}

/**
 *  static_assert-declaration ->
 *          _Static_assert ( constant-expression , string-literal ) ;
 *
 */
// Type *Parser::StaticAssertDeclaration(Type *node) {
//   // auto token = PeekToken();
//   // Match(TOKEN::STATIC_ASSERT);
//   // Match(TOKEN::LPAR);
//   // auto expr = Const();
//   // Match(TOKEN::COMMA);
//   // auto str_literal = StrLiteral();
//   // Match(TOKEN::RPAR);
//   // Match(TOKEN::SEMI);
//   // // TODO: Compose expr and str_literal.
//   // Error("static-assert-declaration is not supported yet.");
//   // return node;
//   return nullptr;
// }

/**
 *  enum-specifier  ->
 *                      enum identifier_{opt} { enumerator-list }
 *                      enum identifier_{opt} { enumerator-list , }
 *                      enum identifier
 */
Type *Parser::EnumSpecifier(Type *node) {
  (void)node;
  // TODO:
  // Match(TOKEN::ENUM);
  // auto token = PeekToken();
  // if (token->tag() == TOKEN::IDENTIFIER) {
  //   token = Match(TOKEN::IDENTIFIER);
  //   // auto identifier = new StructUnionEnumTag(token, )
  // } else {
  //   // anonymous
  // }
  // return node;
  return nullptr;
}

/**
 * enumerator-list  ->
 *                      enumerator
 *                      enumerator-list , enumerator
 *
 * The left-recursion is needed to be rewritten as:
 * enumerator-list  ->
 *                      enumerator enumerator-list'
 * enumerator-list' ->  , enumerator enumerator-list'
 *                      ${epsilon}
 */
// Type *Parser::EnumeratorList(Type *node) {
//   // node = Enumerator(node);
//   // auto token = PeekToken();
//   // while (token->tag() == TOKEN::COMMA) {
//   //   Match(TOKEN::COMMA);
//   //   node = Enumerator(node);
//   // }
//   // return node;
//   return nullptr;
// }

/**
 *  enumerator  ->
 *                  enumeration-constant
 *                  enumeration-constant = constant-expression
 */
// Type *Parser::Enumerator(Type *node) {
//   // node = EnumerationConstant(node);
//   // auto token = PeekToken();
//   // if (token->tag() == TOKEN::ASSIGN) {
//   //   Match(TOKEN::ASSIGN);
//   //   auto constant = Const();
//   //   // TODO: Add this constant to the enumeration-constant.
//   // }
//   return node;
// }

/**
 *  enumeration-constant  ->  identifier
 */
// Type *Parser::EnumerationConstant(Type *node) {
//   // TODO:
//   return node;
// }
