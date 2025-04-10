#include "input.hpp"

Input::Input(TableViewArray& tableViewArray, Prompt prompt) :
    tableViewArray{tableViewArray}, prompt{prompt} {
  // Set up initial prompt
  Table& table = tableViewArray.focusedTable();
  if (table.getAmount() >= 0) {
    this->prompt.debitPrompt(table.displayRow(table.cursor - table.cbegin()));
  } else {
    this->prompt.creditPrompt(table.displayRow(table.cursor - table.cbegin()));
  }

  cursor = buffer.cbegin();
}

void Input::process() {
  while (!quit) {
    wchar_t input = getch();
    state = nextState(input);

    switch (input) {
      case KEY_ENTER:
      case '\n':
      case '\r':
	evaluateInput(buffer);
	break;
      case KEY_BACKSPACE:
      case '\b':
      case 127:
	if (cursor != buffer.cbegin()) {
	  cursor--;
	  buffer.erase(cursor);
	}
	prompt.printInput(buffer, cursor - buffer.cbegin());
	break;
      case KEY_LEFT:
	if (cursor != buffer.cbegin()) cursor--;
	prompt.printInput(buffer, cursor - buffer.cbegin());
	break;
      case KEY_RIGHT:
	if (cursor != buffer.cend()) cursor++;
	prompt.printInput(buffer, cursor - buffer.cbegin());
	break;
      default:
	buffer.insert(cursor, input);
	cursor++;
	prompt.printInput(buffer, cursor - buffer.cbegin());
	break;
    }
  }
}

Input::State Input::nextState(wchar_t input) {
  State next;

  switch (state) {
    case INSERT:
      break;
    case BACKSPACE:
      break;
    case QUIT:
      break;
  }
  
  return next;
}

void Input::evaluateInput(std::string s) {
  if (s == "q") {
    quit = true;
  } else if (s == "s") {
  } else if (s == "b") {
  } else {
  }
}
