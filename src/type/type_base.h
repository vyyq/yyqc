#ifndef _ABSTRACT_TYPE_H_
#define _ABSTRACT_TYPE_H_

#include "../error/error.h"
#include "../ast/ast_base.h"
#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

#define WORD_LENGTH 4
#define INT_LENGTH 4
#define POINTER_WIDTH 4

class Type;
class ArithmeticType;
class DerivedType;
class VoidType;
class CharType;
class IntType;
class FloatType;
class BoolType;
class ArrayType;
class StructUnionType;
class StructType;
class UnionType;
class FunctionType;
class PointerType;
class AtomicType;

class Symbol;

class StructUnionMember;

enum {
  SCS_MASK = 0xff,
  TS_MASK = 0xffff00,
  TQ_MASK = 0xf000000,
  FS_MASK = 0x30000000,
  AS_MASK = 0xc0000000,
};

// SCS stands for storage-class-specifier;
enum {
  SCS_TYPEDEF = 0x01,
  SCS_EXTERN = 0x02,
  SCS_STATIC = 0x04,
  SCS_THREAD_LOCAL = 0x08,
  SCS_AUTO = 0x10,
  SCS_REGISTER = 0x20
};

// TS stands for type-specifier:;
enum {
  TS_VOID = 0x100,
  TS_CHAR = 0x200,
  TS_SHORT = 0x400,
  TS_INT = 0x800,
  TS_LONG = 0x1000,
  TS_LONGLONG = 0x2000,
  TS_FLOAT = 0x4000,
  TS_DOUBLE = 0x8000,
  TS_SIGNED = 0x10000,
  TS_UNSIGNED = 0x20000,
  TS_BOOL = 0x40000,
  TS_COMPLEX = 0x80000,
  TS_ATOMIC = 0x100000,
  TS_STRUCT_UNION = 0x200000,
  TS_ENUM = 0x400000,
  TS_TYPEDEF = 0x800000
};

// TQ stands for type-qualifier;
enum {
  TQ_CONST = 0x1000000,
  TQ_RESTRICT = 0x2000000,
  TQ_VOLATILE = 0x4000000,
  TQ_ATOMIC = 0x8000000
};

// FS stands for function-specifier:
// AS stands for alignment-specifier:
enum {
  FS_INLINE = 0x10000000,
  FS_NOTRETURN = 0x20000000,

  AS_ALIGNAS_TN = 0x40000000, // -Alignas ( type-name )
  AS_ALIGNAS_CE = 0x80000000  // -Alignas ( constant-expression )
};

class Type {
protected:
  // uint32_t _flags = 0;
  uint32_t _storage_class_specifier_flag = 0;
  uint32_t _type_specifier_flag = 0;
  uint32_t _type_qualifier_flag = 0;
  uint32_t _function_specifier_flag = 0;
  bool _complete = false;

public:
  // uint32_t flags() { return _flags; }
  // void set_flags(uint32_t flags) { _flags = flags; }
  uint32_t storage_class_specifier() const {
    return _storage_class_specifier_flag;
  }
  uint32_t type_specifier() const { return _type_specifier_flag; }
  uint32_t type_qualifier() const { return _type_qualifier_flag; }
  uint32_t function_specifier() const { return _function_specifier_flag; }
  bool completed() const { return _complete; }
  void set_completed(bool completed = true) { _complete = completed; }
  void set_complete(bool complete) { this->_complete = complete; }
  virtual int width() const { return -1; };
  virtual void set_base(Type *) {
    Error("Invalid set_point_to() for current Type.\n");
  }

  // void AddFlag(uint32_t flags) { _flags |= flags; }
  void set_storage_class_specifier(uint32_t f) {
    _storage_class_specifier_flag = f;
  }
  void add_storage_class_specifier(uint32_t f) {
    _storage_class_specifier_flag |= f;
  }
  void set_type_specifier(uint32_t f) { _type_specifier_flag = f; }
  void add_type_specifier(uint32_t f) { _type_specifier_flag |= f; }
  void set_type_qualifier(uint32_t f) { _type_qualifier_flag = f; }
  void add_type_qualifier(uint32_t f) { _type_qualifier_flag |= f; }
  void set_function_specifier(uint32_t f) { _function_specifier_flag = f; }
  void add_function_specifier(uint32_t f) { _function_specifier_flag |= f; }

  Type(bool complete = true) : _complete(complete) {}
  Type(const std::unique_ptr<Type> &t)
      : _storage_class_specifier_flag(t->_storage_class_specifier_flag),
        _type_specifier_flag(t->_type_qualifier_flag),
        _type_qualifier_flag(t->_type_qualifier_flag),
        _function_specifier_flag(t->_function_specifier_flag),
        _complete(t->_complete) {}
  Type(uint32_t storage_class_specifier, uint32_t type_specifier,
       uint32_t type_qualifier, uint32_t function_specifier,
       bool complete = true)
      : _storage_class_specifier_flag(storage_class_specifier),
        _type_specifier_flag(type_specifier),
        _type_qualifier_flag(type_qualifier),
        _function_specifier_flag(function_specifier), _complete(complete) {}
  virtual ~Type() = default;
  virtual std::unique_ptr<Type> clone() const {
    auto new_type = std::make_unique<Type>(
        this->_storage_class_specifier_flag, this->_type_specifier_flag,
        this->_type_qualifier_flag, this->_function_specifier_flag,
        this->_complete);
    return new_type;
  }
  virtual void OStreamFullMessage(std::ostream &os) const {
    os << "Type:" << std::endl;
    OStreamSpecifierQualifier(os);
    os << std::endl;
  }

  virtual void OStreamConciseMessage(std::ostream &os) const {
    os << "Type" << std::endl;
  }

  void OStreamSpecifierQualifier(std::ostream &os) const {
    os << "Storage Class Specifier: ";
    OStreamStorageClassSpecifier(os);
    os << std::endl;
    os << "Type Specifier: ";
    OStreamTypeSpecifier(os);
    os << std::endl;
    os << "Type Qualifier: ";
    os << std::endl;
    os << "Function Specifier: ";
  }

  void OStreamStorageClassSpecifier(std::ostream &os) const {
    if (_storage_class_specifier_flag & SCS_AUTO) {
      os << "AUTO ";
    }
    if (_storage_class_specifier_flag & SCS_EXTERN) {
      os << "EXTERN ";
    }
    if (_storage_class_specifier_flag & SCS_REGISTER) {
      os << "REGISTER ";
    }
    if (_storage_class_specifier_flag & SCS_STATIC) {
      os << "STATIC ";
    }
    if (_storage_class_specifier_flag & SCS_THREAD_LOCAL) {
      os << "THREAD_LOCAL ";
    }
    if (_storage_class_specifier_flag & SCS_TYPEDEF) {
      os << "TYPEDEF ";
    }
  }

  void OStreamTypeSpecifier(std::ostream &os) const {
    if (_type_specifier_flag & TS_ATOMIC) {
      os << "ATOMIC ";
    }
    if (_type_specifier_flag & TS_BOOL) {
      os << "BOOL ";
    }
    if (_type_specifier_flag & TS_CHAR) {
      os << "CHAR ";
    }
    if (_type_specifier_flag & TS_COMPLEX) {
      os << "COMPLEX ";
    }
    if (_type_specifier_flag & TS_DOUBLE) {
      os << "DOUBLE ";
    }
    if (_type_specifier_flag & TS_ENUM) {
      os << "ENUM ";
    }
    if (_type_specifier_flag & TS_FLOAT) {
      os << "FLOAT ";
    }
    if (_type_specifier_flag & TS_INT) {
      os << "INT ";
    }
    if (_type_specifier_flag & TS_LONG) {
      os << "LONG ";
    }
    if (_type_specifier_flag & TS_LONGLONG) {
      os << "LONG_LONG ";
    }
    if (_type_specifier_flag & TS_SHORT) {
      os << "SHORT ";
    }
    if (_type_specifier_flag & TS_SIGNED) {
      os << "SIGNED ";
    }
    if (_type_specifier_flag & TS_STRUCT_UNION) {
      os << "STRUCT_UNION ";
    }
    if (_type_specifier_flag & TS_TYPEDEF) {
      os << "TYPEDEF ";
    }
    if (_type_specifier_flag & TS_UNSIGNED) {
      os << "UNSIGNED ";
    }
    if (_type_specifier_flag & TS_VOID) {
      os << "VOID ";
    }
  }

  friend std::ostream &operator<<(std::ostream &os, const Type &type) {
    type.OStreamFullMessage(os);
    return os;
  }

public:
  virtual bool IsArithmeticType() const { return false; }
  virtual bool IsCharType() const { return false; }
  virtual bool IsIntType() const { return false; }
  virtual bool IsFloatType() const { return false; }
  virtual bool isBoolType() const { return false; }

  virtual bool IsDerivedType() const { return false; }
  virtual bool IsArrayType() const { return false; }
  virtual bool IsStructOrUnionType() const { return false; }
  virtual bool IsStructType() const { return false; }
  virtual bool IsUnionType() const { return false; }
  virtual bool IsFunctionType() const { return false; }
  virtual bool IsPointerType() const { return false; }
  virtual bool IsAtomicType() const { return false; }

  virtual bool IsVoidType() const { return false; }
};

class ArithmeticType : public Type {
public:
  ArithmeticType() = default;
  explicit ArithmeticType(uint32_t storage_class_specifier,
                          uint32_t type_specifier, uint32_t type_qualifier,
                          uint32_t function_specifier, bool complete = true)
      : Type(storage_class_specifier, type_specifier, type_qualifier,
             function_specifier, complete) {}
  virtual ~ArithmeticType() = default;
  virtual bool IsArithmeticType() const { return true; }
  static ArithmeticType *Max(ArithmeticType *type1, ArithmeticType *type2);
};

class DerivedType : public Type {
protected:
public:
  DerivedType(bool complete = true) : Type(complete) {}
  virtual ~DerivedType() = default;
  virtual bool IsDerivedType() const { return true; }
};

class VoidType : public Type {
public:
  virtual ~VoidType() = default;
  virtual int width() const override { return 0; }
  VoidType() : Type(false) {}
  virtual bool IsVoidType() const override { return true; }
};

#endif

/**
 *  Chain structure (linked list) is designed for nested type.
 *  Notice that Type object is not allowed to be generated, since it
 *  is pure virtual. So we first let Type* be nullptr, and when eve-
 *  rything is ready, we create a new Type object and let Type* poi-
 *  nt to this new object.
 *           _________         _________
 *          |         |       |         |
 *          |  flags  |       |  flags  |
 *          |---------|       |---------|
 * .......  |  next   |------>|  next   |-------> ....
 *          |---------|       |---------|
 *          | Type *  |       |  Type * |
 *          |---------|       |---------|
 *
 * The actual meaning of this is for delaying the construction of
 * object of Type.
 */

// struct TypeNode {
//   // TypeNode *_point_to = nullptr;
//   // TypeNode(uint32_t flags, TypeNode *point_to, Type *type)
//   //     : _flags(flags), _point_to(point_to), _type(type) {}
//   uint32_t _flags = 0;
//   Type *_type = nullptr;
//   TypeNode() = default;
//   TypeNode(Type *type) : _type(type) {}
//   Type *type() const { return _type; }
//   void AddFlag(uint32_t flags) { _flags |= flags; }
//   bool HasFunctionSpecifier() { return (_flags | FS_MASK) != 0; }
//   uint32_t storage_class_specifier_flag() const { return _flags | SCS_MASK; }
//   uint32_t type_specifier_flag() const { return _flags | TS_MASK; }
//   uint32_t type_qualifier_flag() const { return _flags | TQ_MASK; }
//   uint32_t function_specifier_flag() const { return _flags | FS_MASK; }
//   uint32_t alignment_specifier_flag() const { return _flags | AS_MASK; }
//   bool TestFlag(uint32_t flag) { return (bool)(_flags | flag); }
//   void FillTypeField(Type *type) { _type = type; }
//   void set_type(Type *type) { FillTypeField(type); }

// };

// class TypeChain {
// public:
//   TypeChain() = default;
//   TypeChain(TypeNode *head) : _head(head) {}
//   TypeNode *head() const { return _head; }
//   TypeNode *safe_head() const {
//     assert(_head != nullptr);
//     return _head;
//   }
//   TypeNode *tail() const {
//     auto p = _head;
//     while (p != nullptr) {
//       p = p->point_to();
//     }
//     return p;
//   }

//   // Insert from head.
//   TypeNode *AddChainNode(Type *type = nullptr, uint32_t flags = 0) {
//     auto new_node = new TypeNode(flags, _head, type);
//     _head = new_node;
//     return new_node;
//   }

// private:
//   TypeNode *_head = nullptr;
// };
