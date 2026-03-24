#include "row.hpp"

Row::Row() :metadata{Metadata()} {}

Row::Row(std::string line) : Row(line, Metadata()) {}

Row::Row(std::string line, Metadata metadata) : metadata{metadata} {
  std::stringstream stream{line};
  std::string value;
  for (int i = 0; std::getline(stream, value, ','); i++) {
    cells.push_back(Cell{value});
  }
  // Handle case of trailing comma (denoting an empty cell for the last column)
  if (line.back() == ',') {
    cells.push_back(std::string(""));
  }

  // Populate any missing formatting strings
  this->metadata.formatting.resize(cells.size());
}

Cell& Row::operator[](int index) { return cells[index]; }

Cell const& Row::operator[](int index) const { return cells[index]; }

bool Row::operator<(Row const& other) const {
  // Empty argument for casting function calls is dependent on the table already
  // having parsed every row's date string
  auto date = cells[metadata.sortColumn].as<std::chrono::year_month_day>();
  auto otherDate = other[metadata.sortColumn].as<std::chrono::year_month_day>();
  // Lets rows with no specified sort column (e.g., header row) bubble to the
  // top when sorting
  if (metadata.sortColumn < 0) {
    return false;
  } else if (other.metadata.sortColumn < 0) {
    return true;
  } else {
    return date < otherDate;
  }
}

int Row::size() const { return cells.size(); }

Row::Iterator Row::begin() { return cells.begin(); }

Row::Iterator Row::end() { return cells.end(); }

Row::ConstIterator Row::cbegin() const { return cells.cbegin(); }

Row::ConstIterator Row::cend() const { return cells.cend(); }

void Row::push_back(Cell const& value, std::string formatString) { 
  cells.push_back(value);
  metadata.formatting.push_back(formatString);
}

Row Row::format() const {
  std::vector<int> allColumns;
  for (int i = 0; i < cells.size(); i++) { allColumns.push_back(i); }
  return format(allColumns);
}

Row Row::format(std::vector<int> const& columns) const {
  Row formattedRow;
  for (int i : columns) {
    auto formattedCell = cells[i].as<std::string>(metadata.formatting[i]);
    formattedRow.push_back(formattedCell);
  }
  return formattedRow;
}
