#ifndef TABLE_VIEW_H
#define TABLE_VIEW_H

#include <deque>
#include <string>
#include <vector>
#include <stdexcept>

#include "ncurses.h"

#include "table.hpp"

class TableView {
public:
  TableView(Table& table, WINDOW* window);
  void scrollUp(); // Bound-checking
  void scrollDown(); // Bound-checking
  void draw();
  bool focus = false;
private:
  Table& table;
  WINDOW* window;
  std::vector<int> columnWidths;
  std::deque<std::string> view;
  int head = 0;
  int tail = 0;
  std::string format(std::vector<std::string> row);
};

#endif

