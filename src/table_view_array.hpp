#ifndef TABLE_VIEW_ARRAY_H
#define TABLE_VIEW_ARRAY_H

#include <vector>
#include <algorithm>
#include <chrono>

#include <ncurses.h>

#include "table.hpp"
#include "table_view.hpp"

class TableViewArray {
public:
  TableViewArray(std::vector<Table>& tables, WINDOW* window);
  ~TableViewArray();
  void scrollUp(); // Bound-checking
  void scrollDown(); // Bound-checking
  Table& focusedTable();
  void redrawFocusedView();
private:
  std::vector<WINDOW*> borders;
  std::vector<WINDOW*> contents;
  std::vector<Table>& tables;
  std::vector<TableView> tableViews;
  std::vector<int> indices;
  // Use an index instead of an iterator so that both tables and tableViews can
  // share the same variable
  int focusedIndex;
  int forwardFocus();
  int reverseFocus();
  std::chrono::year_month_day reverseDate(int tableIndex) const;
};

#endif
