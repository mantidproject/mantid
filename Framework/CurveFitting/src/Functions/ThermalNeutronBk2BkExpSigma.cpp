// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/ThermalNeutronBk2BkExpSigma.h"
#include "MantidAPI/FunctionFactory.h"

#include <cmath>
#include <gsl/gsl_sf_erf.h>

using namespace std;

using namespace Mantid;

using namespace Mantid::API;

using namespace Mantid::CurveFitting;

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

DECLARE_FUNCTION(ThermalNeutronBk2BkExpSigma)

//----------------------------------------------------------------------------------------------
/** Defintion of parameter
 */
void ThermalNeutronBk2BkExpSigma::init() {
  declareParameter("Sig0", 0.0);
  declareParameter("Sig1", 0.0);
  declareParameter("Sig2", 0.0);
}

//----------------------------------------------------------------------------------------------
/** Function 1D
 */
void ThermalNeutronBk2BkExpSigma::function1D(double *out, const double *xValues, const size_t nData) const {
  double sig0 = getParameter("Sig0");
  double sig1 = getParameter("Sig1");
  double sig2 = getParameter("Sig2");

  double sig0sq = sig0 * sig0;
  double sig1sq = sig1 * sig1;
  double sig2sq = sig2 * sig2;

  for (size_t i = 0; i < nData; ++i) {
    out[i] = corefunction(xValues[i], sig0sq, sig1sq, sig2sq);
  }
}

/** Derivative: use numerical derivative
 */
void ThermalNeutronBk2BkExpSigma::functionDeriv(const FunctionDomain &domain, Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}

//----------------------------------------------------------------------------------------------
/** Core function
 */
double ThermalNeutronBk2BkExpSigma::corefunction(double dh, double sig0sq, double sig1sq, double sig2sq) const {
  double sigma2 = sig0sq + sig1sq * dh * dh + sig2sq * pow(dh, 4);
  if (sigma2 <= 0.0) {
    throw runtime_error("Sigma^2 cannot be equal to or less than Zero!");
  }

  double sigma = sqrt(sigma2);

  return sigma;
}

} // namespace Mantid::CurveFitting::Functions
