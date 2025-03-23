#ifndef TABLE_H
#define TABLE_H

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include <chrono>
#include <format>

#include "date.h"

#include "statement_importer.hpp"

class Table {
public:
  typedef std::vector<std::vector<std::string>>::iterator Iterator;
  typedef std::vector<std::vector<std::string>>::const_iterator ConstIterator;
  Table(std::string statement, Descriptor descriptor);
  int length();
  int width();
  void duplicate();
  float getAmount();
  void setAmount(float value);
  std::chrono::year_month_day getDate();
  void setDestination(std::string value);
  ConstIterator cbegin();
  Iterator cursor;
private:
  std::vector<std::string> stringToRow(std::string line);
  float parseAmount(std::string format, std::string cell);
  void storeAmount(int column, std::string format, float value);
  Descriptor descriptor;
  std::vector<int> columnWidths;
  std::vector<std::string> headers;
  std::vector<std::vector<std::string>> data;
};

#endif
