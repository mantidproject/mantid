#ifndef MANTID_CURVEFITTING_RESOLUTION_H_
#define MANTID_CURVEFITTING_RESOLUTION_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/TabulatedFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Resolution function. It is implemented in terms of TabulatedFunction but doesn't
inherit form it.
It is done to make Resolution parameterless and at the same time use
TabulatedFunction's attributes.

@author Roman Tolchenov, Tessella plc
@date 12/02/2010

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport Resolution : public API::ParamFunction,
                             public API::IFunction1D {
public:
  /// Constructor
  Resolution();

  /// overwrite IFunction base class methods
  std::string name() const override { return "Resolution"; }
  /// Function values
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  ///  function derivatives
  void functionDeriv1D(API::Jacobian *out, const double *xValues,
                       const size_t nData) override;
  /// Returns the number of attributes associated with the function
  size_t nAttributes() const override;
  /// Returns a list of attribute names
  std::vector<std::string> getAttributeNames() const override;
  /// Return a value of attribute attName
  Attribute getAttribute(const std::string &attName) const override;
  /// Set a value to attribute attName
  void setAttribute(const std::string &attName, const Attribute &) override;
  /// Check if attribute attName exists
  bool hasAttribute(const std::string &attName) const override;

private:
  /// Function that does the actual job
  TabulatedFunction m_fun;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_RESOLUTION_H_*/
