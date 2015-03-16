#ifndef MANTID_CURVEFITTING_PEAKPARAMETERFUNCTION_H_
#define MANTID_CURVEFITTING_PEAKPARAMETERFUNCTION_H_

#include "MantidCurveFitting/DllConfig.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/FunctionParameterDecorator.h"
#include "MantidAPI/IPeakFunction.h"

namespace Mantid {
namespace CurveFitting {

/** PeakParameterFunction :

  This function implements API::FunctionParameterDecorator to wrap an
  IPeakFunction. The function expects a FunctionDomain1D with size exactly 4,
  corresponding to the 4 special parameters centre, height, fwhm and intensity.

  They are stored in the output values in that order. Calculating the derivative
  of the function yields the partial derivatives of these 4 parameters with
  respect to the function's native parameters defined through declareParameter.

    @author Michael Wedel, Paul Scherrer Institut - SINQ
    @date 24/02/2015

    Copyright ©2015 PSI-NXMM

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
class MANTID_CURVEFITTING_DLL PeakParameterFunction
    : virtual public API::IFunction1D,
      virtual public API::FunctionParameterDecorator {
public:
  PeakParameterFunction() : FunctionParameterDecorator() {}
  virtual ~PeakParameterFunction() {}

  std::string name() const { return "PeakParameterFunction"; }

  void function1D(double *out, const double *xValues, const size_t nData) const;

  void functionDeriv(const API::FunctionDomain &domain,
                     API::Jacobian &jacobian);

protected:
  void beforeDecoratedFunctionSet(const API::IFunction_sptr &fn);

  API::IPeakFunction_sptr m_peakFunction;
};

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_PEAKPARAMETERFUNCTION_H_ */
