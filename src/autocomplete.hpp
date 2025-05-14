#ifndef AUTOCOMPLETE_H
#define AUTOCOMPLETE_H

#include <string>
#include <fstream>
#include <stdexcept>
#include <regex>
#include <forward_list>

class Autocomplete {
public:
  Autocomplete(std::string accounts);
  std::string complete(std::string partial);
private:
  struct Node {
    std::string value;
    std::forward_list<Node> children; // Radix is all printable ASCII characters
    int numberOfChildren = 0;
  };
  struct Node root;
  bool search(std::string value, Node*& result, int& charactersFound);
  void insert(std::string value);
};

#endif
