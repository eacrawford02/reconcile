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
  if (focusedIndex != prevFocusedIndex) {
    tableViews[prevFocusedIndex].focus = false;
    tableViews[prevFocusedIndex].draw(); // Remember to redraw after unfocusing
    tableViews[focusedIndex].focus = true;
  }
  if (carryScroll) tableViews[focusedIndex].scrollUp();
  tableViews[focusedIndex].draw();
}

void TableViewArray::scrollDown() {
  int prevFocusedIndex = focusedIndex;
  // Look forward in currently focused table
  focusedIndex = forwardFocus();

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
  if (focusedIndex != prevFocusedIndex) {
    tableViews[prevFocusedIndex].focus = false;
    tableViews[prevFocusedIndex].draw(); // Remember to redraw after unfocusing
    tableViews[focusedIndex].focus = true;
  }
  tableViews[focusedIndex].draw();
}

Table& TableViewArray::focusedTable() { return tables[focusedIndex]; }

void TableViewArray::redrawFocusedView() { 
  tableViews[focusedIndex].refresh();
  tableViews[focusedIndex].draw();
}

int TableViewArray::forwardFocus() {
  // Determine which row, accross all tables, has the closest transaction date
  auto compare = [this](int a, int b) {
    Table const& tableA = tables[a];
    Table const& tableB = tables[b];
    if (tableA.cursor >= (tableA.cend() - 1)) {
      return false;
    } else if (tableB.cursor >= (tableB.cend() - 1)) {
      return true;
    } else {
      auto dateA = focusedIndex == a ? tableA.getNextDate() : tableA.getDate();
      auto dateB = focusedIndex == b ? tableB.getNextDate() : tableB.getDate();
      return dateA < dateB;
    }
  };
  std::vector<int>::iterator nextTable;
  // We use the min_element function because the smallest (i.e., earliest)
  // forward-looking date will be closest to the currently focused date
  nextTable = std::min_element(indices.begin(), indices.end(), compare);
  return std::distance(indices.begin(), nextTable);
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
      return reverseDate(a) < reverseDate(b);
    }
  };
  std::vector<int>::iterator nextTable;
  // We use the max_element function because the greatest (i.e., latest)
  // backward-looking date will be closest to the currently focused date
  nextTable = std::max_element(indices.begin(), indices.end(), compare);
  return std::distance(indices.begin(), nextTable);
}

std::chrono::year_month_day TableViewArray::reverseDate(int tableIndex) const {
  Table const& table = tables[tableIndex];
  if (focusedIndex == tableIndex && table.cursor != table.cbegin()) {
    // If the table is currently focused AND the cursor is not at the
    // front, return the previous date
    return table.getPrevDate();
  } else if (table.cursor == table.cend()) {
    // Otherwise, if the cursor is at the back (out-of-range), return the
    // previous date
    return table.getPrevDate();
  } else {
    // Otherwise, return the date currently pointed to by the cursor
    return table.getDate();
  }
};
