#ifndef YYQC_SRC_VALUE_H_
#define YYQC_SRC_VALUE_H_
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class Value {
public:
  virtual ~Value(){};
  virtual void print() const { std::cout << ""; };
  virtual int IntValue() { return 0; }
  virtual char CharValue() { return 0; }
  virtual std::string StringValue() { return ""; }
};

class StringValue : public Value {
private:
  std::string _value;

public:
  StringValue(const std::string &str) : _value(str) {}
  virtual void print() const override { std::cout << _value; }
};

class CharValue : public Value {
private:
  char _value;

public:
  CharValue(const char ch) : _value(ch) {}
};

class IntValue : public Value {
private:
  long long _value;

public:
  IntValue(const long long l) : _value(l) {}
};

class FloatValue : public Value {
private:
  double _value;

public:
  FloatValue(const double d) : _value(d) {}
};

class IdentifierName : public Value {
private:
  std::string _value;

public:
  IdentifierName(const std::string &name) : _value(name) {}
  virtual void print() const override { std::cout << _value; }
};

#endif