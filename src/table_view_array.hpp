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
  // TODO: automatically detect and merge entries from separate accounts
  // representing either sides of the same transaction
  void scrollUp(); // Bound-checking
  void scrollDown(); // Bound-checking
  Table& focusedTable();
  void redrawFocusedView();
private:
  enum ScrollDirection {UP, DOWN};
  std::vector<WINDOW*> borders;
  std::vector<WINDOW*> contents;
  std::vector<Table>& tables;
  std::vector<TableView> tableViews;
  std::vector<int> indices;
  std::vector<bool> cursorAtHead;
  std::vector<ScrollDirection> lastTableScrolls;
  // Use an index instead of an iterator so that both tables and tableViews can
  // share the same variable. Initialize to negative value to avoid returning
  // the next date for table index 0 (integer default value) when performing
  // initial forward focus in constructor
  int focusedIndex = -1;
  int forwardFocus();
  int reverseFocus();
  std::chrono::year_month_day reverseDate(int tableIndex) const;
  std::chrono::year_month_day forwardDate(int tableIndex) const;
};

#endif
