#ifndef YYQC_ENV_H
#define YYQC_ENV_H

#include "./symbol.h"
#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <vector>

class Scope : public std::enable_shared_from_this<Scope> {
public:
  Scope() = default;
  ~Scope() = default;

  Scope(std::weak_ptr<Scope> parent) : _parent(parent) {}

  Scope(std::shared_ptr<Scope> parent) : _parent(parent) {}

  std::shared_ptr<Scope> &AddSubScope() {
    auto child = std::make_shared<Scope>();
    child->set_parent(weak_from_this());
    _children.push_back(child);
    return _children.back();
  }

  void set_parent(std::weak_ptr<Scope> parent) {
    _parent = parent;
  }

  void AddSubScope(std::shared_ptr<Scope> &child) {
    _children.push_back(std::move(child));
  }

  void AddSymbol(std::unique_ptr<Symbol> &symbol) {
    _symbols.push_back(std::move(symbol));
  }

  void AddSymbols(std::vector<std::unique_ptr<Symbol>> &symbols) {
    _symbols.insert(_symbols.end(), std::make_move_iterator(symbols.begin()),
                    std::make_move_iterator(symbols.end()));
  }

  std::weak_ptr<Scope> parent() { return _parent; }

  std::vector<std::shared_ptr<Scope>> &children() { return _children; }

  std::vector<std::unique_ptr<Symbol>> &symbols() { return _symbols; }

  // bool FindCurrentScope(const Symbol *var) {
  //   auto iter = std::find_if(
  //       _symbols.begin(), _symbols.end(),
  //       [&](std::unique_ptr<Symbol> &p) { return p.get() == var; });
  //   return (iter == _symbols.end());
  // }

  // bool FindAncestorScope(const Symbol *var) {
  //   auto scope = _parent;
  //   while (scope.lock()) {
  //     if (scope.lock()->FindCurrentScope(var)) {
  //       return true;
  //     } else {
  //       scope = scope.lock()->_parent;
  //     }
  //   }
  //   return false;
  // }

  // bool FindValidScope(const Symbol *var) {
  //   return FindCurrentScope(var) || FindAncestorScope(var);
  // }

  void PrintCurrentSymbols() {
    std::cout << (_symbols[0]->_type->IsIntType() ? "true" : "false")
              << std::endl;
    _symbols[0]->_type->OStreamFullMessage(std::cout);
  }

private:
  std::vector<std::unique_ptr<Symbol>> _symbols;
  std::weak_ptr<Scope> _parent;
  std::vector<std::shared_ptr<Scope>> _children;
};

#endif // YYQC_ENV_H
