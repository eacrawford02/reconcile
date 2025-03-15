#ifndef STATEMENT_IMPORTER_H
#define STATEMENT_IMPORTER_H

#include <string>
#include <vector>

#include "toml.hpp"

struct Descriptor {
  std::string ledgerSource;
  int dateColumn;
  int payeeColumn;
  int debitColumn;
  int creditColumn;
  std::string dateFormat;
  std::string debitFormat;
  std::string creditFormat;
  std::vector<int> displayColumns;
};

class StatementImporter {
public:
  StatementImporter(toml::table const& configs);
  Descriptor descriptor(std::string statementFile);
private:
};

#endif
