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

    Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
    National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>.
    Code Documentation is available at: <http://doxygen.mantidproject.org>
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
