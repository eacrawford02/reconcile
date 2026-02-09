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
  Row(std::string line, int const& sortColumn = -1);
  Row& operator=(Row other);
  Cell& operator[](int index);
  Cell const& operator[](int index) const;
  bool operator<(Row const& other);
  int size();
  Iterator begin();
  Iterator end();
  ConstIterator cbegin() const;
  ConstIterator cend() const;
  void push_back(Cell const& value);
  int const& sortColumn;
private:
  std::vector<Cell> cells;
};

#endif
