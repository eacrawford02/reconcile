#include "prompt.hpp"

namespace {
  // For some reason the contents of this string prevent it from being declared
  // a constexpr
  const std::string options = " ([account]/[q]uit/[s]kip/[b]ack/spli[t]) ";
}

Prompt::Prompt(WINDOW* window) : window{window} {
  // Allocate new field and corresponding field window with arbitrary
  // height/width and x position. These will be properly set in the draw
  // function
  fields[0] = new_field(1, 1, 0, 0, 0, 0); // Height/width must be >0
  fields[1] = NULL;
  fieldWindow = derwin(window, 1, 1, 0, 0); // Same for field window

  // Create form and make window associations
  form = new_form(fields);
  set_form_win(form, window);
  set_form_sub(form, fieldWindow);
  post_form(form);
  wrefresh(window);
}

Prompt::~Prompt() {
  unpost_form(form);
  free_form(form);
  free_field(fields[0]);
  delwin(fieldWindow);
}

void Prompt::amountPrompt(float amount, std::vector<std::string> row) {
  if (amount >= 0) debitPrompt(row);
  else creditPrompt(row);
}

void Prompt::splitPrompt(std::vector<std::string> row) {
  draw(row, "What amount should the row being split retain? ", true);
}

Prompt::Type Prompt::response(std::string& value) {
  bool read = true;
  wchar_t inputChar;
  Type responseType;

  while (read) {
    inputChar = wgetch(window);

    switch (inputChar) {
      case KEY_ENTER:
      case '\n':
      case '\r':
	read = false;
	responseType = ENTER;
	form_driver(form, REQ_VALIDATION);
	break;
      case KEY_STAB:
      case '\t':
	read = false;
	responseType = TAB;
	form_driver(form, REQ_VALIDATION);
	break;
      case KEY_BACKSPACE:
      case '\b':
      case 127:
	form_driver(form, REQ_DEL_PREV);
	break;
      case KEY_DC:
	form_driver(form, REQ_DEL_CHAR);
	break;
      case KEY_LEFT:
	form_driver(form, REQ_PREV_CHAR);
	break;
      case KEY_RIGHT:
	form_driver(form, REQ_NEXT_CHAR);
	break;
      case KEY_HOME:
	form_driver(form, REQ_BEG_FIELD);
	break;
      case KEY_END:
	form_driver(form, REQ_END_FIELD);
	break;
      default:
	form_driver(form, inputChar);
	break;
    }
  }

  if (form_driver(form, REQ_VALIDATION) != E_OK) {
    throw std::runtime_error("Error: Invalid prompt input");
  }

  // Retrieve user input from field buffer and trim off any trailing spaces
  char* input = field_buffer(fields[0], 0);
  //set_field_buffer(fields[0], 0, "");
  form_driver(form, REQ_CLR_FIELD);
  int trim = strlen(input) - 1;
  while (input[trim] == ' ' && trim > 0) trim--;
  value = std::string{input, static_cast<std::string::size_type>(trim + 1)};
  return responseType;
}

void Prompt::writeField(std::string contents) {
  set_field_buffer(fields[0], 0, contents.c_str());
  form_driver(form, REQ_END_FIELD);
}

void Prompt::debitPrompt(std::vector<std::string> row) {
  draw(row, "From which account is this amount coming?" + options, false);
}

void Prompt::creditPrompt(std::vector<std::string> row) {
  draw(row, "To which account is this amount going?" + options, false);
}

void Prompt::draw(std::vector<std::string> row, std::string message, bool
    numericInput) {
  werase(window);

  fieldPosition = message.size();
  // Construct prompt based on row contents
  std::string border{'+'};
  std::string content{'|'};
  for (auto cell : row) {
    border.append(cell.size() + 2, '-');
    border.push_back('+');
    content.append(" " + cell + " |");
  }

  // Move field window out of the way so it doesn't block mvwaddstr output
  mvderwin(fieldWindow, 3, 0);

  // Print constructed prompt
  mvwaddstr(window, 0, 0, border.c_str());
  mvwaddstr(window, 1, 0, content.c_str());
  mvwaddstr(window, 2, 0, border.c_str());
  mvwaddstr(window, 4, 0, message.c_str());

  // Resize field window and FIELD* so that it fills the horizontal prompt
  // window space following the printed message
  unpost_form(form);
  free_field(fields[0]);
  int height;
  int width;
  getmaxyx(window, height, width);
  fields[0] = new_field(1, width - fieldPosition, 0, 0, 0, 0);
  if (numericInput) set_field_type(fields[0], TYPE_NUMERIC, 2, 0, 0);
  field_opts_off(fields[0], O_AUTOSKIP);
  field_opts_off(fields[0], O_NULLOK);
  // If we are accepting input after a split prompt (i.e., only numerical input
  // is accepted for the amount field) then set the TYPE_NUMERIC field type
  // before posting the form
  wresize(fieldWindow, 1, width - fieldPosition);
  mvderwin(fieldWindow, 4, fieldPosition); // Move field window back into place
  set_form_fields(form, fields);
  post_form(form);

  wrefresh(window);
}
