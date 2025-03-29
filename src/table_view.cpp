#include "table_view.hpp"

namespace {
  constexpr std::string columnDivider = " | ";
  constexpr int columnSpacing = 3;
}

TableView::TableView(Table& table, WINDOW* window) : table{table},
    window{window} {
  columnWidths = table.displayWidths();

  int height;
  int width;
  getmaxyx(window, height, width);

  // Reserve first two lines for column headers and header divider
  height -= 2;
  std::vector<std::string> headers = table.displayHeaders();
  // Format header row by adding padding and column dividers
  std::string formattedHeaders;
  for (int i = 0; i < headers.size(); i++) {
    formattedHeaders.append(headers[i]);
    int padding = columnWidths[i] - headers[i].size();
    formattedHeaders.append(padding, ' ');
    formattedHeaders.append(columnDivider);
  }
  int position = formattedHeaders.size() - columnSpacing;
  formattedHeaders.replace(position, columnSpacing, columnSpacing, ' ');
  mvwprintw(window, 0, 0, "%s", formattedHeaders.c_str());
  // Print header divider
  mvwprintw(window, 1, 0, "%s", std::string{}.append(width, '-').c_str());

  int viewSize = height > table.length() ? table.length() : height;
  for (tail; tail < viewSize; tail++) {
    view.push_back(format(table.displayRow(tail)));
  }
}

void TableView::scrollUp() {}

void TableView::scrollDown() {}

void TableView::draw() {
  wmove(window, 2, 0); // Don't erase the first two lines (headers and divider)
  wclrtobot(window);
  for (int i = 0; i < view.size(); i++) {
    mvwprintw(window, i + 2, 0, "%s", view[i].c_str());
  }
  wrefresh(window);
}

std::string TableView::format(std::vector<std::string> row) {
  std::string formattedRow;
  for (int i = 0; i < row.size(); i++) {
    formattedRow.append(row[i]);
    int padding = columnWidths[i] - row[i].size() + columnSpacing;
    formattedRow.append(padding, ' ');
  }
  //formattedRow.resize(formattedRow.size() - columnSpacing);
  return formattedRow;
}
