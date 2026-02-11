#include "table_view.hpp"

namespace {
  constexpr std::string columnDivider = " | ";
  constexpr int columnSpacing = 3;
}

TableView::TableView(Table& table, WINDOW* window) : table{table},
    window{window} {
  getmaxyx(window, height, width);
  // Reserve first two lines for column headers and header divider
  height -= 2;

  refresh(); // Refresh initializes tail to the correct value
}

std::string TableView::operator[](int index) {
  // The same index shall return the same element between TableViews and their
  // respective Tables
  return view[index - head];
}

Row TableView::rowView(int index);
  if (index < 0) index = this->index;
  Row rowView;
  for (int i : table.displayColumns()) {
    auto formattedCell = table[index][i].as<std::string>(table.formatString(i));
    rowView.push_back(formattedCell);
  }
  return rowView;
}

void TableView::scrollUp() {
  if (index == 0) {
    throw std::out_of_range("Attempting to scroll past beginning of table");
  }
  index--;

  int cursorIndex = index - head;
  int midpoint = view.size() / 2 + 1;

  // Don't scroll down if the top of the table is already in view or if the
  // cursor hasn't yet reached the midpoint of the view
  if (head > 0 && cursorIndex <= midpoint) {
    tail--;
    view.pop_back();
    view.push_front(format(table[--head]));
  }
}

void TableView::scrollDown() {
  if (index == (table.length() - 1)) {
    // Trailing scroll so that TableViewArray doesn't get "stuck" on a table
    // that has been scrolled to its bottom (if the cursor still points to the
    // last row when at the bottom of the table, the comparisons between tables
    // in TableViewArray, which uses the date of the row being pointed to, will
    // always select the table that has been scrolled to its bottom since it
    // technically has the earliest date)
    index++;
    throw std::out_of_range("Attempting to scroll past end of table");
  }
  index++;

  int cursorIndex = index - head;
  int midpoint = view.size() / 2 + 1;

  // Don't scroll down if the bottom of the table is already in view or if the
  // cursor hasn't yet reached the midpoint of the view
  if (tail < table.length() && cursorIndex >= midpoint) {
    head++;
    view.pop_front();
    // Post-increment tail so that when the view is scrolled to its bottom-most
    // position (i.e., when tail is incremented such that it is equal to
    // table.lenght()), we don't accidentally perform an out-of-bounds access
    view.push_back(format(table[tail++]));
  }
}

int TableView::cursorIndex() const { return index; }

void TableView::draw() {
  werase(window);
  mvwprintw(window, 0, 0, "%s", formattedHeaders.c_str());

  // Print header divider (overwrites any portion of the headers that may have
  // been wrapped to the second line if the screen is too small)
  mvwprintw(window, 1, 0, "%s", std::string{}.append(width, '-').c_str());
  // Print each row of the view, highlighting the row pointed to by the cursor
  int cursorIndex = index - head;
  for (int i = 0; i < view.size(); i++) {
    if (i == cursorIndex) {
      if (focus) {
	wattr_on(window, COLOR_PAIR(1), NULL);
      } else {
	wattr_on(window, COLOR_PAIR(2), NULL);
      }
    } else {
      wattr_off(window, COLOR_PAIR(1), NULL);
      wattr_off(window, COLOR_PAIR(2), NULL);
    }
    mvwprintw(window, i + 2, 0, "%s", view[i].c_str());
  }
  // If the cursor index is equal to view.size()-1 (i.e., last index of view
  // deque), then the COLOR_PAIR highlighting will be left on, since there are
  // no remaining indicies for which the else clause of the above block will be
  // triggered. Thus, we turn them off here as a catch-all
  wattr_off(window, COLOR_PAIR(1), NULL);
  wattr_off(window, COLOR_PAIR(2), NULL);
  wrefresh(window);
}

void TableView::refresh() {
  // Refresh header row (in case column widths have changed)
  formattedHeaders.clear();
  Row headers = table[0];
  // Format header row by adding padding and column dividers
  for (int i : table.displayColumns()) {
    auto formattedHeader = headers[i].as<std::string>(table.formatString(i));
    formattedHeaders.append(formattedHeader);
    int padding = table.columnWidth(i) - formattedHeader.size();
    formattedHeaders.append(padding, ' ');
    formattedHeaders.append(columnDivider);
  }
  // Replace trailing column divider with blank spaces
  int position = formattedHeaders.size() - columnSpacing;
  formattedHeaders.replace(position, columnSpacing, columnSpacing, ' ');

  // Refresh view contents with rows from table
  view.clear();
  int viewSize = height > table.length() ? table.length() : height;
  for (tail = head; tail < head + viewSize; tail++) {
    view.push_back(format(table[tail]));
  }
}

std::string TableView::format(Row const& row) {
  std::vector<int> displayColumns = table.displayColumns();
  std::string formattedRow;
  for (int i : table.displayColumns()) {
    std::string formattedCell = row[i].as<std::string>(table.formatString(i));
    formattedRow.append(formattedCell);
    int padding = table.columnWidth(i) - formattedCell.size() + columnSpacing;
    formattedRow.append(padding, ' ');
  }
  //formattedRow.resize(formattedRow.size() - columnSpacing);
  return formattedRow;
}
