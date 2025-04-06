#include "table.hpp"

Table::Table(std::string statement, Descriptor descriptor) :
    descriptor(descriptor) {
  std::ifstream inputStream{statement};
  if (!inputStream.is_open()) {
    throw std::runtime_error("Error: Could not open file " + statement);
  }

  // Ensure that transaction record is correctly formatted (CSV)
  bool delimiterFound = false;
  std::string line;
  while (std::getline(inputStream, line)) {
    if (line.find(',') != std::string::npos) {
      delimiterFound = true;
      break;
    }
  }
  if (!delimiterFound) {
    inputStream.close();
    throw std::runtime_error("Error: CSV formatting could not be detected in " +
	statement);
  }

  // Break first valid row up into column names, start tracking column widths
  headers = stringToRow(line);
  for (auto header : headers) columnWidths.push_back(header.size());

  // Add category column
  headers.push_back("Destination");
  columnWidths.push_back(headers[headers.size() - 1].size());

  // Process remainder of file
  while (std::getline(inputStream, line)) {
    std::vector<std::string> row = stringToRow(line);
    row.push_back(""); // Empty string for category column
    data.push_back(row);
    
    // Keep track of column widths
    for (int i = 0; i < row.size(); i++) {
      if (row[i].size() > columnWidths[i]) {
	columnWidths[i] = row[i].size();
      }
    }
  }
  inputStream.close();

  cursor = data.begin();
}

int Table::length() { return data.size(); }

int Table::width() { return headers.size(); }

std::vector<int> Table::displayWidths() { return displayColumns(columnWidths); }

std::vector<std::string> Table::displayHeaders() {
  return displayColumns(headers);
}

std::vector<std::string> Table::displayRow(int row) {
  return displayColumns(data[row]);
}

void Table::duplicate() {
  // Apparently (according to Clang) std::vector:iterator is implicitly
  // convertible to std::vector::const_iterator (can't find any docs confirming
  // this, though)
  data.insert(cursor, *cursor);
}

float Table::getAmount() {
  if (descriptor.debitColumn == descriptor.creditColumn) {
    std::string cell = (*cursor)[descriptor.debitColumn];
    float amount;
    try {
      amount = parseAmount(descriptor.debitFormat, cell);
    } catch (const std::runtime_error& e) {
      try {
	amount = parseAmount(descriptor.creditFormat, cell);
      } catch (const std::runtime_error& e) {
	throw std::runtime_error("Error: cell value \"" + cell + "\" does not "
	    "match either debit or credit format specifiers");
      }
    }
    return amount;
  } else {
    std::string debitCell = (*cursor)[descriptor.debitColumn];
    std::string creditCell = (*cursor)[descriptor.creditColumn];
    if (debitCell != "") {
      return parseAmount(descriptor.debitFormat, debitCell);
    } else if (creditCell != "") {
      return parseAmount(descriptor.creditFormat, creditCell);
    } else {
      throw std::runtime_error("Error: neither debit nor credit columns "
	  "contain a value");
    }
  }
}

void Table::setAmount(float value) {
  if (value >= 0) {
    storeAmount(descriptor.debitColumn, descriptor.debitFormat, value);
  } else {
    storeAmount(descriptor.creditColumn, descriptor.creditFormat, value);
  }
}

std::chrono::year_month_day Table::getDate() const {
  std::istringstream rawDate{(*cursor)[descriptor.dateColumn]};
  const char* format = descriptor.dateFormat.c_str();
  // FIXME: replace date library with std::chrono once Clang C++20 Calendar
  // extenstion is complete
  date::year_month_day date;
  date::from_stream(rawDate, format, date);
  // Convert date::year_month_day back to std::chrono::year_month_day
  return std::chrono::year_month_day{std::chrono::sys_days{date}};
}

void Table::setDestination(std::string value) {
  (*cursor)[headers.size() - 1] = value;
}

Table::ConstIterator Table::cbegin() { return data.cbegin(); }

std::vector<std::string> Table::stringToRow(std::string line) {
  std::stringstream stream{line};
  std::string value;
  std::vector<std::string> row;
  while (std::getline(stream, value, ',')) {
    row.push_back(value);
  }
  return row;
}

float Table::parseAmount(std::string format, std::string cell) {
  int floatStart = format.find('{');
  std::string prefix = format.substr(0, floatStart);
  std::string suffix = format.substr(format.find('}') + 1);
  bool prefixFound = cell.find(prefix) != std::string::npos;
  bool suffixFound = cell.find(suffix) != std::string::npos;

  if (prefixFound && suffixFound) {
    int suffixSize = format.size() - format.find('}') - 1;
    int cellSuffixStart = cell.size() - suffixSize;
    float amount = std::stof(cell.substr(floatStart, cellSuffixStart));
    return amount;
  } else {
    throw std::runtime_error("Error parsing \"" + cell + "\": invalid amount "
	"format \"" + format + "\"");
  }
}

void Table::storeAmount(int column, std::string format, float value) {
  if (descriptor.debitColumn == descriptor.creditColumn) {
    int singleColumn = descriptor.debitColumn;
    (*cursor)[singleColumn] = std::vformat(format,
	std::make_format_args(value));
  } else {
    (*cursor)[column] = std::vformat(format, std::make_format_args(value));
  }
}
