#include "prompt.hpp"

Prompt::Prompt(WINDOW* window) : window{window} {}

void Prompt::debitPrompt(std::vector<std::string> row) {
  draw(row, "From which account is this amount coming?");
}

void Prompt::creditPrompt(std::vector<std::string> row) {
  draw(row, "To which account is this amount going?");
}

void Prompt::printInput(std::string input, int cursorIndex) {
  wmove(window, 4, inputHead);
  wclrtoeol(window);
  waddstr(window, input.c_str());

  // Highlight cursor location of input string
  /*
  int cursorIndex = cursor - input.cbegin();
  wattr_on(window, WA_REVERSE, NULL);
  mvwaddch(window, 4, inputHead + cursorIndex, input[cursorIndex]);
  wattr_off(window, WA_REVERSE, NULL);
  */

  wrefresh(window);
}

void Prompt::clear() {}

void Prompt::draw(std::vector<std::string> row, std::string message) {
  werase(window);

  std::string border{'+'};
  std::string content{'|'};
  for (auto cell : row) {
    border.append(cell.size() + 2, '-');
    border.push_back('+');
    content.append(" " + cell + " |");
  }
  message.append(" ([account]/[q]uit/[s]kip/[b]ack) ");
  inputHead = message.size();

  mvwaddstr(window, 0, 0, border.c_str());
  mvwaddstr(window, 1, 0, content.c_str());
  mvwaddstr(window, 2, 0, border.c_str());
  mvwaddstr(window, 4, 0, message.c_str());
  wrefresh(window);
}
