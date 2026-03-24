#include "table_array.hpp"

Table& TableArray::operator[](int index) { return tables[index]; }

Table const& TableArray::operator[](int index) const { return tables[index]; }

// TODO: Implement
bool TableArray::operator<(TableArray const& other) {}

int TableArray::size() const { return tables.size(); }

TableArray::Iterator TableArray::begin() { return tables.begin(); }

TableArray::Iterator TableArray::end() { return tables.end(); }

TableArray::ConstIterator TableArray::cbegin() const { return tables.cbegin(); }

TableArray::ConstIterator TableArray::cend() const { return tables.cend(); }

void TableArray::push_back(Table const& value) {
  for (auto& table : tables) {
    if (value.identifier() == table.identifier()) {
      table += value;
      return;
    }
  }
  tables.push_back(value);
}
