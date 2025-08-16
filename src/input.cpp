#include "input.hpp"

Input::Input(TableViewArray& tableViewArray, Prompt& prompt, Autocomplete&
    autocomplete, TransactionMap& transactionMap) :
  tableViewArray{tableViewArray}, prompt{prompt}, autocomplete{autocomplete},
  transactionMap{transactionMap} {
  // Set up initial prompt
  promptAfterScroll();
}

void Input::evaluate() {
  while (state != QUIT) {
    // Get input
    Prompt::Type responseType;
    std::string input;
    try {
      responseType = prompt.response(input);
    } catch (const std::runtime_error& e) {
      continue; // Let the user re-attempt to enter valid input
    }

    Table* table = &tableViewArray.focusedTable();

    state = nextState(responseType, input);
    switch (state) {
      case RECORD:
	table->setDestination(input);
	tableViewArray.redrawFocusedView();
	transactionMap.addRelation(table->getPayee(), input);
	try {
	  tableViewArray.scrollDown();
	} catch (const std::out_of_range& e) { return; }
	promptAfterScroll();
	break;
      case AUTOCOMPLETE:
	prompt.writeField(autocomplete.complete(input));
	break;
      case SKIP:
	try {
	  tableViewArray.scrollDown();
	} catch (const std::out_of_range& e) { return; }
	promptAfterScroll();
	break;
      case BACK:
	try {
	  tableViewArray.scrollUp();
	} catch (const std::out_of_range& e) { break; }
	promptAfterScroll();
	break;
      case SPLIT:
	prompt.splitPrompt(table->displayRow(table->cursor - table->cbegin()));
	break;
      case RECORD_SPLIT:
	recordSplit(input);
	break;
    }
  }
}

Input::State Input::nextState(Prompt::Type responseType, std::string input) {
  State next;

  switch (state) {
    case SPLIT:
      if (responseType == Prompt::ENTER) {
	next = RECORD_SPLIT;
      } else {
	next = SPLIT;
      }
      break;
    default:
      if (responseType == Prompt::ENTER) {
	if (input == "q") {
	  next = QUIT;
	} else if (input == "s") {
	  next = SKIP;
	} else if (input == "b") {
	  next = BACK;
	} else if (input == "t") {
	  next = SPLIT;
	} else {
	  next = RECORD;
	}
      } else {
	next = AUTOCOMPLETE;
      }
      break;
  }

  return next;
}

void Input::promptAfterScroll() {
  // Recall that a scroll action may change which table is currently focused.
  // Thus, we must re-query the TableViewArray for the currently focused table
  Table& table = tableViewArray.focusedTable();
  auto row = table.displayRow(table.cursor - table.cbegin());
  auto hint = transactionMap.getDestination(table.getPayee());
  prompt.amountPrompt(table.getAmount(), row, hint);
}

void Input::recordSplit(std::string input) {
  float residual;// = std::stof(input);
  // FIXME: uncomment above line, remove try-catch once ncurses validation bug
  // is fixed
  try {
    residual = std::stof(input);
  } catch (const std::exception& e) {
    state = SPLIT; // Let the user re-attempt to enter valid input
    return;
  }
  Table& table = tableViewArray.focusedTable();
  table.duplicate();
  table.setAmount(residual);
  table.cursor++;
  table.setAmount(table.getAmount() - residual);
  table.cursor--;
  tableViewArray.redrawFocusedView();
  auto row = table.displayRow(table.cursor - table.cbegin());
  prompt.amountPrompt(table.getAmount(), row);
}
