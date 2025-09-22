#ifndef INPUT_H
#define INPUT_H

#include <string>
#include <filesystem>
#include <iostream>

#include <ncurses.h>

#include "table_view_array.hpp"
#include "prompt.hpp"
#include "autocomplete.hpp"
#include "transaction_map.hpp"

class Input {
public:
  Input(TableViewArray& tableViewArray, Prompt& prompt, std::string
      accountsFile);
  void evaluate();
private:
  enum State {RECORD, AUTOCOMPLETE, SKIP, BACK, SPLIT, RECORD_SPLIT, QUIT};
  TableViewArray& tableViewArray;
  Prompt& prompt;
  Autocomplete autocomplete;
  TransactionMap transactionMap;
  State state = RECORD;
  Table* focusedTable();
  State nextState(Prompt::Type responseType, std::string input);
  void promptAfterScroll();
  void recordSplit(std::string input);
};

#endif
