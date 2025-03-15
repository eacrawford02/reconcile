#ifndef PROMPT_H
#define PROMPT_H

#include <string>

#include <ncurses.h>

class Prompt {
public:
  Prompt(WINDOW* window);
  Prompt& operator<<(char c);
  void clear();
private:
};

#endif

