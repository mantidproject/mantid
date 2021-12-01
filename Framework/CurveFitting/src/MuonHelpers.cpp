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

#include <algorithm>
#include <numeric>

namespace Mantid::CurveFitting::MuonHelper {

double getAz(double xValue, const double charField){
  const double b = xValue / charField;
  const double bSq = pow(b, 2);
  const double A_z = (3 / 4) - (1 / (4 * bSq)) + (pow(bSq - 1, 2) / (8 * pow(b, 3))) * log(fabs((b + 1) / (b - 1)));
  return A_z;
}


} // namespace Mantid::CurveFitting::MuonHelper
