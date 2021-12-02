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
                     (pow(bSq - 1.0, 2.0) / (8.0 * pow(b, 3.0))) * log10(fabs((b + 1.0) / (b - 1.0)));
  return A_z;
}

} // namespace Mantid::CurveFitting::MuonHelper
