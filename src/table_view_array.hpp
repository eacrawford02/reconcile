#ifndef TABLE_VIEW_ARRAY_H
#define TABLE_VIEW_ARRAY_H

#include <vector>

#include <ncurses.h>

#include "table.hpp"
#include "table_view.hpp"

class TableViewArray {
public:
  TableViewArray(std::vector<Table>& tables, WINDOW* window);
  ~TableViewArray();
private:
  std::vector<WINDOW*> borders;
  std::vector<WINDOW*> contents;
  std::vector<Table>& tables;
  std::vector<TableView> tableViews;
};

#endif
