#include <iostream>

#include <ncurses.h>

#include "toml.hpp"
#include "statement_importer.hpp"
#include "table.hpp"

int main(int argc, char* argv[]) {
  if (argc == 1) {
    std::cout << "Usage: reconcile csv_file1 [csv_file2 ...]\n";
    exit(0);
  }

  toml::table const config = toml::parse_file("sample_config.toml");
  StatementImporter importer{config};

  for (int i = 1; i < argc; i++) {
    std::string statement{argv[i]};
    Descriptor descriptor = importer.descriptor(statement);
    Table table{statement, descriptor};
  }

  return 0;
}
