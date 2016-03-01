#ifndef MANTID_CURVEFITTING_QUADRATIC_H_
#define MANTID_CURVEFITTING_QUADRATIC_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/BackgroundFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Provide quadratic function interface to IFunction.
I.e. the function: A0+A1*x+A2*x^2

Quadratic parameters:
<UL>
<LI> A0 - coefficient for constant term (default 0.0)</LI>
<LI> A1 - coefficient for linear term (default 0.0)</LI>
<LI> A2 - coefficient for quadratic term (default 0.0)</LI>
</UL>

@author Anders Markvardsen, ISIS, RAL
@date 20/10/2009

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
class DLLExport Quadratic : public BackgroundFunction {
public:
  /// overwrite IFunction base class methods
  std::string name() const override { return "Quadratic"; }
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  void functionDeriv1D(API::Jacobian *out, const double *xValues,
                       const size_t nData) override;

protected:
  /// overwrite IFunction base class method, which declare function parameters
  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_QUADRATIC_H_*/
