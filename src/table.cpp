#include "table.hpp"

// TODO: Sort table after CSV file is loaded
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
  headers = Row(line);
  for (auto header : headers) {
    columnWidths.push_back(header.as<std::string>().size());
  }

  // Add category column
  std::string categoryHeader = "Destination";
  headers.push_back(Cell(categoryHeader));
  columnWidths.push_back(categoryHeader.size());

  // For dynamic formatting string lookup in updateWidth function
  formatting = std::vector<std::string>(headers.size());
  formatting[descriptor.dateColumn] = globalDateFormat;
  formatting[descriptor.debitColumn] = descriptor.debitFormat;
  formatting[descriptor.creditColumn] = descriptor.creditFormat;

  // Process remainder of file
  while (std::getline(inputStream, line)) {
    // Parse row, adding a trailing comma to create a category column
    Row row{line + ',', descriptor.dateColumn};
    rows.push_back(row);
    
    // Keep track of column widths
    for (int i = 0; i < row.size(); i++) {
      int width = row[i].as<std::string>().size();
      if (width > columnWidths[i]) {
	columnWidths[i] = width;
      }
    }

    // Parse strings in date column (for efficiency, also means that we don't
    // have to pass parse string to each row for use in their operator<
    // functions)
    std::string format = descriptor.dateFormat;
    Cell& original = row[descriptor.dateColumn];
    Cell parsed{original.as<std::chrono::year_month_day>(format)};
    original = std::move(parsed);
  }

  inputStream.close();
}

int Table::length() { return rows.size(); }

int Table::width() { return headers.size(); }

Row& Table::operator[](int index) { return rows[index]; }

Table::Iterator Table::insert(Table::ConstIterator position, const Row& value) {
  return rows.insert(position, value);
}

Amount Table::amount(Table::ConstIterator position) {
  if (descriptor.debitColumn == descriptor.creditColumn) {
    return (*position)[descriptor.debitColumn].as<Amount>();
  } else {
    Cell debitCell = (*position)[descriptor.debitColumn];
    Cell creditCell = (*position)[descriptor.creditColumn];
    if (!debitCell.as<std::string>().empty()) {
      return debitCell.as<Amount>(descriptor.debitFormat);
    } else if (!creditCell.as<std::string>().empty()) {
      return creditCell.as<Amount>(descriptor.creditFormat);
    } else {
      throw std::runtime_error("Error: neither debit nor credit columns "
	  "contain a value");
    }
  }
}

void Table::amount(Table::Iterator position, Amount value) {
  int column;
  std::string format;
  int complementaryColumn;
  std::string complementaryFormat;

  if (value >= 0) {
    column = descriptor.debitColumn;
    format = descriptor.debitFormat;
    complementaryColumn = descriptor.creditColumn;
    complementaryFormat = descriptor.creditFormat;
  } else {
    column = descriptor.creditColumn;
    format = descriptor.creditFormat;
    complementaryColumn = descriptor.debitColumn;
    complementaryFormat = descriptor.debitFormat;
  }

  // Overwrite existing cell and update column width tracking
  Cell cell{value};
  Cell& existingCell = (*position)[column];
  updateWidth(column, existingCell.as<std::string>(format),
      cell.as<std::string>(format));
  existingCell = cell;

  // If the existing value is negated and there are separate columns for debits
  // & credits, then we must clear the cell in the complementary column to avoid
  // having two amounts in a single row
  if (descriptor.debitColumn != descriptor.creditColumn) {
    Cell& complementaryCell = (*position)[complementaryColumn];
    if (!complementaryCell.as<std::string>(complementaryFormat).empty()) {
      auto complementaryValue =
	  complementaryCell.as<Amount>(complementaryFormat);
      // Amount type is a signed integer, so if the two amounts are of opposite
      // signs, then their respective MSBs are opposites and therefore taking
      // their bitwise XOR will produce a negative number
      if ((value ^ complementaryValue) < 0) {
	complementaryCell = Cell{std::string("")}; // TODO: add erase function
      }
    }
  }
}

std::chrono::year_month_day Table::getDate(Table::ConstIterator position) const {
  Cell cell = (*position)[descriptor.dateColumn];
  return cell.as<std::chrono::year_month_day>(globalDateFormat);
}

std::string Table::getAccount() { return descriptor.ledgerSource; }

std::string Table::getCounterparty(Table::ConstIterator position) {
  return (*position)[headers.size() - 1].as<std::string>();
}

void Table::setCounterparty(Table::Iterator position, std::string value) {
  int column = headers.size() - 1;
  Cell cell{value};
  Cell& existingCell = (*position)[column];
  updateWidth(column, existingCell.as<std::string>(), cell.as<std::string>());
  existingCell = cell;
}

std::string Table::getPayee(Table::ConstIterator position) {
  std::string payee;
  for (auto index : descriptor.payeeColumns) {
    payee += (*position)[index].as<std::string>() + ' ';
  }
  if (!payee.empty()) payee.pop_back();
  return payee;
}

Table::Iterator Table::begin() { return rows.begin(); }

Table::Iterator Table::end() { return rows.end(); }

Table::ConstIterator Table::cbegin() const { return rows.cbegin(); }

Table::ConstIterator Table::cend() const { return rows.cend(); }

Descriptor::AccountKind Table::normalBalance() {
  return descriptor.normalBalance;
}

void Table::updateWidth(int column, std::string existing, std::string value) {
  // Update the column's width, if necessary, before inserting the value into
  // the cell
  int& columnWidth = columnWidths[column];
  if (value.size() > columnWidth) {
    columnWidth = value.size();
  } else if (existing.size() == columnWidth && value.size() < columnWidth) {
    // If the cell being modified is currently the widest in the column and is
    // to be narrower after the insertion, determine the next largest cell in
    // the column
    int newMaxWidth = headers[column].as<std::string>().size();
    for (auto row : rows) {
      // Since the column index is a parameter, we don't know the stored type of
      // the column we're examining and thus must provide the appropriate
      // formatting string when casting the Cell's stored type to a string
      int width = row[column].as<std::string>(formatting[column]).size();
      if (width > newMaxWidth) newMaxWidth = width;
    }
    columnWidth = newMaxWidth;
  }
}
