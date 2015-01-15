#include "MantidCurveFitting/ThermalNeutronBk2BkExpSigma.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FunctionFactory.h"

#include <gsl/gsl_sf_erf.h>
#include <cmath>

using namespace std;
using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;

namespace Mantid {
namespace CurveFitting {

DECLARE_FUNCTION(ThermalNeutronBk2BkExpSigma)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ThermalNeutronBk2BkExpSigma::ThermalNeutronBk2BkExpSigma() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ThermalNeutronBk2BkExpSigma::~ThermalNeutronBk2BkExpSigma() {}

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
void ThermalNeutronBk2BkExpSigma::function1D(double *out, const double *xValues,
                                             const size_t nData) const {
  double sig0 = getParameter("Sig0");
  double sig1 = getParameter("Sig1");
  double sig2 = getParameter("Sig2");

  double sig0sq = sig0 * sig0;
  double sig1sq = sig1 * sig1;
  double sig2sq = sig2 * sig2;

  for (size_t i = 0; i < nData; ++i) {
    out[i] = corefunction(xValues[i], sig0sq, sig1sq, sig2sq);
  }

  return;
}

/** Derivative: use numerical derivative
  */
void ThermalNeutronBk2BkExpSigma::functionDeriv(const FunctionDomain &domain,
                                                Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}

//----------------------------------------------------------------------------------------------
/** Core function
  */
double ThermalNeutronBk2BkExpSigma::corefunction(double dh, double sig0sq,
                                                 double sig1sq,
                                                 double sig2sq) const {
  double sigma2 = sig0sq + sig1sq * dh * dh + sig2sq * pow(dh, 4);
  if (sigma2 <= 0.0) {
    throw runtime_error("Sigma^2 cannot be equal to or less than Zero!");
  }

  double sigma = sqrt(sigma2);

  return sigma;
}

} // namespace CurveFitting
} // namespace Mantid
