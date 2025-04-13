#ifndef INPUT_H
#define INPUT_H

#include <string>

#include <ncurses.h>

#include "table_view_array.hpp"
#include "prompt.hpp"

class Input {
public:
  Input(TableViewArray& tableViewArray, Prompt& prompt);
  void evaluate();
private:
  TableViewArray& tableViewArray;
  Prompt& prompt;
  bool quit = false;
};

#endif
