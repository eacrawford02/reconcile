#ifndef TABLE_H
#define TABLE_H

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <chrono>

#include "statement_importer.hpp"
#include "row.hpp"

class Table {
public:
  typedef std::vector<Row>::iterator Iterator;
  typedef std::vector<Row>::const_iterator ConstIterator;
  Table(std::string statement, std::string globalDateFormat, Descriptor
      descriptor);
  int length();
  int width();
  Row& operator[](int index);
  Iterator insert(ConstIterator position, const Row& value);
  Amount amount(ConstIterator position);
  void amount(Iterator position, Amount value);
  std::chrono::year_month_day getDate(ConstIterator position) const;
  std::string getAccount();
  std::string getCounterparty(ConstIterator position);
  void setCounterparty(Iterator position, std::string value);
  std::string getPayee(ConstIterator position);
  Iterator begin(); // Don't hold reference, may be invalidated
  Iterator end(); // Don't hold reference, may be invalidated
  ConstIterator cbegin() const; // Don't hold reference, may be invalidated
  ConstIterator cend() const; // Don't hold reference, may be invalidated
  Descriptor::AccountKind normalBalance();
private:
  void updateWidth(int column, std::string existing, std::string value);
  std::string globalDateFormat;
  Descriptor descriptor;
  std::vector<int> columnWidths;
  Row headers;
  std::vector<std::string> formatting;
  std::vector<Row> rows;
};

#endif
