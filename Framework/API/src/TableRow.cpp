// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/TableRow.h"
#include "MantidAPI/ITableWorkspace.h"

namespace Mantid::API {

/**   Constructor
      @param trh :: TableRowHelper returned by TableWorkspace::getRow
  */
TableRow::TableRow(const TableRowHelper &trh) : m_row(trh.m_row), m_col(0), m_sep(",") {
  for (size_t i = 0; i < trh.m_workspace->columnCount(); i++)
    m_columns.emplace_back(trh.m_workspace->getColumn(i));
  if (!m_columns.empty())
    m_nrows = int(m_columns[0]->size());
  else
    m_nrows = 0;
}

/**  Makes the TableRow point to i-th row in the TableWorkspace
     @param i :: New row number
 */
void TableRow::row(size_t i) {
  if (i < m_nrows) {
    m_row = i;
    m_col = 0;
  } else {
    throw std::range_error("Row index out of range.");
  }
}

/**  Steps to the next row in the TableWorkspace if there is one
     @return true if the row changed and false if the TableRow is already at the
   end of the TableWorkspace
 */
bool TableRow::next() {
  if (m_row < m_nrows - 1) {
    ++m_row;
    m_col = 0;
    return true;
  }
  return false;
}

/**  Steps to the previous row in the TableWorkspace if there is one
     @return true if the row changed and false if the TableRow is already at the
   beginning of the TableWorkspace
 */
bool TableRow::prev() {
  if (m_row > 0) {
    --m_row;
    m_col = 0;
    return true;
  }
  return false;
}

/// Special case of bool
const TableRow &TableRow::operator>>(bool &t) const {
  Boolean b;
  operator>>(b);
  t = b;
  return *this;
}

/**  Output stream operator
     @param s :: Output stream
     @param row :: The TableRow
     @return stream representation of row
 */
std::ostream &operator<<(std::ostream &s, const TableRow &row) {
  if (row.m_columns.empty())
    return s;
  if (row.m_columns.size() == 1) {
    row.m_columns[0]->print(row.row(), s);
    return s;
  }
  auto ci = row.m_columns.cbegin();
  for (; ci != row.m_columns.cend() - 1; ++ci) {
    (*ci)->print(row.row(), s);
    s << row.m_sep;
  }
  (*ci)->print(row.row(), s);
  return s;
}

} // namespace Mantid::API
