#ifndef YYQC_SRC_VALUE_H_
#define YYQC_SRC_VALUE_H_
#include <cwchar>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <variant>

class Value {
  friend std::ostream& operator<<(std::ostream &os, const Value &value) {
    auto val_ptr = &value._value;
    if (auto pval = std::get_if<long long>(val_ptr)) {
      os << "Integer value: " << *pval; 
    } else if(auto pval = std::get_if<double>(val_ptr)) {
      os << "Float value: " << *pval;
    } else if (auto pval = std::get_if<char>(val_ptr)) {
      os << "Char value: " << *pval;
    } else if (auto pval = std::get_if<std::string>(val_ptr)){
      os << "String value: " << *pval;
    } else {
      os << "Cannot convert!!!";
    }
    return os;
  }
public:
  Value(long long x) : _value(x) {}
  Value(double x) : _value(x) {}
  Value(std::string x) : _value(std::move(x)) {}
  ~Value(){}
private:
  std::variant<long long, double,char ,std::string> _value = "";
};


#endif
