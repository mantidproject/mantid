#ifndef MANTID_CURVEFITTING_LOGNORMAL_H_
#define MANTID_CURVEFITTING_LOGNORMAL_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {
/**
Provide Log Normal function: h*exp(-(log(x)-t)^2 / (2*b^2) )/x

@author Jose Borreguero, NScD
@date 11/14/2011

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
class DLLExport LogNormal : public API::ParamFunction, public API::IFunction1D {
public:
  /// Constructor
  LogNormal();

  /// overwrite IFunction base class methods
  std::string name() const override { return "LogNormal"; }
  const std::string category() const override { return "Peak"; }

protected:
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override;
  void functionDeriv1D(API::Jacobian *out, const double *xValues,
                       const size_t nData) override;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif /*MANTID_CURVEFITTING_LOGNORMAL_H_*/
