#include "prompt.hpp"

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

void Prompt::debitPrompt(std::vector<std::string> row) {
  draw(row, "From which account is this amount coming?");
}

void Prompt::creditPrompt(std::vector<std::string> row) {
  draw(row, "To which account is this amount going?");
}

std::string Prompt::getInput() {
  bool read = true;
  wchar_t inputChar;

  while (read) {
    inputChar = wgetch(window);

    switch (inputChar) {
      case KEY_ENTER:
      case '\n':
      case '\r':
	read = false;
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

  // Retrieve user input from field buffer and trim off any trailing spaces
  char* input = field_buffer(fields[0], 0);
  int trim = strlen(input) - 1;
  char test = input[trim];
  while (input[trim] == ' ' && trim > 0) trim--;
  return std::string{input, static_cast<std::string::size_type>(trim + 1)};
}

void Prompt::draw(std::vector<std::string> row, std::string message) {
  werase(window);

  // Construct prompt based on row contents
  std::string border{'+'};
  std::string content{'|'};
  for (auto cell : row) {
    border.append(cell.size() + 2, '-');
    border.push_back('+');
    content.append(" " + cell + " |");
  }
  message.append(" ([account]/[q]uit/[s]kip/[b]ack) ");
  fieldPosition = message.size();

  // Move field window out of the way so it doesn't block mvwaddstr output
  mvderwin(fieldWindow, 4, fieldPosition);

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
  field_opts_off(fields[0], O_AUTOSKIP);
  wresize(fieldWindow, 1, width - fieldPosition);
  set_form_fields(form, fields);
  post_form(form);

  wrefresh(window);
}
