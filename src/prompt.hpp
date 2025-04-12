#ifndef PROMPT_H
#define PROMPT_H

#include <vector>
#include <string>

#include <ncurses.h>
#include <form.h>

class Prompt {
public:
  Prompt(WINDOW* window);
  ~Prompt();
  void debitPrompt(std::vector<std::string> row);
  void creditPrompt(std::vector<std::string> row);
  std::string getInput();
private:
  WINDOW* window;
  WINDOW* fieldWindow;
  FIELD* fields[2];
  FORM* form;
  int fieldPosition = 0;
  void draw(std::vector<std::string> row, std::string message);
};

#endif

