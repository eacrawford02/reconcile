#ifndef STATEMENT_IMPORTER_H
#define STATEMENT_IMPORTER_H

#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>

#include "toml.hpp"

struct Descriptor {
  std::string ledgerSource;
  int dateColumn;
  int debitColumn;
  int creditColumn;
  std::string dateFormat;
  std::string debitFormat;
  std::string creditFormat;
  std::vector<int> payeeColumns;
  std::vector<int> displayColumns;
};

class StatementImporter {
public:
  StatementImporter(toml::table const& configs);
  Descriptor descriptor(std::string statementFile);
private:
  std::vector<int> arrayToVector(const toml::array* array);
  std::vector<std::string> identifiers;
  std::map<std::string, toml::table> configsMap;
};

#endif
