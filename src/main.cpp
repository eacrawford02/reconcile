#include <iostream>

#include <ncurses.h>

#include "toml.hpp"
#include "statement_importer.hpp"

int main(int argc, char* argv[]) {
  if (argc == 1) {
    std::cout << "Usage: reconcile csv_file1 [csv_file2 ...]\n";
    exit(0);
  }

  toml::table const config = toml::parse_file("sample_config.toml");
  StatementImporter importer{config};

  for (int i = 1; i < argc; i++) {
    Descriptor d = importer.descriptor(std::string{argv[i]});
  }

  return 0;
}
