#ifndef TABLE_H
#define TABLE_H

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <sstream>

#include "statement_importer.hpp"

class Table {
public:
  typedef std::vector<std::vector<std::string>>::iterator Iterator;
  typedef std::vector<std::vector<std::string>>::const_iterator ConstIterator;
  Table(std::string statement, Descriptor descriptor);
  int length();
  int width();
  void duplicate();
  void setAmount(float value);
  void setDestination(std::string value);
  ConstIterator cbegin();
  Iterator cursor;
private:
};

#endif
