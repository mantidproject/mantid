// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/GausDecay.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(GausDecay)

void GausDecay::init() {
  declareParameter("A", 10.0, "Amplitude at time 0");
  declareParameter("Sigma", 0.2, "Decay rate");
}

void GausDecay::function1D(double *out, const double *xValues, const size_t nData) const {
  const double A = getParameter("A");
  const double G = getParameter("Sigma");

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    out[i] = A * exp(-G * G * x * x);
  }
}

void GausDecay::functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) {
  const double A = getParameter("A");
  const double G = getParameter("Sigma");

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    double g = exp(-G * G * x * x);
    out->set(i, 0, g);
    out->set(i, 1, -2 * G * x * x * A * g);
  }
}

void GausDecay::setActiveParameter(size_t i, double value) {
  size_t j = i;

  if (parameterName(j) == "Sigma")
    setParameter(j, fabs(value), false); // Make sigma positive
  else
    setParameter(j, value, false);
}

} // namespace Mantid::CurveFitting::Functions
