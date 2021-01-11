#ifndef _DERIVED_TYPE_H_
#define _DERIVED_TYPE_H_

#include "type_base.h"
#include "../ast/stmt.h"
#include <list>

class Identifier;
class OrdinaryIdentifier;

class ArrayType : public DerivedType {
public:
  ArrayType(std::unique_ptr<Type> &base, int length)
      : _base(std::move(base)), _length(length) {}
  virtual bool IsArrayType() const { return true; }
  unsigned length() const { return _length; }
  virtual int width() const { return _base->width() * _length; }

private:
  std::unique_ptr<Type> _base;
  int _length;
};

class StructUnionType : public DerivedType {
protected:
  std::list<StructUnionMember *> _mem_list;

public:
  virtual bool IsStructOrUnionType() const { return true; }
  virtual void AddMemberObject(StructUnionMember *mem) {
    _mem_list.push_back(mem);
  }
};

class StructType : public StructUnionType {
public:
  virtual int width() const;
  virtual bool IsStructType() const { return true; }
};

class UnionType : public StructUnionType {
public:
  virtual int width() const;
  virtual bool IsUnionType() const { return true; }
};

class FunctionType : public DerivedType {
  friend std::ostream &operator<<(std::ostream &out, const FunctionType &type) {
    type.OStreamFullMessage(out);
    return out;
  }
public:
  FunctionType(std::unique_ptr<Type> &base) : _base(std::move(base)) {}
  virtual bool IsFunctionType() const override { return true; }
  virtual int width() const override { return 0; }
  void set_variadic(bool v) { _is_variadic = v; }
  void AddParameters(std::vector<std::unique_ptr<Symbol>> &src) {
    _parameter_list.insert(_parameter_list.end(),
                           std::make_move_iterator(src.begin()),
                           std::make_move_iterator(src.end()));
  }
  void PrintParameters() {}
  void set_compound_stmt(std::unique_ptr<CompoundStmt> &compound_stmt) {
    _compound_stmt = std::move(compound_stmt);
  }
  std::unique_ptr<CompoundStmt> &compound_stmt() {
    return _compound_stmt;
  }

private:
  std::unique_ptr<Type> _base = nullptr; // Actually the returned one.
  bool _is_variadic = false;
  std::vector<std::unique_ptr<Symbol>> _parameter_list;
  std::unique_ptr<CompoundStmt> _compound_stmt;

  virtual void OStreamConciseMessage(std::ostream &os) const override {
    os << "Function" << std::endl;
  }

  virtual void OStreamFullMessage(std::ostream &os) const override {
    os << "Type: Function" << std::endl;
    OStreamSpecifierQualifier(os);
    os << std::endl;
    os << "Total parameters: " << _parameter_list.size() << std::endl;
    os << "Total expressions: ";
    if (_compound_stmt) {
      os << _compound_stmt->stmts().size();
    } else {
      os << 0;
    }
    os << std::endl;
    os << "Variadic: " << (_is_variadic ? "true" : "false") << std::endl;
  }
};

class PointerType : public DerivedType {
  friend std::ostream &operator<<(std::ostream &out, const PointerType &type) {
    type.OStreamFullMessage(out);
    return out;
  }

public:
  PointerType(std::unique_ptr<Type> &base)
      : DerivedType(true), _base(std::move(base)) {}
  PointerType(std::unique_ptr<PointerType> &base)
      : DerivedType(true), _base(std::move(base)) {}
  virtual ~PointerType() = default;
  virtual int width() const override { return POINTER_WIDTH; }
  virtual bool IsPointerType() const override { return true; }
  std::unique_ptr<Type> &base() { return _base; }
  virtual void OStreamConciseMessage(std::ostream &os) const override {
    os << "Pointer" << std::endl;
  }

private:
  std::unique_ptr<Type> _base;
  virtual void OStreamFullMessage(std::ostream &os) const override {
    os << "Type: Pointer" << std::endl;
    auto type = _base.get();
    while (true) {
      os << "Pointer -> ";
      if (type->IsPointerType()) {
        type = (((PointerType *)type)->base()).get();
      } else {
        break;
      }
    }
    type->OStreamConciseMessage(os);
    OStreamSpecifierQualifier(os);
    os << std::endl;
  }
};

class AtomicType : public DerivedType {
public:
  virtual bool IsPointerType() const { return true; }
};

#endif
