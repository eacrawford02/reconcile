#include "table_view_array.hpp"

TableViewArray::TableViewArray(std::vector<Table>& tables, WINDOW* window) :
    tables{tables} {
  // Allocate screen space for each TableView
  int height;
  int width;
  getmaxyx(window, height, width);
  int subWidth = (int) width / tables.size();

  for (int i = 0; i < tables.size(); i++) {
    WINDOW* border = derwin(window, height, subWidth, 0, subWidth * i);
    WINDOW* content = derwin(border, height - 2, subWidth - 2, 1, 1);
    TableView tableView{tables[i], content};

    box(border, 0, 0); // Use default border characters
    wrefresh(border);
    tableView.draw();
    
    borders.push_back(border);
    contents.push_back(content);
    tableViews.push_back(tableView);
  }

  // Focus on a table
  nextFocus();
  tableViews[focusedIndex].focus = true;
  tableViews[focusedIndex].draw();
}

TableViewArray::~TableViewArray() {
  for (auto content : contents) delwin(content);
  for (auto border : borders) delwin(border);
}

void TableViewArray::scrollUp() {}

void TableViewArray::scrollDown() {
  // TODO: unfocus current view

  for (auto& table : tables) table.cursor++;
  auto compare = [](Table const& a, Table const& b) {
    return a.getDate() < b.getDate();
  };
  std::vector<Table>::iterator next;
  next = std::min_element(tables.begin(), tables.end(), compare);
  for (auto& table : tables) table.cursor--;

  //windows[0].scrollDown();
  // TODO: focus new view
}

Table& TableViewArray::focusedTable() { return tables[focusedIndex]; }

void TableViewArray::nextFocus() {
  // Determine which row, accross all tables, has the closest transaction date
  auto compare = [](Table const& a, Table const& b) {
    return a.getDate() < b.getDate();
  };
  std::vector<Table>::iterator nextTable;
  nextTable = std::min_element(tables.begin(), tables.end(), compare);
  focusedIndex = std::distance(tables.begin(), nextTable);
}
