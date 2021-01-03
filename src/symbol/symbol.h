#ifndef YYQC_SYMBOL_H
#define YYQC_SYMBOL_H
#include "../lexer/token.h"
#include "../type/type_base.h"
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class Symbol {
public:
  std::unique_ptr<Type> _type;
  std::shared_ptr<Token> _token;

public:
  std::unique_ptr<Type> &type() { return _type; }
  void set_type(std::unique_ptr<Type> &type) { _type = std::move(type); }
  std::shared_ptr<Token> token() { return _token; }
  Symbol(std::shared_ptr<Token> token, std::unique_ptr<Type> &type)
      : _type(std::move(type)), _token(token) {}
  friend std::ostream &operator<<(std::ostream &os, const Symbol &symbol) {
    os << *(symbol._token);
    return os;
  }
};

#endif // YYQC_SYMBOL_H
