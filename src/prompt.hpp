#ifndef PROMPT_H
#define PROMPT_H

#include <vector>
#include <string>

#include <ncurses.h>
#include <form.h>

class Prompt {
public:
  enum Type {TAB, ENTER};
  Prompt(WINDOW* window);
  ~Prompt();
  void debitPrompt(std::vector<std::string> row);
  void creditPrompt(std::vector<std::string> row);
  Type response(std::string& value);
  void writeField(std::string contents);
private:
  WINDOW* window;
  WINDOW* fieldWindow;
  FIELD* fields[2];
  FORM* form;
  int fieldPosition = 0;
  void draw(std::vector<std::string> row, std::string message);
};

#endif

