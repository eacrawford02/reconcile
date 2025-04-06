#include "prompt.hpp"

Prompt::Prompt(WINDOW* window) : window{window} {}

void Prompt::debitPrompt(std::vector<std::string> row) {
  draw(row, "From which account is this amount coming?");
}

void Prompt::creditPrompt(std::vector<std::string> row) {
  draw(row, "To which account is this amount going?");
}

Prompt& Prompt::operator<<(char c) {}

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

  mvwprintw(window, 0, 0, "%s", border.c_str());
  mvwprintw(window, 1, 0, "%s", content.c_str());
  mvwprintw(window, 2, 0, "%s", border.c_str());
  mvwprintw(window, 4, 0, "%s", message.c_str());
  wrefresh(window);
}
