#include "input.hpp"

Input::Input(TableViewArray& tableViewArray, Prompt& prompt, std::string
    accountsFile) : tableViewArray{tableViewArray}, prompt{prompt} {
  try {
    autocomplete = {accountsFile};
  } catch (std::runtime_error const& e) {
    // TODO: log warning properly
    std::cerr << e.what() << '\n';
  }

#ifdef DEBUG
  std::filesystem::path transactionMapFile;
  transactionMapFile = std::filesystem::current_path() / MAP;
  // std::filesystem::path is implicitly convertible to a string in this case
  // (since std::string is explicitly defined as the constructor argument's
  // type)
  transactionMap = {transactionMapFile};
#else
  // Define transaction map file path
  std::filesystem::path transactionMapFile;
  if (char const* home = std::getenv("HOME")) {
    transactionMapFile = std::filesystem::path{home} / MAP;
  } else {
    // TODO: log warning properly
    std::cerr << "Warning: User's $HOME environment variable is not set, ";
    std::cerr << "unable to access transaction mapping file at path ";
    std::cerr << MAP << '\n';
  }

  // Create reconcile cache directory if it does not exist
  if (!std::filesystem::exists(transactionMapFile.parent_path())) {
    try {
      std::filesystem::create_directories(transactionMapFile.parent_path());
    } catch (std::filesystem::filesystem_error const& e) {
      std::cerr << "Error: Unable to create transaction mapping file parent ";
      std::cerr << "directory - " << e.what() << '\n';
    }
  }
  transactionMap = {transactionMapFile};
#endif

  // Set up initial prompt
  promptAfterScroll();
}

// TODO: dynamically update autocomplete radix tree with new account names
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

    // Can't declare variables inside switch statement. Pointers so that they
    // can be reassigned after resizing operations
    Table* table = &tableViewArray.focusedTable();
    TableView* tableView = &tableViewArray.focusedTableView();
    Table::Iterator iterator = table->begin() + tableView->cursorIndex();

    state = nextState(responseType, input);
    switch (state) {
      case RECORD:
	table->setCounterparty(iterator, input);
	tableViewArray.redrawFocusedView();
	transactionMap.addRelation(table->getPayee(iterator), input);
	try {
	  tableViewArray.scrollDown();
	} catch (const std::out_of_range& e) { return; }
	promptAfterScroll();
	break;
      case AUTOCOMPLETE:
	prompt.writeField(autocomplete.complete(input));
	break;
      // TODO: give user option to select between pending and uncleared
      // transaction states
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
	prompt.splitPrompt(tableView->rowView());
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
  TableView& tableView = tableViewArray.focusedTableView();
  Table::ConstIterator iterator = table.begin() + tableView.cursorIndex();
  auto row = tableView.rowView();
  auto hint = transactionMap.getCounterparty(table.getPayee(iterator));
  prompt.amountPrompt(table.amount(iterator), row, hint);
}

void Input::recordSplit(std::string input) {
  int residual;// = std::stof(input) * 100;
  // FIXME: uncomment above line, remove try-catch once ncurses validation bug
  // is fixed
  try {
    residual = std::stof(input) * 100;
  } catch (const std::exception& e) {
    state = SPLIT; // Let the user re-attempt to enter valid input
    return;
  }
  Table& table = tableViewArray.focusedTable();
  TableView& tableView = tableViewArray.focusedTableView();
  Table::Iterator iterator = table.begin() + tableView.cursorIndex();
  Row duplicate = table[tableView.cursorIndex()];
  table.insert(iterator, duplicate);
  iterator = table.begin() + tableView.cursorIndex();
  table.amount(iterator, residual);
  iterator++;
  table.amount(iterator, table.amount(iterator) - residual);
  tableViewArray.redrawFocusedView();
  auto row = tableView.rowView();
  prompt.amountPrompt(table.amount(--iterator), row);
}
