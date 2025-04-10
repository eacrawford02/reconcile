#ifndef PROMPT_H
#define PROMPT_H

#include <vector>
#include <string>

#include <ncurses.h>

class Prompt {
public:
  Prompt(WINDOW* window);
  void debitPrompt(std::vector<std::string> row);
  void creditPrompt(std::vector<std::string> row);
  void printInput(std::string input, int cursorIndex);
  void clear();
private:
  WINDOW* window;
  int inputHead = 0;
  void draw(std::vector<std::string> row, std::string message);
};

#endif

