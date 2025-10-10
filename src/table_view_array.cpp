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
    cursorAtHead.push_back(true);
    lastTableScrolls.push_back(DOWN);
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
    // same table), we don't want the cursor of the next focused table to change
    // (unless we are reversing scroll directions, in which case we do).
    // Instead, we want to advance the cursor of the table we are scrolling from
    if (focusedIndex != prevFocusedIndex) {
      if (lastTableScrolls[focusedIndex] == DOWN) {
	// Set carryScroll to true instead of explicitly calling
	// tableViews[focusedIndex].scrollUp() to avoid scrolling up twice (line
	// 82)
        carryScroll = true;
      }
      tableViews[prevFocusedIndex].scrollUp();
      lastTableScrolls[prevFocusedIndex] = UP;
    } else {
      tableViews[focusedIndex].scrollUp();
      lastTableScrolls[focusedIndex] = UP;
    }
  } catch (const std::out_of_range& e) {
    int count = 0;
    for (auto& i : indices) if (cursorAtHead[i]) count++;
    if (count == tables.size()) {
      throw std::out_of_range("Attempting to scroll past end of view array");
    }

    // If two consecutive up-scrolls occur and the first involved both scrolling
    // across tables and was a scroll reversal, the previously focused table's
    // cursor from the first up-scroll may already be at the head of the table
    // (albeit untraversed). In that case we don't want to carry the scroll over
    // in the second up-scroll and instead just want to refocus
    if (tables[focusedIndex].cursor != tables[focusedIndex].cbegin()) {
      carryScroll = true;
    }
  }

  // Focus on next closest table in reverse direction, applying the scroll
  // action if required
  if (focusedIndex != prevFocusedIndex) {
    tableViews[prevFocusedIndex].focus = false;
    tableViews[prevFocusedIndex].draw(); // Remember to redraw after unfocusing
    tableViews[focusedIndex].focus = true;
  }
  if (carryScroll) {
    tableViews[focusedIndex].scrollUp();
    lastTableScrolls[focusedIndex] = UP;
  }

  // If, at this point, the currently focused table's cursor is at the head of
  // the table then we know that we have indeed traversed the first row and
  // shouldn't expect to return to this table on future consecutive up-scroll's
  // (as would otherwise be the case when scrolling across tables, where the
  // previously focused table's cursor would be at the head but we wouldn't have
  // yet traversed the row). We check here instead of the else clause in the try
  // block to ensure the case where a scroll reversal and carry occurs is
  // caught
  if (tables[focusedIndex].cursor == tables[focusedIndex].cbegin()) {
    cursorAtHead[focusedIndex] = true;
  }

  tableViews[focusedIndex].draw();
}

void TableViewArray::scrollDown() {
  int prevFocusedIndex = focusedIndex;
  // Look forward in currently focused table
  focusedIndex = forwardFocus();

  try {
    // If we are scrolling across tables (as opposed to scrolling down in the
    // same table), we don't want the cursor of the next focused table to change
    // (unless we are reversing scroll directions, in which case we do).
    // Instead, we want to advance the cursor of the table we are scrolling
    // from. Of course, this should only be done if the table we are scrolling
    // from has not already reached its end (otherwise we will attempt to scroll
    // out of bounds)
    Table& prevTable = tables[prevFocusedIndex];
    bool outOfBounds = prevTable.cursor == prevTable.cend();
    if (focusedIndex != prevFocusedIndex && !outOfBounds) {
      if (lastTableScrolls[focusedIndex] == UP) {
        tableViews[focusedIndex].scrollDown();
      }
      tableViews[prevFocusedIndex].scrollDown();
      cursorAtHead[prevFocusedIndex] = false;
      lastTableScrolls[prevFocusedIndex] = DOWN;
      // The below draw call will not be reached if the previously focused
      // table's cursor is at its last element (TableView::scrollDown will throw
      // an exception). To cover this case, remember to call TableView::draw in
      // the exception handler
      tableViews[prevFocusedIndex].draw();
    } else {
      tableViews[focusedIndex].scrollDown();
      cursorAtHead[focusedIndex] = false;
      lastTableScrolls[focusedIndex] = DOWN;
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
    // Ignore tables with their cursors at their tail by forcing them to the top
    // of the sort/min_element search
    if (tableA.cursor >= (tableA.cend() - 1)) {
      return false;
    } else if (tableB.cursor >= (tableB.cend() - 1)) {
      return true;
    } else {
      // If a particular table had last been subjected to a cross-table scroll
      // (i.e., the table is no longer focused), we always want to compare the
      // next date following its cursor because the previously focused table
      // will always have its cursor incremented in a cross-table scroll and
      // thus the row indexed by the cursor won't have yet been visited (since
      // focus will have been shifted to the other table in the cross-table
      // scroll). Additionally, if a scroll reversal is now occurring on the
      // focused table, then we also want to inspect the next date
      return forwardDate(a) < forwardDate(b);
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
    // Ignore tables with their cursors at their head by forcing them to the
    // bottom of the sort/max_element search
    if (cursorAtHead[a]) {
      return true;
    } else if (cursorAtHead[b]) {
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

std::chrono::year_month_day TableViewArray::forwardDate(int tableIndex) const {
  Table const& table = tables[tableIndex];
  if (focusedIndex == tableIndex) {
    // If the table is currently focused, return the next date
    return table.getNextDate();
  } else if (lastTableScrolls[tableIndex] == UP) {
    // Otherwise, if the table is unfocused (implied) AND the last scroll was in
    // the opposite direction, return the next date
    return table.getNextDate();
  } else {
    // Otherwise, return the date currently pointed to by the cursor
    return table.getDate();
  }
};

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
  } else if (lastTableScrolls[tableIndex] == DOWN &&
	     table.cursor != table.cbegin()) {
    // Otherwise, if the table is unfocused (implied) AND the last scroll was in
    // the opposite direction AND the cursor is not at the front, return the
    // previous date
    return table.getPrevDate();
  } else {
    // Otherwise, return the date currently pointed to by the cursor
    return table.getDate();
  }
};
