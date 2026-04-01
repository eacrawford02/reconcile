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
  rows.push_back(Row(line));
  for (auto header : rows[0]) {
    columnWidths.push_back(header.as<std::string>().size());
  }

  // Add category column
  std::string categoryHeader = "Destination";
  rows[0].push_back(Cell(categoryHeader));
  columnWidths.push_back(categoryHeader.size());

  // For dynamic formatting string lookup in updateWidth function
  formatting = std::vector<std::string>(rows[0].size());
  formatting[descriptor.dateColumn] = globalDateFormat;
  formatting[descriptor.debitColumn] = descriptor.debitFormat;
  formatting[descriptor.creditColumn] = descriptor.creditFormat;
  struct Row::Metadata metadata = {
    .sortColumn = descriptor.dateColumn,
    .formatting = formatting
  };

  // Process remainder of file
  while (std::getline(inputStream, line)) {
    // Parse row, adding a trailing comma to create a category column
    Row row{line + ',', metadata};

    // Parse strings in date column (for efficiency, also means that we don't
    // have to pass parse string to each row for use in their operator<
    // functions). Must be done before row is added to rows vector since
    // push_back creates a copy of the row object
    std::string format = descriptor.dateFormat;
    Cell& original = row[descriptor.dateColumn];
    Cell parsed{original.as<std::chrono::year_month_day>(format)};
    original = std::move(parsed);

    rows.push_back(row);
    
    // Keep track of column widths
    for (int i = 0; i < row.size(); i++) {
      // Since dates column has been parsed, pass formatting string to re-format
      // those cells
      int width = row[i].as<std::string>(formatting[i]).size();
      if (width > columnWidths[i]) {
	columnWidths[i] = width;
      }
    }
  }

  inputStream.close();
}

int Table::length() const { return rows.size(); }

int Table::width() const { return rows[0].size(); }

Row& Table::operator[](int index) { return rows[index]; }

Row const& Table::operator[](int index) const { return rows[index]; }

Table& Table::operator+=(Table const& table) {
  if (width() != table.width()) {
    throw std::length_error("Error: Table " + table.descriptor.ledgerSource +
	" is either too narrow or too wide to be appended to "
	+ descriptor.ledgerSource);
  }
  for (int i = 1; i < table.length(); i++) rows.push_back(table[i]);

  for (int i = 0; i < table.width(); i++) {
    if (table.columnWidth(i) > columnWidths[i]) {
      columnWidths[i] = table.columnWidth(i);
    }
  }
  return *this;
}

int Table::columnWidth(int column) const { return columnWidths[column]; }

std::string Table::formatString(int column) const { return formatting[column]; }

Table::Iterator Table::insert(Table::ConstIterator position, const Row& value) {
  return rows.insert(position, value);
}

Amount Table::amount(Table::ConstIterator position) const {
  if (descriptor.debitColumn == descriptor.creditColumn) {
    // TODO: check that either debit or credit format strings are non-empty
    Cell cell = (*position)[descriptor.debitColumn];
    return cell.as<Amount>(descriptor.debitFormat);
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
  std::string formattedCell = cell.as<std::string>(format);
  Cell& existingCell = (*position)[column];
  // For some reason, if cell.as<std::string>(format) is passed as an argument
  // to the below function call instead of formattedCell, the Cell:as function
  // will be called on the copy of the object cell, rather than the
  // pre-passed-by-value object
  updateWidth(column, existingCell.as<std::string>(format), formattedCell);
  existingCell = formattedCell;

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

std::chrono::year_month_day Table::getDate(Table::ConstIterator position) const
{
  Cell cell = (*position)[descriptor.dateColumn];
  return cell.as<std::chrono::year_month_day>();
}

std::string Table::getAccount() const { return descriptor.ledgerSource; }

std::string Table::getCounterparty(Table::ConstIterator position) const {
  return (*position)[rows[0].size() - 1].as<std::string>();
}

void Table::setCounterparty(Table::Iterator position, std::string value) {
  int column = rows[0].size() - 1;
  Cell cell{value};
  Cell& existingCell = (*position)[column];
  updateWidth(column, existingCell.as<std::string>(), cell.as<std::string>());
  existingCell = cell;
}

std::string Table::getPayee(Table::ConstIterator position) const {
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

std::string Table::identifier() const {
  return descriptor.identifier;
}

Descriptor::AccountKind Table::normalBalance() const {
  return descriptor.normalBalance;
}

std::vector<int> const& Table::displayColumns() {
  return descriptor.displayColumns;
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
    int newMaxWidth = rows[0][column].as<std::string>().size();
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
