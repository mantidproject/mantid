// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "InvalidDefaultsError.h"

namespace MantidQt {
namespace CustomInterfaces {

InvalidDefaultsError::InvalidDefaultsError( // cppcheck-suppress passedByValue
    int row, std::vector<int> invalidColumns)
    : m_invalidColumns(std::move(invalidColumns)), m_row(row) {}

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
