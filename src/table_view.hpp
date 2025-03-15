#ifndef TABLE_VIEW_H
#define TABLE_VIEW_H

#include <vector>

#include <ncurses.h>

class TableView {
public:
  TableView(std::vector<WINDOW*> windows);
  void scrollUp(); // Bound-checking
  void scrollDown(); // Bound-checking
private:
};

#endif
