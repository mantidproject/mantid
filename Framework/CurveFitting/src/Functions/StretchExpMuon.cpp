// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/StretchExpMuon.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(StretchExpMuon)

void StretchExpMuon::init() {
  declareParameter("A", 0.2, "Amplitude (height at origin)");
  declareParameter("Lambda", 0.2, "Decay rate of the standard exponential");
  declareParameter("Beta", 0.2, "Stretching exponent, usually in the (0,2] range");
}

void StretchExpMuon::function1D(double *out, const double *xValues, const size_t nData) const {
  const double A = getParameter("A");
  const double G = getParameter("Lambda");
  const double b = getParameter("Beta");

  for (size_t i = 0; i < nData; i++) {
    out[i] = A * exp(-pow(G * xValues[i], b));
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
