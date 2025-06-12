#ifndef INPUT_H
#define INPUT_H

#include <string>

#include <ncurses.h>

#include "table_view_array.hpp"
#include "prompt.hpp"
#include "autocomplete.hpp"

class Input {
public:
  Input(TableViewArray& tableViewArray, Prompt& prompt, Autocomplete&
      autocomplete);
  void evaluate();
private:
  enum State {RECORD, AUTOCOMPLETE, SKIP, BACK, SPLIT, RECORD_SPLIT, QUIT};
  TableViewArray& tableViewArray;
  Prompt& prompt;
  Autocomplete& autocomplete;
  State state = RECORD;
  Table* focusedTable();
  State nextState(Prompt::Type responseType, std::string input);
  void promptAfterScroll();
  void recordSplit(std::string input);
};

#endif
