// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/DeltaFunction.h"
#include "MantidAPI/FunctionFactory.h"
#include <algorithm>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(DeltaFunction)

DeltaFunction::DeltaFunction() : IPeakFunction() {
  declareParameter("Height", 1.0, "Scaling factor to be applied to the resolution.");
  declareParameter("Centre", 0.0, "Shift along the x-axis to be applied to the resolution.");
}

void DeltaFunction::function1D(double *out, const double *xValues, const size_t nData) const {
  UNUSED_ARG(xValues);
  std::fill(out, out + nData, 0);
}

void DeltaFunction::functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) {
  UNUSED_ARG(out);
  UNUSED_ARG(xValues);
  UNUSED_ARG(nData);
  throw std::runtime_error("Cannot compute derivative of a delta function");
}

} // namespace Mantid::CurveFitting::Functions
