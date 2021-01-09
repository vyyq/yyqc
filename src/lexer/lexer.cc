#include "lexer.h"
#include <cctype>
#include <cstddef>
#include <cassert>
#include <iostream>
#include <string>
const size_t APPEND_SIZE = 10;

bool HandleNumber(Lexer &);
bool HandleIdentifier(Lexer &);
bool HandleCharLiteral(Lexer &);

bool Lexer::OpenFile(const std::string &path) {
  std::ifstream ifs(path);
  // TODO: if opening file not successfully?
  std::string content = std::string((std::istreambuf_iterator<char>(ifs)),
                                    (std::istreambuf_iterator<char>()));
  content.append(std::string(APPEND_SIZE, '\0'));
  _file_content_ptr = new std::string(content);
  return true;
}

bool Lexer::Tokenize() {
  TOKEN tag = TOKEN::FILE_EOF;
  bool need_space = false;
  for (;;) {
    while (isblank(PeekCurrentChar()) || PeekCurrentChar() == '\n') {
      ConsumeChar();
    }

    Position start_position = _position;
    if (PeekCurrentChar('\0')) {
      AddToken(TOKEN::FILE_EOF, _position);
      return true;
    }

    char curr = PeekCurrentChar();
    if (need_space && curr != ' ') {
      return false;
    }

    if (curr == '!') {
      // !
      // !=
      if (PeekNextChar('=')) {
        tag = TOKEN::NE;
        ConsumeChar();
      } else {
        tag = TOKEN::NOT;
      }
      AddToken(tag, start_position);
      ConsumeChar();
      continue;
    } else if (curr == '\"') {
      // string literal
      size_t length = 0;
      ConsumeChar();
      unsigned literal_start = CurrentIndex();
      while (PeekCurrentChar() != '\0' && PeekCurrentChar() != '\"') {
        ConsumeChar();
        ++length;
      }
      std::string string_value = file_content().substr(literal_start, length);
      AddToken(TOKEN::STRING_LITERAL, start_position,
               std::make_unique<Value>(string_value));
      ConsumeChar();
    } else if (curr == '#') {
      /* TODO */
      AddToken(TOKEN::SHARP, start_position);
      ConsumeChar();
      continue;
    } else if (curr == '%') {
      // %  modulo
      // %= modulo assign
      if (PeekNextChar('=')) {
        tag = TOKEN::MOD_ASSIGN;
        ConsumeChar();
      } else {
        tag = TOKEN::MOD;
      }
      AddToken(tag, start_position);
      ConsumeChar();
      continue;
    } else if (curr == '&') {
      // &
      // &=
      // &&
      if (PeekNextChar('&')) {
        tag = TOKEN::LOGICAL_AND;
        ConsumeChar();
      } else if (PeekNextChar('=')) {
        tag = TOKEN::AND_ASSIGN;
        ConsumeChar();
      } else {
        tag = TOKEN::AND;
      }
      AddToken(tag, start_position);
      ConsumeChar();
      continue;
    } else if (curr == '\'') {
      // char literal
      HandleCharLiteral(*this);
      continue;
    } else if (curr == '(') {
      AddToken(TOKEN::LPAR, start_position);
      ConsumeChar();
    } else if (curr == ')') {
      AddToken(TOKEN::RPAR, start_position);
      ConsumeChar();
    } else if (curr == '*') {
      // *        pointer, mul
      // *=
      if (PeekNextChar('=')) {
        tag = TOKEN::MUL_ASSIGN;
        ConsumeChar();
      } else {
        tag = TOKEN::STAR;
      }
      AddToken(tag, start_position);
      ConsumeChar();
    } else if (curr == '+') {
      // +
      // +=
      // ++
      if (PeekNextChar('=')) {
        tag = TOKEN::ADD_ASSIGN;
        ConsumeChar();
      } else if (PeekNextChar('+')) {
        tag = TOKEN::INCREMENT;
        ConsumeChar();
      } else {
        tag = TOKEN::ADD;
      }
      AddToken(tag, start_position);
      ConsumeChar();
    } else if (curr == ',') {
      AddToken(TOKEN::COMMA, start_position);
      ConsumeChar();
    } else if (curr == '-') {
      // -
      // --
      // -=
      // ->     dereference
      if (PeekNextChar('=')) {
        tag = TOKEN::SUB_ASSIGN;
        ConsumeChar();
      } else if (PeekNextChar('-')) {
        tag = TOKEN::DECREMENT;
        ConsumeChar();
      } else if (PeekNextChar('>')) {
        tag = TOKEN::PTR_MEM_REF;
        ConsumeChar();
      } else {
        tag = TOKEN::SUB;
      }
      AddToken(tag, start_position);
      ConsumeChar();
    } else if (curr == '.') {
      // ...      ellipsis
      // .5       float number
      // .        reference
      std::string value;
      if (PeekNextChar('.')) {
        if (PeekForward(2) == '.') {
          tag = TOKEN::ELLIPSIS;
          value = "...";
          ConsumeChars(2);
        } else {
          /* TODO: handle error. */
        }
      } else if (isdigit(PeekNextChar())) {
        HandleNumber(*this);
        continue;
      } else {
        tag = TOKEN::DOT;
        value = ".";
      }
      AddToken(tag, start_position);
      ConsumeChar();
      continue;
    } else if (curr == '/') {
      // /
      // /=
      // /*
      // //
      if (PeekNextChar('/')) {
        while (PeekCurrentChar() != '\n') {
          if (PeekCurrentChar('\0')) {
            AddToken(TOKEN::FILE_EOF, _position);
            return true;
          }
          ConsumeChar();
        }
        ConsumeChar();
        continue;
      } else if (PeekNextChar('*')) {
        ConsumeChars(2);
        while (!PeekCurrentChar('\0') &&
               !(PeekCurrentChar('*') && PeekNextChar('/'))) {
          ConsumeChar();
        }
        if (PeekCurrentChar('\0')) {
          /* TODO: Handle error. */
        } else {
          ConsumeChars(2);
        }
        continue;
      } else if (PeekNextChar('=')) {
        tag = TOKEN::DIV_ASSIGN;
        ConsumeChar();
      } else {
        tag = TOKEN::DIV;
      }
      AddToken(tag, start_position);
      continue;
    } else if (curr == ':') {
      AddToken(TOKEN::COLON, start_position);
      ConsumeChar();
      continue;
    } else if (curr == ';') {
      AddToken(TOKEN::SEMI, start_position);
      ConsumeChar();
      continue;
    } else if (curr == '<') {
      // <
      // <=
      // <<
      // <<=
      if (PeekNextChar('=')) {
        tag = TOKEN::LE;
        ConsumeChar();
      } else if (PeekNextChar('<')) {
        if (PeekForward(2) == '=') {
          tag = TOKEN::LEFT_ASSIGN;
          ConsumeToken();
        } else {
          tag = TOKEN::LEFT_SHIFT;
        }
        ConsumeChar();
      } else {
        tag = TOKEN::LESS;
      }
      AddToken(tag, start_position);
      ConsumeChar();
    } else if (curr == '=') {
      if (PeekNextChar('=')) {
        tag = TOKEN::EQ;
        ConsumeChar();
      } else {
        tag = TOKEN::ASSIGN;
      }
      AddToken(tag, start_position);
      ConsumeChar();
    } else if (curr == '>') {
      if (PeekNextChar('=')) {
        tag = TOKEN::GE;
        ConsumeChar();
      } else if (PeekNextChar('>')) {
        if (PeekForward(2) == '=') {
          tag = TOKEN::RIGHT_ASSIGN;
          ConsumeToken();
        } else {
          tag = TOKEN::RIGHT_SHIFT;
        }
        ConsumeChar();
      } else {
        tag = TOKEN::GREATER;
      }
      AddToken(tag, start_position);
      ConsumeChar();
    } else if (curr == '?') {
      AddToken(TOKEN::COND, start_position);
      ConsumeChar();
    } else if (curr == '@') {
      AddToken(TOKEN::AT, start_position);
      ConsumeChar();
    } else if (curr == '[') {
      AddToken(TOKEN::LSQUBRKT, start_position);
      ConsumeChar();
    } else if (curr == '\\') {
      /* TODO */
    } else if (curr == ']') {
      AddToken(TOKEN::RSQUBRKT, start_position);
      ConsumeChar();
    } else if (curr == '^') {
      if (PeekNextChar('=')) {
        tag = TOKEN::XOR_ASSIGN;
      } else {
        tag = TOKEN::XOR;
      }
      AddToken(tag, start_position);
      ConsumeToken();
    } else if (curr == '_') {
      HandleIdentifier(*this);
      continue;
    } else if (curr == '`') {
      /* TODO */
    } else if (curr == '{') {
      AddToken(TOKEN::LBRACE, start_position);
      ConsumeChar();
    } else if (curr == '|') {
      if (PeekNextChar('=')) {
        tag = TOKEN::OR_ASSIGN;
      } else if (PeekNextChar('|')) {
        tag = TOKEN::LOGICAL_OR;
      } else {
        tag = TOKEN::OR;
      }
      AddToken(tag, start_position);
      ConsumeToken();
    } else if (curr == '}') {
      AddToken(TOKEN::RBRACE, start_position);
      ConsumeChar();
    } else if (curr == '~') {
      AddToken(TOKEN::NOT, start_position);
      ConsumeChar();
    } else if ((curr >= 'a' && curr <= 'z') || (curr >= 'A' && curr <= 'Z')) {
      HandleIdentifier(*this);
    } else {
      if (isdigit(PeekCurrentChar())) {
        HandleNumber(*this);
        continue;
      }
    }
  }
}

void Lexer::PrintTokenList() const {
  for (auto &token_ptr : _token_list) {
    std::cout << "tag: " << Token::tag_to_string[token_ptr->tag()] << ", "
              << "val: ";
    if (token_ptr->tag() != TOKEN::FILE_EOF) {
      if (token_ptr->value() == nullptr) {
        std::cout << std::string(1, (char)(token_ptr->tag()));
      } else {
        std::cout << *token_ptr->value();
      }
    }
    std::cout << ", position: (" << token_ptr->position().row() << ", "
              << token_ptr->position().column() << ")";
    std::cout << std::endl;
  }
}

bool HandleNumber(Lexer &lexer) {
  bool has_signed = false;
  bool is_integer = true;
  unsigned base = 0x0;
  (void)base;
  unsigned int literal_start = lexer.CurrentIndex();
  Position start_position = lexer.position();
  unsigned int length = 0;
  TOKEN tag = TOKEN::INTEGER_CONTANT;
  while (true) {
    char current_char = lexer.PeekCurrentChar();
    switch (current_char) {
    case '.':
      is_integer = false;
      tag = TOKEN::FLOATING_CONSTANT;
      break;
    case '0' ... '9':
    case 'e':
    case 'E':
      break;
    case '+':
    case '-':
      if (has_signed) {
        /* TODO: handle error */
        break;
      } else {
        has_signed = true;
        break;
      }
    case '\0':
    default:
      std::string str_val = lexer.file_content().substr(literal_start, length);
      if (is_integer) {
        // TODO
        auto val = std::make_unique<Value>(std::stoll(str_val));
        lexer.AddToken(tag, start_position, std::move(val));
      } else {
        auto val = std::make_unique<Value>(std::stod(str_val));
        lexer.AddToken(tag, start_position, std::move(val));
      }
      return true;
    }
    ++length;
    lexer.ConsumeChar();
  }
  return false;
}

bool HandleIdentifier(Lexer &lexer) {
  unsigned int identifier_start = lexer.CurrentIndex();
  Position start_position = lexer.position();
  unsigned int length = 0;
  bool first = true;
  TOKEN tag = TOKEN::IDENTIFIER;
  while (true) {
    char current_char = lexer.PeekCurrentChar();
    switch (current_char) {
    case 'a' ... 'z':
    case 'A' ... 'Z':
    case '_':
      break;
    case '0' ... '9':
      if (first) {
        return false;
      }
      break;
    default:
      std::string word = lexer.file_content().substr(identifier_start, length);
      if (Token::string_to_tag.find(word) != Token::string_to_tag.end()) {
        tag = Token::string_to_tag[word];
      }
      std::string str_val =
          lexer.file_content().substr(identifier_start, length);
      lexer.AddToken(tag, start_position,
                     std::make_unique<Value>(str_val));
      return true;
    }
    ++length;
    lexer.ConsumeChar();
    if (first) {
      first = false;
    }
  }
  return false;
}

char stoc(const std::string &str) {
  if (str[0] != '\\') {
    assert(str.size() == 1);
    return str[0];
  } else {
    return '\0';
  }
}

bool HandleCharLiteral(Lexer &lexer) {
  Position start_position = lexer.position();
  unsigned length = 0;
  lexer.ConsumeChar();
  unsigned literal_start = lexer.CurrentIndex();
  while ((!lexer.PeekCurrentChar('\0')) && (!lexer.PeekCurrentChar('\''))) {
    lexer.ConsumeChar();
    ++length;
  }
  std::string str_val = lexer.file_content().substr(literal_start, length);
  lexer.AddToken(TOKEN::CHARACTER_CONSTANT, start_position,
                 std::make_unique<Value>(str_val));
  lexer.ConsumeChar();
  return true;
}

/**
 *  integer-constant  ->
 *                        decimal-constant integer-suffix_{opt}
 *                        octal-constant integer-suffix_{opt}
 *                        hexadecimal-constant integer-suffix_{opt}
 */

/**
 *  decimal-constant  ->
 *                        nonzero-digit
 *                        decimal-constant digit
 *
 *  which is left recursive and needed to be rewritten as:
 *
 *  decimal-constant  ->  nonzero-digit decimal-constant'
 *  decimal-constant' ->  digit decimal-consant'
 *                        ${epsilon}
 */

/**
 *  floating-constant ->
 *                        decimal-floating-constant
 *                        hexadecimal-floating-constant
 */

/**
 *  character-constant  ->
 *                          ' c-char-sequence  '
 *                          L' c-char-sequence '
 *                          u' c-char-sequence '
 *                          U' c-char-sequence '
 */

/**
 *  enumeration-constant  -> identifier
 */
