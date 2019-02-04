// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_API_FUNCTIONDOMAINGENERAL_H_
#define MANTID_API_FUNCTIONDOMAINGENERAL_H_

#include "MantidAPI/FunctionDomain.h"
#include <boost/shared_ptr.hpp>
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
  void addColumn(boost::shared_ptr<Column> column);
  /// Get i-th column
  boost::shared_ptr<Column> getColumn(size_t i) const;

private:
  /// Columns containing function arguments
  std::vector<boost::shared_ptr<Column>> m_columns;
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_FUNCTIONDOMAINGENERAL_H_*/
