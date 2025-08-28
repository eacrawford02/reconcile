#ifndef FORMATTER_H
#define FORMATTER_H

#include <vector>
#include <string>
#include <algorithm>
#include <ostream>
#include <locale>
#include <iomanip>

#include "toml.hpp"

#include "table.hpp"

class Formatter {
public:
  Formatter(std::vector<Table>& tables, toml::table const& format);
  friend std::ostream& operator<<(std::ostream& out, Formatter const&
      formatter);
private:
  std::vector<Table>& tables;
  std::string locale;
  std::string indentation;
  std::string margin ;
  void formatRow(std::ostream& out, Table& table, int amountAlignment) const;
};

#endif
