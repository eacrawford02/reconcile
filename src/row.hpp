#ifndef ROW_H
#define ROW_H

#include <vector>
#include <string>
#include <sstream>
#include <chrono>

#include "cell.hpp"

class Row {
public:
  struct Metadata {
    int sortColumn = -1;
    std::vector<std::string> formatting;
  };

  typedef std::vector<Cell>::iterator Iterator;
  typedef std::vector<Cell>::const_iterator ConstIterator;

  Row();
  Row(std::string line);
  Row(std::string line, Metadata metadata);
  Cell& operator[](int index);
  Cell const& operator[](int index) const;
  bool operator<(Row const& other);
  int size() const;
  Iterator begin();
  Iterator end();
  ConstIterator cbegin() const;
  ConstIterator cend() const;
  void push_back(Cell const& value, std::string formatString = "");
  Row format() const;
  Row format(std::vector<int> const& columns) const;
private:
  Metadata metadata;
  std::vector<Cell> cells;
};

#endif
