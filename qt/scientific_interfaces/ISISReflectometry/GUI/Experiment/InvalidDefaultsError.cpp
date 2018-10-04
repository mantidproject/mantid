#include "InvalidDefaultsError.h"

namespace MantidQt {
namespace CustomInterfaces {

InvalidDefaultsError::InvalidDefaultsError(int row,
                                           std::vector<int> invalidColumns)
    : m_invalidColumns(invalidColumns), m_row(row) {}

std::vector<int> const &InvalidDefaultsError::invalidColumns() const {
  return m_invalidColumns;
}

int InvalidDefaultsError::row() const { return m_row; }

bool operator==(InvalidDefaultsError const &lhs,
                InvalidDefaultsError const &rhs) {
  return lhs.row() == rhs.row() && lhs.invalidColumns() == rhs.invalidColumns();
}

bool operator!=(InvalidDefaultsError const &lhs,
                InvalidDefaultsError const &rhs) {
  return !(lhs == rhs);
}
} // namespace CustomInterfaces
} // namespace MantidQt
