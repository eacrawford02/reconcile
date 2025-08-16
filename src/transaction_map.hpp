#ifndef TRANSACTION_MAP_H
#define TRANSACTION_MAP_H

#include <string>
#include <filesystem>
#include <fstream>
#include <algorithm>

#include "toml.hpp"

class TransactionMap {
public:
  TransactionMap(std::string mappingFile);
  ~TransactionMap();
  void addRelation(std::string payee, std::string destination);
  std::string getDestination(std::string payee);
private:
  toml::table map;
  std::ofstream out;
};

#endif
