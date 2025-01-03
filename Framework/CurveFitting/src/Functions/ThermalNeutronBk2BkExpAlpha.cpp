// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/ThermalNeutronBk2BkExpAlpha.h"
#include "MantidAPI/FunctionFactory.h"

#include <cmath>
#include <gsl/gsl_sf_erf.h>

using namespace std;

using namespace Mantid;

using namespace Mantid::API;

using namespace Mantid::CurveFitting;

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

DECLARE_FUNCTION(ThermalNeutronBk2BkExpAlpha)

//----------------------------------------------------------------------------------------------
/** Defintion of parameter
 */
void ThermalNeutronBk2BkExpAlpha::init() {
  // Geometry related
  declareParameter("Width", 1.0);
  declareParameter("Tcross", 1.0);

  declareParameter("Alph0", 0.0);
  declareParameter("Alph1", 0.0);
  declareParameter("Alph0t", 0.0);
  declareParameter("Alph1t", 0.0);
}

//----------------------------------------------------------------------------------------------
/** Function 1D
 */
void ThermalNeutronBk2BkExpAlpha::function1D(double *out, const double *xValues, const size_t nData) const {
  double width = getParameter("Width");
  double tcross = getParameter("Tcross");
  double alph0 = getParameter("Alph0");
  double alph1 = getParameter("Alph1");
  double alph0t = getParameter("Alph0t");
  double alph1t = getParameter("Alph1t");

  for (size_t i = 0; i < nData; ++i) {
    out[i] = corefunction(xValues[i], width, tcross, alph0, alph1, alph0t, alph1t);
  }
}

/** Derivative: use numerical derivative
 */
void ThermalNeutronBk2BkExpAlpha::functionDeriv(const FunctionDomain &domain, Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}

//----------------------------------------------------------------------------------------------
/** Core function
 */
double ThermalNeutronBk2BkExpAlpha::corefunction(double dh, double width, double tcross, double alph0, double alph1,
                                                 double alph0t, double alph1t) const {
  double n = 0.5 * gsl_sf_erfc(width * (tcross - 1.0 / dh));
  double alpha = 1.0 / (n * (alph0 + alph1 * dh) + (1.0 - n) * (alph0t - alph1t / dh));

  return alpha;
}

} // namespace Mantid::CurveFitting::Functions
