// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidCurveFitting/MuonHelpers.h"
#include <cmath>

namespace Mantid::CurveFitting::MuonHelper {

double getAz(double xValue, const double charField) {
  const double b = xValue / charField;
  const double bSq = pow(b, 2);
  const double A_z = (3.0 / 4.0) - (1.0 / (4.0 * bSq)) +
                     (pow(bSq - 1.0, 2.0) / (8.0 * pow(b, 3.0))) * log(fabs((b + 1.0) / (b - 1.0)));
  return A_z;
}

double getDiffAz(double xValue, const double charField) {
  double diffcharfield = -xValue / pow(charField, 2);
  double b = xValue / charField;
  double diffb = ((pow(b, 2) - 1) * ((pow(b, 2) + 3) * log((b + 1.0) / (b - 1.0)) - (2 * b))) / (8 * pow(b, 4));
  double diffAz = diffcharfield * diffb;
  if (!std::isfinite(diffAz)) {
    diffAz = 0.0;
  }
  return diffAz;
}

double getActivationFunc(double xValue, const double attemptRate, const double barrier, const double unitMultiply) {
  return attemptRate * exp(-(unitMultiply * barrier) / xValue);
}

double getAttemptRateDiff(double xValue, const double barrier, const double unitMultiply) {
  return exp(-(unitMultiply * barrier) / xValue);
}

double getBarrierDiff(double xValue, const double attemptRate, const double barrier, const double unitMultiply) {
  return -(attemptRate * unitMultiply * (exp(-(unitMultiply * barrier) / xValue))) / xValue;
}

} // namespace Mantid::CurveFitting::MuonHelper
