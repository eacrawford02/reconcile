#ifndef ROW_H
#define ROW_H

#include <vector>
#include <string>
#include <sstream>
#include <chrono>

#include "cell.hpp"

class Row {
public:
  typedef std::vector<Cell>::iterator Iterator;
  typedef std::vector<Cell>::const_iterator ConstIterator;

  Row();
  Row(std::string line, int sortColumn = -1);
  Cell& operator[](int index);
  Cell const& operator[](int index) const;
  bool operator<(Row const& other);
  int size();
  Iterator begin();
  Iterator end();
  ConstIterator cbegin() const;
  ConstIterator cend() const;
  void push_back(Cell const& value);
private:
  int sortColumn;
  std::vector<Cell> cells;
};

#endif
