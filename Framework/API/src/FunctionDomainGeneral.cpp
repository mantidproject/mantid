// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FunctionDomainGeneral.h"
#include "MantidAPI/Column.h"

namespace Mantid::API {

/// Return the number of arguments in the domain
size_t FunctionDomainGeneral::size() const { return m_columns.empty() ? 0 : m_columns.front()->size(); }

/// Get the number of columns
size_t FunctionDomainGeneral::columnCount() const { return m_columns.size(); }

/// Add a new column. All columns must have the same size.
void FunctionDomainGeneral::addColumn(const std::shared_ptr<Column> &column) {
  if (!column) {
    throw std::runtime_error("Cannot add null column to FunctionDomainGeneral.");
  }
  if (!m_columns.empty() && size() != column->size()) {
    throw std::runtime_error("Cannot add a column to FunctionDomainGeneral. "
                             "All columns must have the same size.");
  }

  m_columns.emplace_back(column);
}

/// Get i-th column.
/// @param i :: Index of a column to get.
std::shared_ptr<Column> FunctionDomainGeneral::getColumn(size_t i) const { return m_columns.at(i); }

} // namespace Mantid::API
