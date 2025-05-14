#include "autocomplete.hpp"

Autocomplete::Autocomplete(std::string accounts) {
  // Open accounts file
  std::ifstream inputStream{accounts};
  if (!inputStream.is_open()) {
    throw std::runtime_error("Error: Could not open file " + accounts);
  }

  // Step through each line of the file and parse account names
  std::string line;
  while (std::getline(inputStream, line)) {
    std::smatch matches;
    std::regex expression{"^account (.*)", std::regex::extended};

    if (std::regex_search(line, matches, expression)) {
      // Insert into radix trie
      insert(matches[1]);
    }
  }
}

std::string Autocomplete::complete(std::string partial) {
  std::string completed = partial;

  int charactersFound;
  Node* node = nullptr;
  bool completeMatch = search(partial, node, charactersFound);
  std::string remaining = partial.substr(charactersFound);

  // Determine which child node under the closest partially matching parent node
  // to traverse from
  if (!completeMatch) {
    for (Node& child : node->children) {
      if (child.value.starts_with(remaining)) {
	// Don't add any overlapping characters between the remaining string and
	// the child value to the completion string (they already exist in the
	// completion string since it is seeded with the partial string)
	completed += child.value.substr(remaining.size());
	node = &child;
	break;
      }
    }
  }

  // Follow the chain of single child nodes until we hit a fork, adding each
  // single child node's value along the way
  while (node->numberOfChildren == 1) {
    node = &(node->children.front());
    completed += node->value;
  }

  return completed;
}

bool Autocomplete::search(std::string value, Node*& result, int&
    charactersFound) {
  Node* currentNode = &root;
  charactersFound = 0;

  while (currentNode != nullptr && charactersFound < value.size()) {
    Node* next = nullptr;

    // Determine the next node to traverse by evaluating which child node's
    // value forms the prefix of the sub-string of value not yet found in the
    // trie
    for (Node& child : currentNode->children) {
      std::string remaining = value.substr(charactersFound);
      
      if (remaining.starts_with(child.value)) next = &child;
    }

    if (next != nullptr) {
      charactersFound += next->value.size();
    } else { // No further matches for the remaining unfound characters
      break; // Ensures we don't assign currentNode to next and return a nullptr
    }
    currentNode = next;
  }

  value = value.substr(charactersFound);
  result = currentNode;
  return charactersFound == value.size();
}

void Autocomplete::insert(std::string value) {
  int charactersFound;
  Node* insertUnder = nullptr;
  search(value, insertUnder, charactersFound);
  std::string remaining = value.substr(charactersFound);
  int overlap = 0;

  for (Node& child : insertUnder->children) {
    if (remaining[0] != child.value[0]) continue;
    insertUnder = &child;
    overlap++;

    // Use the minimum between the remaining string size and the child node's
    // string size as the range of the for loop
    int remainingSize = remaining.size();
    int childSize = child.value.size();
    int range = remainingSize < childSize ? remainingSize : childSize;

    // Count the number of consecutive characters that are common between the
    // two strings, starting at the front
    for (int i = overlap; i < range; i++) {
      if (remaining[i] == child.value[i]) {
	overlap++;
      } else {
	break;
      }
    }

    // The first character of each child node's string is guaranteed to be
    // unique from all other child nodes (this first character is the radix of
    // the trie). If we have made it this far in the loop (i.e., we have matched
    // the first character of this particular node's string with that of the
    // remaining insertion string), then we can safely disregard all other child
    // nodes and break out of the loop
    break;
  }

  if (overlap > 0) {
    // Split the node we're inserting under
    std::string originalValue = insertUnder->value;
    insertUnder->value = originalValue.substr(0, overlap);

    Node suffixNode = {
      .value = originalValue.substr(overlap),
      .children = std::move(insertUnder->children),
      .numberOfChildren = insertUnder->numberOfChildren
    };

    insertUnder->children.push_front(std::move(suffixNode));
    insertUnder->numberOfChildren = 1;
  }

  // Insert the remaining string, less any overlap, under the node to be
  // inserted under. In the event where the node we're inserting under has been
  // split, make sure we only insert a new node if the remaining string to be
  // inserted is not equal to the newly-split prefix node's string; otherwise,
  // the remaining string is already "present" in the trie (this would occur
  // when inserting e.g., "tea" after "team")
  if (overlap < remaining.size()) {
    Node newNode = {
      .value = remaining.substr(overlap)
    };

    insertUnder->children.push_front(std::move(newNode));
    insertUnder->numberOfChildren++;
  }
}
