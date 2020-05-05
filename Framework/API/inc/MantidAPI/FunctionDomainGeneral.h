// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FunctionDomain.h"
#include <memory>
#include <vector>

namespace Mantid {
namespace API {

class Column;

/**
    Represent a domain of a very general type.
*/
class MANTID_API_DLL FunctionDomainGeneral : public FunctionDomain {
public:
  /// Return the number of arguments in the domain
  size_t size() const override;
  /// Get the number of columns
  size_t columnCount() const;
  /// Add a new column. All columns must have the same size.
  void addColumn(const std::shared_ptr<Column> &column);
  /// Get i-th column
  std::shared_ptr<Column> getColumn(size_t i) const;

private:
  /// Columns containing function arguments
  std::vector<std::shared_ptr<Column>> m_columns;
};

} // namespace API
} // namespace Mantid
