#include "input.hpp"

Input::Input(TableViewArray& tableViewArray, Prompt& prompt, Autocomplete&
    autocomplete) : tableViewArray{tableViewArray}, prompt{prompt},
    autocomplete{autocomplete} {
  // Set up initial prompt
  Table& table = tableViewArray.focusedTable();
  if (table.getAmount() >= 0) {
    prompt.debitPrompt(table.displayRow(table.cursor - table.cbegin()));
  } else {
    prompt.creditPrompt(table.displayRow(table.cursor - table.cbegin()));
  }
}

void Input::evaluate() {
  while (state != QUIT) {
    std::string input;
    Prompt::Type responseType = prompt.response(input);

    state = nextState(responseType, input);
    switch (state) {
      case RECORD:
	break;
      case AUTOCOMPLETE:
	prompt.writeField(autocomplete.complete(input));
	break;
      case SKIP:
	tableViewArray.scrollDown();
	break;
      case BACK:
	tableViewArray.scrollUp();
	break;
      case SPLIT:
	break;
    }
  }
}

Input::State Input::nextState(Prompt::Type responseType, std::string input) {
  State next;

  if (responseType == Prompt::ENTER) {
    if (input == "q") {
      next = QUIT;
    } else if (input == "s") {
      next = SKIP;
    } else if (input == "b") {
      next = BACK;
    } else {
      next = RECORD;
    }
  } else {
    next = AUTOCOMPLETE;
  }

  /*
  switch (state) {
    case RECORD:
      if (responseType == Prompt::ENTER) {
	if (input == "q") {
	  next = QUIT;
	} else if (input == "s") {
	  next = SKIP;
	} else if (input == "b") {
	  next = BACK;
	} else {
	  next = RECORD;
	}
      } else {
	next = AUTOCOMPLETE;
      }
      break;
    case AUTOCOMPLETE:
      break;
    case SKIP:
      break;
    case BACK:
      break;
    case SPLIT:
      break;
  }
  */

  return next;
}
