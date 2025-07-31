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

  refresh();
}

void TableView::scrollUp() {
  if (table.cursor == table.cbegin()) {
    throw std::out_of_range("Attempting to scroll past beginning of table");
  }
  table.cursor--;

  int cursorIndex = (table.cursor - table.cbegin()) - head;
  int midpoint = view.size() / 2 + 1;

  // Don't scroll down if the top of the table is already in view or if the
  // cursor hasn't yet reached the midpoint of the view
  if (head > 0 && cursorIndex <= midpoint) {
    tail--;
    view.pop_back();
    head--;
    view.push_front(format(table.displayRow(head)));
  }
}

void TableView::scrollDown() {
  if (table.cursor == (table.cend() - 1)) {
    // Trailing scroll so that TableViewArray doesn't get "stuck" on a table
    // that has been scrolled to its bottom (if the cursor still points to the
    // last row when at the bottom of the table, the comparisons between tables
    // in TableViewArray, which uses the date of the row being pointed to, will
    // always select the table that has been scrolled to its bottom since it
    // technically has the earliest date)
    table.cursor++;
    throw std::out_of_range("Attempting to scroll past end of table");
  }
  table.cursor++;

  int cursorIndex = (table.cursor - table.cbegin()) - head;
  int midpoint = view.size() / 2 + 1;

  // Don't scroll down if the bottom of the table is already in view or if the
  // cursor hasn't yet reached the midpoint of the view
  if (tail < table.length() && cursorIndex >= midpoint) {
    head++;
    view.pop_front();
    tail++;
    view.push_back(format(table.displayRow(tail)));
  }
}

void TableView::draw() {
  werase(window);
  mvwprintw(window, 0, 0, "%s", formattedHeaders.c_str());

  // Print header divider (overwrites any portion of the headers that may have
  // been wrapped to the second line if the screen is too small)
  mvwprintw(window, 1, 0, "%s", std::string{}.append(width, '-').c_str());
  // Print each row of the view, highlighting the row pointed to by the cursor
  int cursorIndex = (table.cursor - table.cbegin()) - head;
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
  std::vector<int> columnWidths = table.displayWidths();
  std::vector<std::string> headers = table.displayHeaders();
  // Format header row by adding padding and column dividers
  for (int i = 0; i < headers.size(); i++) {
    formattedHeaders.append(headers[i]);
    int padding = columnWidths[i] - headers[i].size();
    formattedHeaders.append(padding, ' ');
    formattedHeaders.append(columnDivider);
  }
  int position = formattedHeaders.size() - columnSpacing;
  formattedHeaders.replace(position, columnSpacing, columnSpacing, ' ');

  // Refresh view contents with rows from table
  view.clear();
  int viewSize = height > table.length() ? table.length() : height;
  for (tail = head; tail < viewSize; tail++) {
    view.push_back(format(table.displayRow(tail)));
  }
}

std::string TableView::format(std::vector<std::string> row) {
  std::vector<int> columnWidths = table.displayWidths();
  std::string formattedRow;
  for (int i = 0; i < row.size(); i++) {
    formattedRow.append(row[i]);
    int padding = columnWidths[i] - row[i].size() + columnSpacing;
    formattedRow.append(padding, ' ');
  }
  //formattedRow.resize(formattedRow.size() - columnSpacing);
  return formattedRow;
}
