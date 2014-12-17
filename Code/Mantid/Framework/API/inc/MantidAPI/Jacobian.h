#ifndef MANTID_API_JACOBIAN_H_
#define MANTID_API_JACOBIAN_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Exception.h"

namespace Mantid {
namespace API {
/**
    Represents the Jacobian in IFitFunction::functionDeriv.

    @author Roman Tolchenov, Tessella plc
    @date 15/11/2011

    Copyright &copy; 2009 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class Jacobian {
public:
  /**  Set a value to a Jacobian matrix element.
  *   @param iY :: The index of a data point.
  *   @param iP :: The index of a declared parameter.
  *   @param value :: The derivative value.
  */
  virtual void set(size_t iY, size_t iP, double value) = 0;

  /**  Get the value to a Jacobian matrix element.
  *   @param iY :: The index of a data point.
  *   @param iP :: The index of a declared parameter.
  */
  virtual double get(size_t iY, size_t iP) = 0;

  ///@cond do not document
  /**  Add number to all iY (data) Jacobian elements for a given iP (parameter)
  *   @param value :: Value to add
  */
  virtual void addNumberToColumn(const double &value, const size_t &iActiveP) {
    (void)value;
    (void)iActiveP; // Avoid compiler warning
    throw Kernel::Exception::NotImplementedError(
        "No addNumberToColumn() method of Jacobian provided");
  }
  ///@endcond

  /// Virtual destructor
  virtual ~Jacobian(){};

protected:
};

} // namespace API
} // namespace Mantid

#endif /*MANTID_API_JACOBIAN_H_*/
