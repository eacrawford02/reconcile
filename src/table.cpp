#include "table.hpp"

Table::Table(std::string statement, std::string globalDateFormat, Descriptor
    descriptor) : globalDateFormat{globalDateFormat}, descriptor(descriptor) {
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

    // Convert the date to the global date format
    std::string& dateCell = row[descriptor.dateColumn];
    std::string statementDateFormat = descriptor.dateFormat;
    std::chrono::year_month_day date = parseDate(dateCell, statementDateFormat);
    std::string convertedDate = std::vformat("{:" + globalDateFormat + "}",
	std::make_format_args(date));
    dateCell = convertedDate;

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
  // Note that if the vector is at capacity (the number of elements stored in
  // the vector is equal to the number of elements the vector has space
  // allocated for) then an insert will cause an internal reallocation and copy
  // to occur, which will invalidate all element iterators and references. We
  // thus need to stash the iterator index and get a new iterator lease to avoid
  // illegal memory acesses during future operations on the vector
  int index = cursor - data.cbegin();
  // Apparently (according to Clang) std::vector:iterator is implicitly
  // convertible to std::vector::const_iterator (can't find any docs confirming
  // this, though)
  data.insert(cursor, *cursor);
  // Get new iterator handle
  cursor = data.begin() + index;
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
  return parseDate((*cursor)[descriptor.dateColumn], globalDateFormat);
}

std::chrono::year_month_day Table::getNextDate() const {
  if (cursor == data.cend()) {
    throw std::out_of_range("Attempting to access out of range element");
  }
  return parseDate((*(cursor + 1))[descriptor.dateColumn], globalDateFormat);
}

std::chrono::year_month_day Table::getPrevDate() const {
  if (cursor == data.cbegin()) {
    throw std::out_of_range("Attempting to access out of range element");
  }
  return parseDate((*(cursor - 1))[descriptor.dateColumn], globalDateFormat);
}

std::chrono::year_month_day Table::parseDate(std::string dateString, std::string
    dateFormat) const {
  std::istringstream dateStream{dateString};
  // FIXME: replace date library with std::chrono once Clang C++20 Calendar
  // extenstion is complete
  date::year_month_day date;
  date::from_stream(dateStream, dateFormat.c_str(), date);
  // Convert date::year_month_day back to std::chrono::year_month_day
  return std::chrono::year_month_day{std::chrono::sys_days{date}};
}

void Table::setDestination(std::string value) {
  writeCell(headers.size() - 1, value);
}

std::string Table::getPayee() {
  std::string payee;
  for (auto index : descriptor.payeeColumns) {
    payee += (*cursor)[index] + ' ';
  }
  if (!payee.empty()) payee.pop_back();
  return payee;
}

Table::ConstIterator Table::cbegin() const { return data.cbegin(); }

Table::ConstIterator Table::cend() const { return data.cend(); }

std::vector<std::string> Table::stringToRow(std::string line) {
  std::stringstream stream{line};
  std::string value;
  std::vector<std::string> row;
  while (std::getline(stream, value, ',')) {
    row.push_back(value);
  }
  // Handle case of trailing comma (denoting an empty cell for the last column)
  if (line.back() == ',') {
    row.push_back("");
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
  std::string formattedValue = std::vformat(format,
      std::make_format_args(value));
  if (descriptor.debitColumn == descriptor.creditColumn) {
    int singleColumn = descriptor.debitColumn;
    writeCell(singleColumn, formattedValue);
  } else {
    writeCell(column, formattedValue);
  }
}

void Table::writeCell(int column, std::string value) {
  // Update the column's width, if necessary, before inserting the value into
  // the cell
  int& columnWidth = columnWidths[column];
  auto& cell = (*cursor)[column];
  if (value.size() > columnWidth) {
    columnWidth = value.size();
  } else if (cell.size() == columnWidth && value.size() < columnWidth) {
    // If the cell being modified is currently the widest in the column and is
    // to be narrower after the insertion, determine the next largest cell in
    // the column
    int newMaxWidth = headers[column].size();
    for (auto row : data) {
      if (row[column].size() > newMaxWidth) newMaxWidth = row[column].size();
    }
    columnWidth = newMaxWidth;
  }
  cell = value;
}
