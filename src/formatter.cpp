#include "formatter.hpp"

Formatter::Formatter(std::vector<Table>& tables, toml::table const& format) :
    tables{tables} {
  locale = format["locale"].value_or("");
  // In this case we use direct-initialization (i.e., ()) instead of
  // list-initialization (i.e., {}) to avoid calling std::basic_string's
  // std::initializer_list constructor, which would treat both arguments as
  // characters to construct the string with. This is not the constructor we
  // want to use, and has the added downside of creating a string with (likely)
  // a non-printable ASCII character due to the first argument being an integer
  indentation = std::string(format["indentation"].value_or(4), ' ');
  margin = std::string(format["margin "].value_or(8), ' ');
}

std::ostream& operator<<(std::ostream& out, Formatter const& formatter) {
  std::vector<Table>& tables = formatter.tables;

  // Determine the column against which amounts should be aligned in the ledger
  // output based off of the widest source/destination string across all tables
  int amountAlignment = 0;
  for (auto& table : tables) {
    int sourceWidth = table.getSource().size();
    int destinationWidth = table.displayWidths().back();
    amountAlignment = std::max({amountAlignment, sourceWidth,
	destinationWidth});
  }

  // Reset each table's cursor to the start
  for (auto& table : tables) table.cursor = table.begin();

  // Start tracking the number of tables whose cursors have not reached their
  // end
  int remaining = tables.size();

  auto compare = [](Table const& a, Table const& b) {
    if (a.cursor == a.cend()) {
      return false;
    } else if (b.cursor == b.cend()) {
      return true;
    } else {
      return a.getDate() < b.getDate();
    }
  };

  // Step through all rows across all tables and write formatted output to file
  // until each table has reached its end
  while (remaining > 0) {
    // Determine, accross all tables, which cursor points to the row with the
    // smallest (i.e., earliest) date
    std::vector<Table>::iterator table;
    table = std::min_element(tables.begin(), tables.end(), compare);
    formatter.formatRow(out, *table, amountAlignment);

    // Each table will only hit this condition once since the following cursor
    // increment will put the table's cursor at its end and thus the table
    // will never be returned again from the call to min_element (the
    // comparison function will always return the complementary table if one's
    // cursor is at its end). This makes sure that the remaining count is
    // never double-decremented
    if (table->cursor == (table->cend() - 1)) remaining--;

    if (remaining > 0) out << '\n';
    (table->cursor)++;
  }
  return out;
}

void Formatter::formatRow(std::ostream& out, Table& table, int amountAlignment)
    const {
  out.imbue(std::locale(locale));
  std::string destination = table.getDestination();
  float amount;
  out << std::showbase; // Show dollar sign when reporting amount
  out << table.getDate() << ' ';
  if (!destination.empty()) {
    std::string destinationPadding;
    destinationPadding.append(amountAlignment - destination.size(), ' ');
    // TODO: amounts stored by table should really be denominated in cents and
    // stored in integers
    amount = table.getAmount() * 100;
    out << "* ";
    out << table.getPayee() << '\n';
    out << indentation << destination << destinationPadding;
    out << margin  << std::put_money(amount) << '\n';
    out << indentation << table.getSource();
  } else {
    std::string sourcePadding;
    sourcePadding.append(amountAlignment - table.getSource().size(), ' ');
    amount = -table.getAmount() * 100;
    out << "! ";
    out << table.getPayee() << '\n';
    out << indentation << table.getSource() << sourcePadding;
    out << margin  << std::put_money(amount);
  }
  out << '\n';
}
