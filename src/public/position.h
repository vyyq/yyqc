#ifndef YYQC_SRC_POSITION_H_
#define YYQC_SRC_POSITION_H_
#include <iostream>

class Position {
private:
  unsigned int _index = 0;
  unsigned int _row = 1;
  unsigned int _column = 1;
  unsigned int _line_head = 0;

public:
  Position() = default;
  Position(const unsigned int index, const unsigned int line_head,
           const unsigned int row, const unsigned int column)
      : _index(index), _line_head(line_head), _row(row), _column(column) {}
  const int line_head() const { return _line_head; }
  const unsigned int index() const { return _index; }
  const unsigned int row() const { return _row; }
  const unsigned int column() const { return _column; }
  const int StartAt() const { return _line_head + _column - 1; }
  void NewLine(const unsigned int new_line_head) {
    ++_row;
    ++_index;
    _column = 1;
    _line_head = new_line_head;
  }
  void NextColumn() {
    ++_column;
    ++_index;
  }
  friend std::ostream &operator<<(std::ostream &os, const Position &pos) {
    os << "Position: (" << pos._row << ", " << pos._column << ")";
    return os;
  }
};

#endif