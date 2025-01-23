// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/ThermalNeutronBk2BkExpBeta.h"
#include "MantidAPI/FunctionFactory.h"

#include <cmath>
#include <gsl/gsl_sf_erf.h>

using namespace std;

using namespace Mantid;

using namespace Mantid::CurveFitting;

using namespace Mantid::API;

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

DECLARE_FUNCTION(ThermalNeutronBk2BkExpBeta)

//----------------------------------------------------------------------------------------------
/** Defintion of parameter
 */
void ThermalNeutronBk2BkExpBeta::init() {
  declareParameter("Width", 1.0);
  declareParameter("Tcross", 1.0);

  declareParameter("Beta0", 0.0);
  declareParameter("Beta1", 0.0);
  declareParameter("Beta0t", 0.0);
  declareParameter("Beta1t", 0.0);
}

//----------------------------------------------------------------------------------------------
/** Function 1D
 */
void ThermalNeutronBk2BkExpBeta::function1D(double *out, const double *xValues, const size_t nData) const {
  double width = getParameter("Width");
  double tcross = getParameter("Tcross");
  double beta0 = getParameter("Beta0");
  double beta1 = getParameter("Beta1");
  double beta0t = getParameter("Beta0t");
  double beta1t = getParameter("Beta1t");

  for (size_t i = 0; i < nData; ++i) {
    out[i] = corefunction(xValues[i], width, tcross, beta0, beta1, beta0t, beta1t);
  }
}

/** Derivative: use numerical derivative
 */
void ThermalNeutronBk2BkExpBeta::functionDeriv(const FunctionDomain &domain, Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}

//----------------------------------------------------------------------------------------------
/** Core function
 */
double ThermalNeutronBk2BkExpBeta::corefunction(double dh, double width, double tcross, double beta0, double beta1,
                                                double beta0t, double beta1t) const {
  double n = 0.5 * gsl_sf_erfc(width * (tcross - 1.0 / dh));
  double beta = 1.0 / (n * (beta0 + beta1 * dh) + (1.0 - n) * (beta0t - beta1t / dh));

  return beta;
}

} // namespace Mantid::CurveFitting::Functions
