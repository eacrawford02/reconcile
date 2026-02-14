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
  std::string operator[](int index);
  void scrollUp(); // Bound-checking
  void scrollDown(); // Bound-checking
  int cursorIndex() const;
  void draw();
  void refresh();
  bool focus = false;
private:
  Table& table;
  int index = 1;
  std::vector<int> displayColumns;
  WINDOW* window;
  int height;
  int width;
  std::string formattedHeaders;
  std::deque<std::string> view;
  int head = 1;
  int tail = 1;
  std::string rowView(Row const& row);
};

#endif

