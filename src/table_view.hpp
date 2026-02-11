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
  Row rowView(int index = -1);
  void scrollUp(); // Bound-checking
  void scrollDown(); // Bound-checking
  int cursorIndex() const;
  void draw();
  void refresh();
  bool focus = false;
private:
  Table& table;
  int index = 0;
  std::vector<int> displayColumns;
  WINDOW* window;
  int height;
  int width;
  std::string formattedHeaders;
  std::deque<std::string> view;
  int head = 0;
  int tail = 0;
  std::string format(Row const& row);
};

#endif

