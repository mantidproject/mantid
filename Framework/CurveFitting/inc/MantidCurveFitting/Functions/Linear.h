#ifndef MANTID_CURVEFITTING_LINEAR_H_
#define MANTID_CURVEFITTING_LINEAR_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/BackgroundFunction.h"
#include <boost/scoped_array.hpp>

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Provide linear function interface to IFunction.
I.e. the function: A0+A1*x.

Linear parameters:
<UL>
<LI> A0 - coefficient for constant term (default 0.0)</LI>
<LI> A1 - coefficient for linear term (default 0.0)</LI>
</UL>

@author Anders Markvardsen, ISIS, RAL
@date 23/10/2009

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
class DLLExport Linear : public BackgroundFunction {
public:
  /// overwrite IFunction1D base class methods
  std::string name() const override { return "Linear"; }

  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  void derivative1D(double *out, const double *xValues,
                    const size_t nData,
                    const size_t order = 1) const override;

  /// Set a value to attribute attName
  void setAttribute(const std::string &attName, const Attribute &) override;

protected:
  /// overwrite IFunction base class method, which declares function parameters
  void init() override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_LINEARBACKGROUND_H_*/
