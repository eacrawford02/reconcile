#ifndef INPUT_H
#define INPUT_H

#include <string>

#include <ncurses.h>

#include "table_view_array.hpp"
#include "prompt.hpp"

class Input {
public:
  Input(TableViewArray& tableViewArray, Prompt prompt);
  void process();
private:
  enum State {INSERT, BACKSPACE, QUIT};
  TableViewArray& tableViewArray;
  Prompt prompt;
  State state = INSERT;
  bool quit = false;
  std::string buffer = " ";
  std::string::const_iterator cursor;
  void evaluateInput(std::string s);
  State nextState(wchar_t input);
};

#endif
