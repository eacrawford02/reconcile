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
    int sourceWidth = table.getAccount().size();
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

// TODO: Change name to formatTransaction(?) Add formatPosting function to
// format a row that's been split as a compository posting in a single
// transaction (will require tracking which rows in a table have been split)
void Formatter::formatRow(std::ostream& out, Table& table, int amountAlignment)
    const {
  out.imbue(std::locale(locale));
  out << std::showbase; // Show dollar sign when reporting amount
  out << table.getDate() << ' ';

  std::string padding;
  // TODO: amounts stored by table should really be denominated in cents and
  // stored in integers
  float amount = table.getAmount() * 100;

  // Format either a complete transaction (two postings) or an incomplete
  // transaction (one posting)
  if (!table.getCounterparty().empty()) {
    // Select the correct accounts to use for the positive-valued posting and
    // the elided posting that constitute the Ledger transaction. See section
    // 5.2 of Ledger manual for meaning of elision in a formatting context
    std::string positiveAccount; // The debtor in the transaction
    std::string elidedAccount; // The creditor in the transaction
    if (table.normalBalance() == Descriptor::DEBIT) {
      if (amount >= 0) { // DEBIT
	positiveAccount = table.getAccount();
	elidedAccount = table.getCounterparty();
      } else { // CREDIT
	amount = -amount;
	positiveAccount = table.getCounterparty();
	elidedAccount = table.getAccount();
      }
    } else { // table.normalBalance() == Descriptor::CREDIT
      if (amount >= 0) { // CREDIT
	positiveAccount = table.getCounterparty();
	elidedAccount = table.getAccount();
      } else { // DEBIT
	amount = -amount;
	positiveAccount = table.getAccount();
	elidedAccount = table.getCounterparty();
      }
    }

    padding.append(amountAlignment - positiveAccount.size(), ' ');
    out << "* ";
    out << table.getPayee() << '\n';
    out << indentation << positiveAccount << padding;
    out << margin << std::put_money(amount) << '\n';
    out << indentation << elidedAccount;
  } else {
    padding.append(amountAlignment - table.getAccount().size(), ' ');
    out << "! ";
    out << table.getPayee() << '\n';
    out << indentation << table.getAccount() << padding;
    out << margin  << std::put_money(amount);
  }
  out << '\n';
}
