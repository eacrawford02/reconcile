#include <iostream>
#include <vector>

#include <ncurses.h>

#include "toml.hpp"
#include "statement_importer.hpp"
#include "table.hpp"
#include "table_view_array.hpp"
#include "prompt.hpp"
#include "input.hpp"

int main(int argc, char* argv[]) {
  if (argc == 1) {
    std::cout << "Usage: reconcile csv_file1 [csv_file2 ...]\n";
    exit(0);
  }

  toml::table const config = toml::parse_file("sample_config.toml");
  StatementImporter importer{config};

  std::vector<Table> tables;
  for (int i = 1; i < argc; i++) {
    std::string statement{argv[i]};
    Descriptor descriptor = importer.descriptor(statement);
    tables.push_back(Table{statement, descriptor});
  }

  // Configure ncurses
  initscr();
  start_color();
  init_pair(1, COLOR_BLACK, COLOR_WHITE);
  init_pair(2, COLOR_BLACK, 8);
  cbreak();
  keypad(stdscr, TRUE);
  noecho();
  refresh();

  // Allocate sceen space for main application windows
  //int height = LINES;
  //int width = COLS;
  //getmaxyx(stdscr, height, width);
  const int promptHeight = 7;
  const int tableHeight = LINES - promptHeight;
  //const int commandY = height - commandHeight;

  // Create windows
  WINDOW* tableContent = newwin(tableHeight, COLS, 0, 0);
  WINDOW* promptBorder = newwin(promptHeight, COLS, tableHeight, 0);
  WINDOW* promptContent = derwin(promptBorder, promptHeight - 2, COLS - 2,
      1, 1);

  box(promptBorder, 0, 0);
  wrefresh(promptBorder);

  TableViewArray tableViewArray{tables, tableContent};
  Prompt prompt{promptContent};
  Input input{tableViewArray, prompt};

  input.process();

  delwin(tableContent);
  delwin(promptBorder);
  delwin(promptContent);
  endwin();

  return 0;
}
