#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include <ncurses.h>

#include "toml.hpp"
#include "statement_importer.hpp"
#include "table.hpp"
#include "table_view_array.hpp"
#include "prompt.hpp"
#include "input.hpp"
#include "formatter.hpp"

int main(int argc, char* argv[]) {
  if (argc == 1) {
    std::cout << "Usage: reconcile csv_file1 [csv_file2 ...]\n";
    exit(0);
  }

#ifdef DEBUG
  std::filesystem::path configFile{std::filesystem::current_path() / CONF};

  if (!std::filesystem::exists(configFile)) {
    std::cerr << "Error: Could not find config file at path ";
    std::cerr << configFile << '\n';
    return 1;
  }
#else
  // Define config file and transaction map file paths
  std::filesystem::path configFile;
  if (char const* home = std::getenv("HOME")) {
    configFile = std::filesystem::path{home} / CONF;
  } else {
    std::cerr << "Error: User's $HOME environment variable is not set\n";
    return 1;
  }

  // If no config file exists at the expected path, attempt to copy the sample
  // config file that was installed along with the program
  if (!std::filesystem::exists(configFile)) {
    std::filesystem::path sampleConfigFile{SAMPLE_CONF};
    std::cerr << "Info: Could not find config file at path " << configFile;
    std::cerr << ", attempting to copy sample config file from ";
    std::cerr << sampleConfigFile << '\n';
    if (std::filesystem::exists(sampleConfigFile)) {
      if (!std::filesystem::exists(configFile.parent_path())) {
	try {
	  std::filesystem::create_directories(configFile.parent_path());
	} catch (std::filesystem::filesystem_error const& e) {
	  std::cerr << "Error: Unable to create config file parent directory - ";
	  std::cerr << e.what() << '\n';
	  return 1;
	}
      }

      try {
	std::filesystem::copy_file(sampleConfigFile, configFile);
      } catch (std::filesystem::filesystem_error const& e) {
	std::cerr << "Error: Unable to copy sample config file - ";
	std::cerr << e.what() << '\n';
	return 1;
      }
    } else {
      std::cerr << "Error: Could not find sample config file at path ";
      std::cerr << sampleConfigFile << '\n';
      return 1;
    }
  }
#endif

  toml::table const config = toml::parse_file(configFile.string());
  std::string dateFormat = config["date_format"].value_or("");
  StatementImporter importer{config};

  std::vector<Table> tables;
  for (int i = 1; i < argc; i++) {
    std::string statement{argv[i]};
    Descriptor descriptor = importer.descriptor(statement);
    tables.push_back(Table{statement, dateFormat, descriptor});
  }

  // Configure ncurses
  initscr();
  start_color();
  init_pair(1, COLOR_BLACK, COLOR_WHITE); // Focused row cursor colour pair
  init_pair(2, COLOR_BLACK, 8); // Unfocused row cursor colour pair
  init_pair(3, 8, COLOR_BLACK); // Hint colour pair
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
  keypad(promptContent, TRUE);

  TableViewArray tableViewArray{tables, tableContent};
  Prompt prompt{promptContent};

  std::string accountsFile = config["ledger_accounts"].value_or("");
  Input input{tableViewArray, prompt, accountsFile};

  input.evaluate();

  // Open the output file and append Ledger-formatted transactions from tables
  std::string outputFile{config["output"]["file"].value_or("")};
  std::ofstream ledgerOutput{outputFile, std::ios_base::app};
  ledgerOutput << Formatter{tables, *config["output"]["format"].as_table()};

  delwin(tableContent);
  delwin(promptBorder);
  delwin(promptContent);
  endwin();

  return 0;
}
