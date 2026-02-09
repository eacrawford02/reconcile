#include "row.hpp"

Row::Row() : sortColumn{-1} {}

Row::Row(std::string line, int const& sortColumn) : sortColumn{sortColumn} {
  std::stringstream stream{line};
  std::string value;
  for (int i = 0; std::getline(stream, value, ','); i++) {
    cells.push_back(Cell{value});
  }
  // Handle case of trailing comma (denoting an empty cell for the last column)
  if (line.back() == ',') {
    cells.push_back("");
  }
}

Row::Row& operator=(Row::Row other) {
  std::swap(*this, other);
  return *this;
}

Cell& Row::operator[](int index) { return cells[index]; }

Cell const& Row::operator[](int index) const { return cells[index]; }

bool Row::operator<(Row const& other) {
  // Empty argument for casting function calls is dependent on the table already
  // having parsed every row's date string
  auto date = cells[sortColumn].as<std::chrono::year_month_day>();
  auto otherDate = other[sortColumn].as<std::chrono::year_month_day>();
  // Lets rows with no specified sort column (e.g., header row) bubble to the
  // top when sorting
  if (sortColumn < 0) {
    return false;
  } else if (other.sortColumn < 0) {
    return true;
  } else {
    return date < otherDate;
  }
}

int Row::size() { return cells.size(); }

Row::Iterator Row::begin() { return cells.begin(); }

Row::Iterator Row::end() { return cells.end(); }

Row::ConstIterator Row::cbegin() const { return cells.cbegin(); }

Row::ConstIterator Row::cend() const { return cells.cend(); }

void Row::push_back(Cell const& value) { cells.push_back(value); }
