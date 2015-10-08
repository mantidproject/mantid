//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/CostFuncIgnorePosPeaks.h"
#include "MantidKernel/PhysicalConstants.h"
#include <cmath>
#include <gsl/gsl_sf_erf.h>

namespace Mantid {
namespace CurveFitting {

DECLARE_COSTFUNCTION(CostFuncIgnorePosPeaks, Ignore positive peaks)

/// Calculate value of cost function from observed and calculated values
/// note yCal modified for computational efficiency
/// @param yData :: Array of yData
/// @param inverseError :: Array of inverse error values
/// @param yCal :: Calculated y
/// @param n :: The number of points
/// @return The calculated cost value
double CostFuncIgnorePosPeaks::val(const double *yData,
                                   const double *inverseError, double *yCal,
                                   const size_t &n) {
  for (size_t i = 0; i < n; i++)
    yCal[i] = (yData[i] - yCal[i]) * inverseError[i];

  double retVal = 0.0;

  double a = 2.0 / sqrt(M_PI);
  double b = 1 / sqrt(2.0);
  for (size_t i = 0; i < n; i++) {
    if (yCal[i] <= 0.0)
      retVal += yCal[i] * yCal[i];
    else
      retVal += 6.0 * log(a * yCal[i] / gsl_sf_erf(yCal[i] * b));
  }

  return retVal;
}

/// Calculate the derivatives of the cost function
/// @param yData :: Array of yData
/// @param inverseError :: Array of inverse error values
/// @param yCal :: Calculated y
/// @param jacobian :: Output jacobian
/// @param outDerivs :: Output derivaties
/// @param p :: The number of parameters
/// @param n :: The number of points
void CostFuncIgnorePosPeaks::deriv(const double *yData,
                                   const double *inverseError,
                                   const double *yCal, const double *jacobian,
                                   double *outDerivs, const size_t &p,
                                   const size_t &n) {
  double a = 2.0 / sqrt(M_PI);
  double b = 1 / sqrt(2.0);

  for (size_t iP = 0; iP < p; iP++) {
    outDerivs[iP] = 0.0;
    for (size_t iY = 0; iY < n; iY++) {
      if (yCal[iY] >= yData[iY])
        outDerivs[iP] += 2.0 * (yCal[iY] - yData[iY]) * jacobian[iY * p + iP] *
                         inverseError[iY] * inverseError[iY];
      else {
        double z = (yData[iY] - yCal[iY]) * inverseError[iY];
        double erf = gsl_sf_erf(b * z);

        outDerivs[iP] -=
            jacobian[iY * p + iP] * 6.0 *
            (-2.0 * a * b * exp(-b * b * z * z) * z / sqrt(M_PI) + a * erf) /
            (a * z * erf);
      }
    }
  }
}

} // namespace CurveFitting
} // namespace Mantid
