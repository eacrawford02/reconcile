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
  int length() const;
  int width() const;
  Row& operator[](int index);
  Row const& operator[](int index) const;
  Table& operator+=(Table const& table);
  Iterator insert(ConstIterator position, const Row& value);
  int columnWidth(int column) const;
  // TODO: potentially move out of Table class
  std::string formatString(int column) const;
  Amount amount(ConstIterator position) const;
  void amount(Iterator position, Amount value);
  std::chrono::year_month_day getDate(ConstIterator position) const;
  std::string getAccount() const;
  std::string getCounterparty(ConstIterator position) const;
  void setCounterparty(Iterator position, std::string value);
  std::string getPayee(ConstIterator position) const;
  Iterator begin(); // Don't hold reference, may be invalidated
  Iterator end(); // Don't hold reference, may be invalidated
  ConstIterator cbegin() const; // Don't hold reference, may be invalidated
  ConstIterator cend() const; // Don't hold reference, may be invalidated
  std::string identifier() const;
  Descriptor::AccountKind normalBalance() const;
  std::vector<int> const& displayColumns();
private:
  void updateWidth(int column, std::string existing, std::string value);
  std::string globalDateFormat;
  Descriptor descriptor;
  std::vector<int> columnWidths;
  std::vector<std::string> formatting;
  std::vector<Row> rows;
};

#endif
