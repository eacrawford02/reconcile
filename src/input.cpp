#include "input.hpp"

Input::Input(TableViewArray& tableViewArray, Prompt& prompt) :
    tableViewArray{tableViewArray}, prompt{prompt} {
  // Set up initial prompt
  Table& table = tableViewArray.focusedTable();
  if (table.getAmount() >= 0) {
    prompt.debitPrompt(table.displayRow(table.cursor - table.cbegin()));
  } else {
    prompt.creditPrompt(table.displayRow(table.cursor - table.cbegin()));
  }
}

void Input::evaluate() {
  while (!quit) {
    std::string input = prompt.getInput();

    if (input == "q") {
      quit = true;
    } else if (input == "s") {
      // TODO: scroll down
    } else if (input == "b") {
      // TODO: scroll up
    } else {
    }
  }
}
