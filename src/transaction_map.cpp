#include "transaction_map.hpp"

TransactionMap::TransactionMap(std::string mappingFile) {
  if (std::filesystem::exists(mappingFile)) {
    map = toml::parse_file(mappingFile);
  }
  // Open ofstream after parsing file, otherwise contents will be erased/lost
  out = std::ofstream{mappingFile};
}

TransactionMap& TransactionMap::operator=(TransactionMap other) {
  std::swap(map, other.map);
  std::swap(out, other.out);
  return *this;
}

TransactionMap::~TransactionMap() { out << map; }

void TransactionMap::addRelation(std::string payee, std::string destination) {
  // By default, create a new table containing a single destination key with an
  // initial tally of one and attempt to insert it into the top level map at a
  // new "payee" node
  toml::table newDestinationTally{{destination, 1}};
  bool payeeExists = !map.insert(payee, newDestinationTally).second;
  if (payeeExists) {
    // If that "payee" node already exists (will cause the insertion to fail),
    // then attempt to insert the destination key with an initial tally of one
    // under the "payee" node
    toml::table* payeeTable = map[payee].as_table();
    bool destinationExists = !payeeTable->insert(destination, 1).second;
    if (destinationExists) {
      // If that destination key is also already present in the "payee" node's
      // table, then simply retrieve its value and increment it
      auto tally = payeeTable->get_as<int64_t>(destination);
      payeeTable->insert_or_assign(destination, ++(**tally));
    }
  }
}

std::string TransactionMap::getDestination(std::string payee) {
  std::string result;
  // Retrieve the table nested under "payee" and check that it exists
  auto* payeeTable = map.get_as<toml::table>(payee);
  if (payeeTable != nullptr) {
    // If the payee exists in the map, then find the pair with the largest tally
    auto compare = [](auto const& a, auto const& b) {
      int tallyA = a.second.value_or(0);
      int tallyB = b.second.value_or(0);
      return tallyA < tallyB;
      /*
       * See
       * https://stackoverflow.com/questions/77144003/use-of-template-keyword-before-dependent-template-name
       * for why the following form cannot be used:
       *
       * return a.second.value_or<int64_t>(0) < b.second.value_or<int64_t>(0);
       */
    };
    toml::table_iterator pair;
    pair = std::max_element(payeeTable->begin(), payeeTable->end(), compare);
    // If the largest tally isn't greater than zero, then return an empty result
    if (pair->second.value_or<int64_t>(0) > 0) {
      result = pair->first.str();
    }
  }
  return result;
}
