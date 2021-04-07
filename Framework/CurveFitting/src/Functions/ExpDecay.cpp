// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/ExpDecay.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(ExpDecay)

ExpDecay::ExpDecay() {
  declareParameter("Height", 1.0, "Height at time 0");
  declareParameter("Lifetime", 1.0, "Lifetime of the process");
}

void ExpDecay::function1D(double *out, const double *xValues, const size_t nData) const {
  const double h = getParameter("Height");
  const double t = getParameter("Lifetime");

  for (size_t i = 0; i < nData; i++) {
    out[i] = h * exp(-(xValues[i]) / t);
  }
}

void ExpDecay::functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) {
  const double h = getParameter("Height");
  const double t = getParameter("Lifetime");

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    double e = exp(-x / t);
    out->set(i, 0, e);
    out->set(i, 1, h * e * x / t / t);
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
