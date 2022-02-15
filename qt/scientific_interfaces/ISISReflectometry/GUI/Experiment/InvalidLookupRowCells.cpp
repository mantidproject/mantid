// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InvalidLookupRowCells.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

InvalidLookupRowCells::InvalidLookupRowCells(int row, std::vector<int> invalidColumns)
    : m_invalidColumns(std::move(invalidColumns)), m_row(row) {}

std::vector<int> const &InvalidLookupRowCells::invalidColumns() const { return m_invalidColumns; }

int InvalidLookupRowCells::row() const { return m_row; }

bool operator==(InvalidLookupRowCells const &lhs, InvalidLookupRowCells const &rhs) {
  return lhs.row() == rhs.row() && lhs.invalidColumns() == rhs.invalidColumns();
}

bool operator!=(InvalidLookupRowCells const &lhs, InvalidLookupRowCells const &rhs) { return !(lhs == rhs); }
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
