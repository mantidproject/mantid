#ifndef MANTID_CURVEFITTING_THERMALNEUTRONDTOTOFFUNCTION_H_
#define MANTID_CURVEFITTING_THERMALNEUTRONDTOTOFFUNCTION_H_

#include "MantidKernel/System.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidKernel/Logger.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/Jacobian.h"

#include <gsl/gsl_sf_erf.h>
#include <cmath>

namespace Mantid {
namespace CurveFitting {

/** ThermalNeutronDtoTOFFunction : TODO: DESCRIPTION

  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class DLLExport ThermalNeutronDtoTOFFunction : virtual public API::IFunction1D,
                                               public API::ParamFunction {
public:
  ThermalNeutronDtoTOFFunction();
  virtual ~ThermalNeutronDtoTOFFunction();

  /// Override
  virtual void function1D(double *out, const double *xValues,
                          const size_t nData) const;

  /// overwrite IFunction base class methods
  std::string name() const { return "ThermalNeutronDtoTOFFunction"; }

  /// Overwrite IFunction
  virtual const std::string category() const { return "General"; }

  /// Calculate function values
  void function1D(std::vector<double> &out,
                  const std::vector<double> xValues) const;

protected:
  /// overwrite IFunction base class method, which declare function parameters
  virtual void init();

private:
  /// Core function (inline) to calcualte TOF_h from d-spacing
  inline double corefunction(double dh, double dtt1, double dtt1t, double dtt2t,
                             double zero, double zerot, double width,
                             double tcross) const;

  /// Derivative
  void functionDerivLocal(API::Jacobian *, const double *, const size_t);

  /// Derivative
  // void functionDeriv(const API::FunctionDomain& domain, API::Jacobian&
  // jacobian);

  /// Derviate to overwritten
  void functionDeriv1D(API::Jacobian *out, const double *xValues,
                       const size_t nData);

  // void functionDeriv(const API::FunctionDomain& domain, API::Jacobian&
  // jacobian);
};

typedef boost::shared_ptr<ThermalNeutronDtoTOFFunction>
    ThermalNeutronDtoTOFFunction_sptr;

/// Calcualte TOF from d-spacing value for thermal neutron
inline double calThermalNeutronTOF(double dh, double dtt1, double dtt1t,
                                   double dtt2t, double zero, double zerot,
                                   double width, double tcross) {
  double n = 0.5 * gsl_sf_erfc(width * (tcross - 1 / dh));
  double Th_e = zero + dtt1 * dh;
  double Th_t = zerot + dtt1t * dh - dtt2t / dh;
  double tof_h = n * Th_e + (1 - n) * Th_t;

  return tof_h;
}

} // namespace CurveFitting
} // namespace Mantid

#endif /* MANTID_CURVEFITTING_THERMALNEUTRONDTOTOFFUNCTION_H_ */
