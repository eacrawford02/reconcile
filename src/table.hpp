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
  std::vector<int> displayWidths();
  std::vector<std::string> displayHeaders();
  std::vector<std::string> displayRow(int row);
  void duplicate();
  float getAmount();
  void setAmount(float value);
  std::chrono::year_month_day getDate() const;
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

  template<typename T>
  std::vector<T> displayColumns(std::vector<T> row) {
    std::vector<T> displayedColumns;
    for (int column : descriptor.displayColumns) {
      displayedColumns.push_back(row[column]);
    }
    return displayedColumns;
  }
};

#endif
