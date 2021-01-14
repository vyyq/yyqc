#ifndef YYQC_SRC_TOKEN_H_
#define YYQC_SRC_TOKEN_H_

#include "../public/position.h"
#include "./value.h"
#include <iostream>
#include <unordered_map>

class Token;
enum class TOKEN;

enum class TOKEN {
  FILE_EOF = 0,
  // ASCII characters
  TAB = '\t',
  NEWLINE = '\n',      // '\n'
  SPACE = ' ',         // ' ' 32
  LOGICAL_NOT = '!',   // '!' 33
  DOUBLE_QUOTE = '\"', // '\"' 34
  SHARP = '#',         // 35
  DOLLAR = '$',        // 36
  MOD = '%',           // 37
  AND = '&',           // 38
  SQUOTE = '\'',       // 39
  LPAR = '(',
  RPAR = ')',
  STAR = '*',
  ADD = '+', // 42: '+'
  COMMA = ',',
  SUB = '-',
  DOT = '.',
  DIV = '/',
  COLON = ':',    // 58
  SEMI = ';',     // 59
  LESS = '<',     // 60
  ASSIGN = '=',   // 61
  GREATER = '>',  // 62
  COND = '?',     // 63
  AT = '@',       // 64
  LSQUBRKT = '[', // 91
  BKSLASH = '\\',
  RSQUBRKT = ']',
  XOR = '^',
  UNDERSCORE = '_',
  BKQUT = '`',
  LBRACE = '{', // 123
  OR = '|',     // 124
  RBRACE = '}', // 125
  NOT = '~',    // 126, bitwise operator

  PTR_MEM_REF,
  ALTER_LBRACE,   // '<%': alternative spelling of {
  ALTER_LSQUBRKT, // '<:': alternative spelling of [

  // relational operators
  LE,
  GE,
  EQ,
  NE,

  // logical operators
  LOGICAL_AND,
  LOGICAL_OR,
  // LOGICAL_NOT = 33

  // shift operators
  LEFT_SHIFT,
  RIGHT_SHIFT,

  //
  INCREMENT,
  DECREMENT,
  ELLIPSIS,

  // assign operators
  ADD_ASSIGN,
  SUB_ASSIGN,
  MUL_ASSIGN,
  DIV_ASSIGN,
  MOD_ASSIGN,
  AND_ASSIGN,
  OR_ASSIGN,
  XOR_ASSIGN,
  NOT_ASSIGN,   // ~=
  LEFT_ASSIGN,  // >>=
  RIGHT_ASSIGN, // >>=
  // ASSIGN = 61

  // KEYWORD
  KEYWORD_START,
  // type
  AUTO,
  CHAR,
  DOUBLE,
  ENUM,
  FLOAT,
  INT,
  LONG,
  SHORT,
  SIGNED,
  STRUCT,
  UNION,
  UNSIGNED,
  VOID,
  BOOL,    // _Bool
  COMPLEX, // _Complex

  // qualifier
  CONST,
  RESTRICT,
  VOLATILE,

  //
  BREAK,
  CASE,
  CONTINUE,
  DEFAULT,
  DO,
  ELSE,
  EXTERN,
  FOR,
  GOTO,
  IF,
  INLINE,
  REGISTER,
  RETURN,
  SIZEOF,
  STATIC,
  SWITCH,
  TYPEDEF,
  WHILE,

  // NOT FAMILIAR KEYWORD
  ALIGNAS,
  ALIGNOF,
  ATOMIC,
  GENERIC,
  IMAGINARY,
  NORETURN,
  STATIC_ASSERT,
  THREAD_LOCAL,

  KEYWORD_END,

  //
  IDENTIFIER,
  STRING_LITERAL,
  CONSTANT_START,
  INTEGER_CONTANT,
  FLOATING_CONSTANT,
  ENUMERATION_CONSTANT,
  CHARACTER_CONSTANT,
  CONSTANT_END
};

class Token {
private:
  TOKEN _tag = TOKEN::FILE_EOF;
  const Position _position;
  std::unique_ptr<Value> _value;

public:
  Token(TOKEN tag, const Position &position) : _tag(tag), _position(position) {}
  Token(TOKEN tag, const Position &position, std::unique_ptr<Value> &value)
      : _tag(tag), _position(position), _value(std::move(value)) {}
  TOKEN tag() { return _tag; }
  std::unique_ptr<Value> &value() { return _value; }
  const Position &position() const { return _position; }
  friend std::ostream &operator<<(std::ostream &os, const Token &token) {
    os << "[Token: " << tag_to_string[token._tag];
    if (token._tag == TOKEN::IDENTIFIER) {
      os << " : " << *token._value;
    }
    os << "] [" << token._position << "] ---> " << *token._value;
    return os;
  }
  bool IsKeyword() const {
    return TOKEN::KEYWORD_START < _tag && _tag < TOKEN::KEYWORD_END;
  }
  bool IsConstant() const {
    return TOKEN::CONSTANT_START < _tag && _tag < TOKEN::CONSTANT_END;
  }
  static std::unordered_map<std::string, TOKEN> string_to_tag;
  static std::unordered_map<TOKEN, std::string> tag_to_string;
};

#endif
