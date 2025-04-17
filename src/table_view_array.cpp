#include "table_view_array.hpp"

TableViewArray::TableViewArray(std::vector<Table>& tables, WINDOW* window) :
    tables{tables} {
  // Allocate screen space for each TableView
  int height;
  int width;
  getmaxyx(window, height, width);
  int subWidth = (int) width / tables.size();

  for (int i = 0; i < tables.size(); i++) {
    WINDOW* border = derwin(window, height, subWidth, 0, subWidth * i);
    WINDOW* content = derwin(border, height - 2, subWidth - 2, 1, 1);
    TableView tableView{tables[i], content};

    box(border, 0, 0); // Use default border characters
    wrefresh(border);
    tableView.draw();
    
    borders.push_back(border);
    contents.push_back(content);
    tableViews.push_back(tableView);

    indices.push_back(i);
  }

  // Focus on a table
  focusedIndex = forwardFocus();
  tableViews[focusedIndex].focus = true;
  tableViews[focusedIndex].draw();
}

TableViewArray::~TableViewArray() {
  for (auto content : contents) delwin(content);
  for (auto border : borders) delwin(border);
}

void TableViewArray::scrollUp() {
  tableViews[focusedIndex].focus = false;
  tableViews[focusedIndex].draw(); // Remember to redraw after unfocusing

  int prevFocusedIndex = focusedIndex;
  focusedIndex = reverseFocus();

  // If we attempt to scroll up when the focused table's cursor is already
  // pointing to the first element, the effect of that scroll action on the
  // TableViewArray is lost since no table will be scrolled. We still want the
  // scroll action to carry over to the table with the closest previous date, so
  // we must track this occurrence
  bool carryScroll = false;

  try {
    // If we are scrolling accross tables (as opposed to scrolling down in the
    // same table), we don't want the cursor of the next focused table to
    // change. Instead, we want to advance the cursor of the table we are
    // scrolling from
    if (focusedIndex != prevFocusedIndex) {
      tableViews[prevFocusedIndex].scrollUp();
    } else {
      tableViews[focusedIndex].scrollUp();
    }
  } catch (const std::out_of_range& e) {
    int count = 0;
    for (auto& table : tables) if (table.cursor == table.cbegin()) count++;
    if (count == tables.size()) {
      throw std::out_of_range("Attempting to scroll past end of view array");
    }
    carryScroll = true;
  }

  // Focus on next closest table in reverse direction, applying the scroll
  // action if required
  tableViews[focusedIndex].focus = true;
  if (carryScroll) tableViews[focusedIndex].scrollUp();
  tableViews[focusedIndex].draw();
}

void TableViewArray::scrollDown() {
  tableViews[focusedIndex].focus = false;
  tableViews[focusedIndex].draw(); // Remember to redraw after unfocusing

  int prevFocusedIndex = focusedIndex;
  // TODO: replace look forward with getNextDate usage in forwardFocus
  // Look forward in currently focused table
  tables[focusedIndex].cursor++;
  focusedIndex = forwardFocus();
  tables[prevFocusedIndex].cursor--;

  try {
    // If we are scrolling accross tables (as opposed to scrolling down in the
    // same table), we don't want the cursor of the next focused table to
    // change. Instead, we want to advance the cursor of the table we are
    // scrolling from. Of course, this should only be done if the table we are
    // scrolling from has not already reached its end (otherwise we will attempt
    // to scroll out of bounds)
    Table& prevTable = tables[prevFocusedIndex];
    bool outOfBounds = prevTable.cursor == prevTable.cend();
    if (focusedIndex != prevFocusedIndex && !outOfBounds) {
      tableViews[prevFocusedIndex].scrollDown();
      // The below draw call will not be reached if the previously focused
      // table's cursor is at its last element (TableView::scrollDown will throw
      // an exception). To cover this case, remember to call TableView::draw in
      // the exception handler
      tableViews[prevFocusedIndex].draw();
    } else {
      tableViews[focusedIndex].scrollDown();
    }
  } catch (const std::out_of_range& e) {
    int count = 0;
    for (auto& table : tables) if (table.cursor == table.cend()) count++;
    if (count == tables.size()) {
      throw std::out_of_range("Attempting to scroll past end of view array");
    }
    tableViews[prevFocusedIndex].draw();
  }

  // Focus on next closest table in forward direction
  tableViews[focusedIndex].focus = true;
  tableViews[focusedIndex].draw();
}

Table& TableViewArray::focusedTable() { return tables[focusedIndex]; }

int TableViewArray::forwardFocus() {
  // Determine which row, accross all tables, has the closest transaction date
  auto compare = [](Table const& a, Table const& b) {
    if (a.cursor == a.cend()) {
      return false;
    } else if (b.cursor == b.cend()) {
      return true;
    } else {
      return a.getDate() < b.getDate();
    }
  };
  std::vector<Table>::iterator nextTable;
  nextTable = std::min_element(tables.begin(), tables.end(), compare);
  return std::distance(tables.begin(), nextTable);
}

int TableViewArray::reverseFocus() {
  // Determine which row, accross all tables, has the closest transaction date
  auto compare = [this](int a, int b) {
    Table const& tableA = tables[a];
    Table const& tableB = tables[b];
    if (tableA.cursor == tableA.cbegin()) {
      return true;
    } else if (tableB.cursor == tableB.cbegin()) {
      return false;
    } else {
      // Recall that a table's cursor will point to the same index as cend when
      // it has been scrolled to its bottom (due to the trailing scroll
      // associated with TableView::ScrollDown()). If that's the case,
      // attempting to access the date of a scrolled-through table using
      // Table::getDate() will be an out-of-bounds access. We instead need to
      // reference that table's previous row's date
      // TODO: clean up
      //auto dateA = a.cursor == a.cend() ? a.getPrevDate() : a.getDate();
      //auto dateB = b.cursor == b.cend() ? b.getPrevDate() : b.getDate();
      bool dateALookBehind = (focusedIndex == a && tableA.cursor != tableA.cbegin()) || tableA.cursor == tableA.cend();
      auto dateA = dateALookBehind ? tableA.getPrevDate() : tableA.getDate();
      bool dateBLookBehind = (focusedIndex == b && tableB.cursor != tableB.cbegin()) || tableB.cursor == tableB.cend();
      auto dateB = dateBLookBehind ? tableB.getPrevDate() : tableB.getDate();
      return dateA < dateB;
    }
  };
  std::vector<int>::iterator nextTable;
  nextTable = std::max_element(indices.begin(), indices.end(), compare);
  return std::distance(indices.begin(), nextTable);
}
