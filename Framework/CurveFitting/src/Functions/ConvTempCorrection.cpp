// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidCurveFitting/Functions/ConvTempCorrection.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace {
// Conversion factor from meV to K
constexpr double CONVERSIONFACTOR = 11.606;
} // namespace

namespace Mantid {
namespace CurveFitting {
namespace Functions {

DECLARE_FUNCTION(ConvTempCorrection)

void ConvTempCorrection::init() { declareParameter("Temperature", 300.0, "Temperature correction value (K)"); }

void ConvTempCorrection::function1D(double *out, const double *xValues, const size_t nData) const {
  const double T = getParameter("Temperature");
  for (size_t i = 0; i < nData; ++i) {
    double x = xValues[i];
    if (x == 0.0) {
      out[i] = 1.00;
    } else {
      out[i] = (x * CONVERSIONFACTOR) / T / (1 - exp(-((x * CONVERSIONFACTOR) / T)));
    }
  }
}
} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
