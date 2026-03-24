#include "table_view_array.hpp"

TableViewArray::TableViewArray(TableArray& tables, WINDOW* window) :
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
    prevScroll = UP;
    outstandingScrolls.push_back(false);
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

  // If we are reversing scroll directions, then we want to decrement the
  // cursors of all other TableViews so that they point to the next row to be
  // traversed should the change in scroll direction continue to be maintained
  if (prevScroll == DOWN) {
    for (int i : indices) {
      if (i != prevFocusedIndex && outstandingScrolls[i] && !cursorAtHead[i]) {
	tableViews[i].scrollUp(); // Should not throw an exception due to
				  // cursorAtHead guard
	if (tableViews[i].cursorIndex() == 1) cursorAtHead[i] = true;
	tableViews[i].draw();
      }
    }
  }

  prevScroll = UP;

  if (focusedIndex != prevFocusedIndex && !cursorAtHead[prevFocusedIndex]) {
    // Now that we have checked that there was no outstanding down-scroll for
    // the previously focused table, we can safely set the corresponding
    // outstanding scroll field. It is imperative that this be done before
    // up-scrolling the previously focused table otherwise it may be erronously
    // recorded as having no outstanding scrolls since it's cursor post-scroll
    // may point to the first row, despite the fact that focus is shifting to
    // another table (i.e., the first row remains untraversed)
    outstandingScrolls[prevFocusedIndex] = true;
  }

  try {
    tableViews[prevFocusedIndex].scrollUp();
  } catch (const std::out_of_range& e) {
    int count = 0;
    for (int i : indices) {
      // Qualifying whether or not the given table has an outstanding scroll
      // verifies that not only is its cursor pointing to its first row, but
      // that that first row has indeed been traversed
      if (cursorAtHead[i] && !outstandingScrolls[i]) count++;
    }
    if (count == tables.size()) {
      throw std::out_of_range("Attempting to scroll past end of view array");
    }
  }

  if (focusedIndex != prevFocusedIndex) {
    outstandingScrolls[focusedIndex] = false;

    // Focus on next closest table in reverse direction, applying the scroll
    // action if required
    tableViews[prevFocusedIndex].focus = false;
    tableViews[prevFocusedIndex].draw(); // Remember to redraw after unfocusing
    tableViews[focusedIndex].focus = true;
  }

  // If, at this point, the currently focused table's cursor is at the head of
  // the table then we know that we have indeed traversed the first row and
  // shouldn't expect to return to this table on future consecutive up-scroll's
  // (as would otherwise be the case when scrolling across tables, where the
  // previously focused table's cursor would be at the head but we wouldn't have
  // yet traversed the row). We check here instead of the else clause in the try
  // block to ensure the case where a scroll reversal and carry occurs is
  // caught
  if (tableViews[prevFocusedIndex].cursorIndex() == 1) {
    cursorAtHead[prevFocusedIndex] = true;
  }

  tableViews[focusedIndex].draw();
}

void TableViewArray::scrollDown() {
  int prevFocusedIndex = focusedIndex;
  // Look forward in currently focused table
  focusedIndex = forwardFocus();

  // If we are reversing scroll directions, then we want to increment the
  // cursors of all other TableViews so that they point to the next row to be
  // traversed should the change in scroll direction continue to be maintained
  if (prevScroll == UP) {
    for (int i : indices) {
      // Because a cursor cannot "underrun" the first row in a table the way it
      // can "overrun" the last row in a table, we cannot universally apply down
      // scrolls to other tables when a scroll reversal occurs. If the cursor
      // for a particular table points to its first row and there are no
      // outstanding scrolls for that table (indicating that its first row has
      // been traversed), then we refrain from incrementing it
      if (cursorAtHead[i] && !outstandingScrolls[i]) continue;

      if (i != prevFocusedIndex && outstandingScrolls[i] && cursorInBounds(i)) {
	tableViews[i].scrollDown(); // Should not throw an exception due to
				    // cursorInBounds guard
	cursorAtHead[i] = false;
	tableViews[i].draw();
      }
    }
  }

  prevScroll = DOWN;

  try {
    cursorAtHead[prevFocusedIndex] = false;
    tableViews[prevFocusedIndex].scrollDown(); // May throw an exception, so do
					       // last
  } catch (const std::out_of_range& e) {
    int count = 0;
    for (int i : indices) {
      if (tableViews[i].cursorIndex() == tables[i].length()) count++;
    }
    if (count == tables.size()) {
      tableViews[prevFocusedIndex].draw();
      throw std::out_of_range("Attempting to scroll past end of view array");
    }
    focusedIndex = forwardFocus();
  }

  if (focusedIndex != prevFocusedIndex) {
    // Now that we have checked that there was no outstanding up-scroll for the
    // previously focused table, we can safely set the corresponding outstanding
    // scroll fields
    outstandingScrolls[focusedIndex] = false;
    outstandingScrolls[prevFocusedIndex] = true;

    // Focus on next closest table in forward direction
    tableViews[prevFocusedIndex].focus = false;
    tableViews[prevFocusedIndex].draw(); // Remember to redraw after unfocusing
    tableViews[focusedIndex].focus = true;
  }

  tableViews[focusedIndex].draw();
}

Table& TableViewArray::focusedTable() { return tables[focusedIndex]; }

TableView& TableViewArray::focusedTableView() {
  return tableViews[focusedIndex];
}

void TableViewArray::redrawFocusedView() { 
  tableViews[focusedIndex].refresh();
  tableViews[focusedIndex].draw();
}

// TODO: sort first by date and then by table view order (e.g., if all
// forward/reverse dates across all tables are equal, then prioritise focusing
// on table in left-to-right/right-to-left order)
int TableViewArray::forwardFocus() {
  // Determine which row, accross all tables, has the closest transaction date
  auto compare = [this](int a, int b) {
    Table const& tableA = tables[a];
    Table const& tableB = tables[b];
    // Ignore tables with their cursors at their tail by forcing them to the top
    // of the sort/min_element search
    if (tableViews[a].cursorIndex() >= tableA.length()) {
      return false;
    } else if (tableViews[b].cursorIndex() >= tableB.length()) {
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
    if (cursorAtHead[a] && !outstandingScrolls[a]) {
      return true;
    } else if (cursorAtHead[b] && !outstandingScrolls[b]) {
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
  TableView const& tableView = tableViews[tableIndex];
  bool outstandingUpScroll = outstandingScrolls[tableIndex] && prevScroll == UP;
  if (focusedIndex == tableIndex && cursorInBounds(tableIndex)) {
    // If the table is currently focused, return the next date
    return table.getDate(table.cbegin() + tableView.cursorIndex() + 1);
  } else if (focusedIndex != tableIndex && outstandingUpScroll) {
    // Otherwise, if the table is unfocused AND the last scroll was in the
    // opposite direction AND there's an outstanding scroll for the table,
    // return the next date
    return table.getDate(table.cbegin() + tableView.cursorIndex() + 1);
  } else {
    // Otherwise, return the date currently pointed to by the cursor
    return table.getDate(table.cbegin() + tableView.cursorIndex());
  }
};

std::chrono::year_month_day TableViewArray::reverseDate(int tableIndex) const {
  Table const& table = tables[tableIndex];
  TableView const& tableView = tableViews[tableIndex];
  if (focusedIndex == tableIndex && tableView.cursorIndex() != 1) {
    // If the table is currently focused AND the cursor is not at the
    // front, return the previous date
    return table.getDate(table.cbegin() + tableView.cursorIndex() - 1);
  } else if (tableView.cursorIndex() == table.length()) {
    // Otherwise, if the cursor is at the back (out-of-range), return the
    // previous date
    return table.getDate(table.cbegin() + tableView.cursorIndex() - 1);
  } else if (prevScroll == DOWN && tableView.cursorIndex() != 1) {
    // Otherwise, if the table is unfocused (implied) AND the last scroll was in
    // the opposite direction AND the cursor is not at the front, return the
    // previous date
    return table.getDate(table.cbegin() + tableView.cursorIndex() - 1);
  } else {
    // Otherwise, return the date currently pointed to by the cursor
    return table.getDate(table.cbegin() + tableView.cursorIndex());
  }
};

bool TableViewArray::cursorInBounds(int tableIndex) const {
  auto cursorIndex = tableViews[tableIndex].cursorIndex();
  auto length = tables[tableIndex].length();
  return cursorIndex < (length - 1);
}
