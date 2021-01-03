#ifndef _TYPE_ARITHMETIC_
#define _TYPE_ARITHMETIC_

#include "type_base.h"
#define INT_SIZE 4
#define SHORT_SIZE ((INT_SIZE) >> (1))
#define LONG_SIZE ((INT_SIZE) << (1))
#define LONGLONG_SIZE ((INT_SIZE) << (2))
#define FLOAT_SIZE (INT_SIZE)
#define DOUBLE_SIZE ((INT_SIZE) << (1))

class CharType : public ArithmeticType {
public:
  CharType() {}
  virtual bool IsCharType() const { return true; }
  virtual int width() const { return 1; }
};

class IntType : public ArithmeticType {
  friend std::ostream &operator<<(std::ostream &out, const IntType &type) {
    type.OStreamFullMessage(out);
    return out;
  }

private:
  virtual void OStreamFullMessage(std::ostream &os) const override {
    os << "Type: INT" << std::endl;
    OStreamSpecifierQualifier(os);
    os << std::endl;
  }

public:
  IntType() = default;
  IntType(uint32_t storage_class_specifier, uint32_t type_specifier,
          uint32_t type_qualifier, uint32_t function_specifier,
          bool complete = true)
      : ArithmeticType(storage_class_specifier, type_specifier, type_qualifier,
                       function_specifier, complete) {}
  explicit IntType(const IntType *original)
      : ArithmeticType(original->storage_class_specifier(),
                       original->type_specifier(), original->type_qualifier(),
                       original->function_specifier(), original->completed()) {}
  virtual bool IsIntType() const override { return true; }
  virtual std::unique_ptr<Type> clone() const override {
    auto new_type = std::make_unique<IntType>(this);
    return std::move(new_type);
  }
  virtual int width() const override {
    switch (type_specifier()) {
    case TS_SHORT:
    case TS_SHORT | TS_UNSIGNED:
      return SHORT_SIZE;
    case TS_INT:
    case TS_INT | TS_UNSIGNED:
    case TS_UNSIGNED:
      return INT_SIZE;
    case TS_LONG:
    case TS_LONG | TS_UNSIGNED:
      return LONG_SIZE;
    case TS_LONGLONG:
    case TS_LONGLONG | TS_UNSIGNED:
      return LONGLONG_SIZE;
    default:
      // TODO: Error Handler.
      Error("Invalid int type!");
      return -1;
    }
  }
  virtual void OStreamConciseMessage(std::ostream &os) const override {
    os << "Int" << std::endl;
  }
};

class FloatType : public ArithmeticType {
public:
  virtual bool IsFloatType() const { return true; }
  virtual int width() const {
    switch (type_specifier()) {
    case TS_FLOAT:
      return FLOAT_SIZE;
    case TS_DOUBLE:
      return DOUBLE_SIZE;
    default:
      // TODO: Error Handler.
      Error("Invalid float type!");
      return -1;
    }
  }
};

class BoolType : public ArithmeticType {
public:
  virtual bool IsBoolType() const { return true; }
  virtual int width() const { return 1; }
};

#endif
