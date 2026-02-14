#ifndef PROMPT_H
#define PROMPT_H

#include <vector>
#include <string>

#include <ncurses.h>
#include <form.h>

#include "row.hpp"

class Prompt {
public:
  enum Type {TAB, ENTER};
  Prompt(WINDOW* window);
  ~Prompt();
  void amountPrompt(float amount, Row row, std::string hint = "");
  void splitPrompt(Row row);
  Type response(std::string& value);
  void writeField(std::string contents);
private:
  WINDOW* window;
  WINDOW* fieldWindow;
  FIELD* fields[2];
  FORM* form;
  int fieldPosition = 0;
  bool showHint;
  void debitPrompt(Row row);
  void creditPrompt(Row row);
  void draw(Row row, std::string message, bool numericInput);
};

#endif
