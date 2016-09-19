#ifndef MANTID_API_IFUNCTIONGENERAL_H_
#define MANTID_API_IFUNCTIONGENERAL_H_

#include "MantidAPI/DllConfig.h"

#include "MantidAPI/FunctionDomainGeneral.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/Logger.h"

namespace Mantid {
namespace API {

/** IFunctionGeneral: a very general function definition.
    It gets its arguments from a FunctionDomainGeneral and they
    can have any type. An argument can be a collection of a number
    of values of different types.

    The domain and the values object can have different sizes.
    In particular the domain can be empty.


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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
class DLLExport IFunctionGeneral : public virtual IFunction {
public:
  void function(const FunctionDomain &domain,
                FunctionValues &values) const override;
  void functionDeriv(const FunctionDomain &domain, Jacobian &jacobian) override;

  size_t getValuesSize(const FunctionDomain &domain) const override;

  /// Provide a concrete function in an implementation that operates on a
  /// FunctionDomainGeneral.
  virtual void functionGeneral(const FunctionDomainGeneral &domain,
                               FunctionValues &values) const = 0;

  /// Get number of columns that the domain must have.
  /// If consider the collection of these columns as a table
  /// then a row corresponds to a single (multi-valued) argument.
  virtual size_t getNumberDomainColumns() const = 0;

  /// Get number of values per argument in the domain.
  virtual size_t getNumberValuesPerArgument() const = 0;

  /// Get the default size of a domain.
  /// If a function is given an empty domain then it must output
  /// a values object of the size:
  ///     getDefaultDomainSize() * getNumberValuesPerArgument()
  /// The default size must not be infinite (max of size_t).
  virtual size_t getDefaultDomainSize() const;

protected:
  static Kernel::Logger g_log;
};

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_IFUNCTIONGENERAL_H_ */
