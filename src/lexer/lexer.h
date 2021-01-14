#ifndef YYQC_SRC_SCANNER_H_
#define YYQC_SRC_SCANNER_H_
#include "../public/position.h"
#include "token.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class Lexer {
  friend bool HandleNumber(Lexer &);
  friend bool HandleIdentifier(Lexer &);
  friend bool HandleCharLiteral(Lexer &);

public:
  typedef std::vector<std::shared_ptr<Token>> TokenList;
  std::shared_ptr<Token> PeekCurrentToken() const {
    return _token_list[_current_token_index];
  }
  std::shared_ptr<Token> PeekNextToken() const {
    return _token_list[_current_token_index + 1];
  }
  std::shared_ptr<Token> ConsumeToken() { return _token_list[_current_token_index++]; }
  bool Tokenize();
  void PrintTokenList() const;
  void PrintPosition() const {
    std::cout << "(" << position().row() << ", " << position().column() << ")";
  }
  unsigned ScreenShot() { return _current_token_index; }
  void PutBack(unsigned screenshot) { _current_token_index = screenshot; }

private:
  Position _position;
  TokenList _token_list;
  const std::string _file_name;
  unsigned int _current_token_index = 0;
  std::string *_file_content_ptr;
  bool OpenFile(const std::string &);
  const std::string &file_content() const { return *_file_content_ptr; }
  unsigned int CurrentIndex() { return _position.index(); }
  const char &file_content(const unsigned int i) const {
    return file_content()[i];
  }
  const char &PeekCurrentChar() const {
    return file_content(_position.index());
  }
  bool PeekCurrentChar(const char ch) const { return PeekCurrentChar() == ch; }
  const char &PeekNextChar() const {
    return file_content(_position.index() + 1);
  }
  bool PeekNextChar(const char ch) const { return PeekNextChar() == ch; }
  const char &PeekForward(int forward) const {
    return file_content(_position.index() + forward);
  }
  const char &ConsumeChar() {
    const char &current = file_content(_position.index());
    if (current != '\n') {
      _position.NextColumn();
    } else {
      _position.NewLine(_position.index() + 1);
    }
    return current;
  }
  void ConsumeChars(unsigned int number) {
    for (unsigned i = 0; i < number; ++i) {
      ConsumeChar();
    }
  }

  void AddToken(const TOKEN tag, const Position &position) {
    auto token = std::make_shared<Token>(tag, position);
    _token_list.push_back(token);
  }
  void AddToken(const TOKEN tag, const Position &position,
                std::unique_ptr<Value> &value) {
    auto token = std::make_shared<Token>(tag, position, value);
    _token_list.push_back(token);
  }

public:
  Lexer(const std::string path, bool tokenized = true) : _file_name(path) {
    (void)tokenized;
    if (!OpenFile(_file_name)) {
      // TODO: handle failure of opening file.
    }
    Tokenize();
  }
  const Position &position() const { return _position; }
};

#endif
