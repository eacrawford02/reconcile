#ifndef TABLE_ARRAY_H
#define TABLE_ARRAY_H

#include <vector>
#include <string>

#include "table.hpp"

class TableArray {
public:
  typedef std::vector<Table>::iterator Iterator;
  typedef std::vector<Table>::const_iterator ConstIterator;

  Table& operator[](int index);
  Table const& operator[](int index) const;
  bool operator<(TableArray const& other);
  int size() const;
  Iterator begin();
  Iterator end();
  ConstIterator cbegin() const;
  ConstIterator cend() const;
  void push_back(Table const& value);
private:
  std::vector<Table> tables;
};

#endif
