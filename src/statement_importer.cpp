#include "statement_importer.hpp"

namespace {
  namespace Key {
    constexpr std::string accountsArray = "accounts";
    constexpr std::string identifier = "identifier";
    constexpr std::string ledgerSource = "ledger_source";
    constexpr std::string dateColumn = "date_column";
    constexpr std::string dateFormat = "date_format";
    constexpr std::string debitColumn = "debit_column";
    constexpr std::string debitFormat = "debit_format";
    constexpr std::string creditColumn = "credit_column";
    constexpr std::string creditFormat = "credit_format";
    constexpr std::string payeeColumns = "payee_columns";
    constexpr std::string displayColumns = "display_columns";
  }
}

StatementImporter::StatementImporter(toml::table const& configs) {
  // Retrieve per-account config info
  toml::node_view<const toml::node> accounts = configs[Key::accountsArray];
  if (!accounts.is_array_of_tables()) {
    throw std::runtime_error("No array of tables found in configuration file");
  }

  // Iterate through each account's config table and copy values into
  // intermediate representation
  for (const toml::node& node : *accounts.as_array()) {
    const toml::table& table = *node.as_table();
    std::string identifier = table[Key::identifier].value_or("");
    identifiers.push_back(identifier);
    configsMap[identifier] = table;
  }
}

Descriptor StatementImporter::descriptor(std::string statementFile) {
  std::ifstream inputStream{statementFile};
  if (!inputStream.is_open()) {
    throw std::runtime_error("Error: Could not open file " + statementFile);
  }

  std::string line;
  for (auto identifier : identifiers) {
    while (std::getline(inputStream, line)) {
      if (line.find(identifier) != std::string::npos) {
	toml::table table = configsMap[identifier];
	struct Descriptor d = {
	  .ledgerSource = table[Key::ledgerSource].value_or(""),
	  .dateColumn = table[Key::dateColumn].value_or(0),
	  .debitColumn = table[Key::debitColumn].value_or(0),
	  .creditColumn = table[Key::creditColumn].value_or(0),
	  .dateFormat = table[Key::dateFormat].value_or(""),
	  .debitFormat = table[Key::debitFormat].value_or(""),
	  .creditFormat = table[Key::creditFormat].value_or(""),
	  .payeeColumns = arrayToVector(table[Key::payeeColumns].as_array()),
	  .displayColumns = arrayToVector(table[Key::displayColumns].as_array())
	};
	return d;
      }
    }
    // The while loop condition (std::basic_ios operator bool) returns false
    // when the stream's failbit (or badbit) is true. Both the eofbit and
    // failbit are set simultaneously when no characters are able to be
    // extracted from the screen (i.e., when getline attempts to read past the
    // end of the file); however, seekg only clears the eofbit. In order to
    // enter the body of the while loop during the next iteration of the for
    // loop, we must clear the failbit as well by calling std::basic_ios::clear
    inputStream.clear();
    inputStream.seekg(0);
  }
  inputStream.close();
  throw std::runtime_error("Error: Account identifier not found in " +
      statementFile);
}

std::vector<int> StatementImporter::arrayToVector(const toml::array* array) {
  std::vector<int> vector;
  array->for_each([&vector](const toml::node& element) {
    vector.push_back(element.value_or(0));
  });
  return vector;
}
