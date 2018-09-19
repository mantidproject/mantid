#include "MantidQtWidgets/Common/Batch/Row.h"
#include "MantidQtWidgets/Common/Batch/AssertOrThrow.h"
#include <boost/algorithm/string/predicate.hpp>
namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

Row::Row(RowLocation location, std::vector<Cell> cells)
    : m_location(std::move(location)), m_cells(std::move(cells)) {}

RowLocation const &Row::location() const { return m_location; }

std::vector<Cell> const &Row::cells() const { return m_cells; }
std::vector<Cell> &Row::cells() { return m_cells; }

std::ostream &operator<<(std::ostream &os, Row const &row) {
  os << row.location() << " ";
  for (auto &&cell : row.cells())
    os << cell;
  return os;
}

bool operator==(Row const &lhs, Row const &rhs) {
  return lhs.location() == rhs.location() && lhs.cells() == rhs.cells();
}

bool operator!=(Row const &lhs, Row const &rhs) { return !(lhs == rhs); }

bool operator<(Row const &lhs, Row const &rhs) {
  return lhs.location() < rhs.location();
}

bool operator<=(Row const &lhs, Row const &rhs) {
  return lhs < rhs || lhs == rhs;
}

bool operator>=(Row const &lhs, Row const &rhs) { return !(lhs < rhs); }

bool operator>(Row const &lhs, Row const &rhs) { return !(lhs <= rhs); }
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
