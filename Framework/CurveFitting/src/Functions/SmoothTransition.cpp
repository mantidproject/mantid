// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/SmoothTransition.h"
#include "MantidAPI/FunctionFactory.h"

#include <cmath>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(SmoothTransition)

void SmoothTransition::init() {
  declareParameter("A1", 0.0, "the limit of the function as x tends to zero");
  declareParameter("A2", 0.1, "the limit of the function as x tends to infinity");
  declareParameter("Midpoint", 100.0, "Sigmoid Midpoint");
  declareParameter("GrowthRate", 1.0, "Growth rate");
}

void SmoothTransition::function1D(double *out, const double *xValues, const size_t nData) const {
  const double a1 = getParameter("A1");
  const double a2 = getParameter("A2");
  const double midpoint = getParameter("Midpoint");
  const double gr = getParameter("GrowthRate");

  for (size_t i = 0; i < nData; i++) {
    out[i] = a2 + (a1 - a2) / (exp((xValues[i] - midpoint) / gr) + 1);
  }
}

void SmoothTransition::functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) {
  const double a1 = getParameter("A1");
  const double a2 = getParameter("A2");
  const double midpoint = getParameter("Midpoint");
  const double gr = getParameter("GrowthRate");

  for (size_t i = 0; i < nData; i++) {
    double expFunc = exp((xValues[i] - midpoint) / gr);
    double denominatorSq = pow((expFunc + 1), 2);

    double diffa1 = 1 / (expFunc + 1);
    double diffa2 = 1 - diffa1;
    double diffmidpoint = ((a1 - a2) * expFunc) / (gr * denominatorSq);
    double diffgr = ((a1 - a2) * (xValues[i] - midpoint) * expFunc) / (pow(gr, 2) * denominatorSq);

    out->set(i, 0, diffa1);
    out->set(i, 1, diffa2);
    out->set(i, 2, diffmidpoint);
    out->set(i, 3, diffgr);
  }
}

} // namespace Mantid::CurveFitting::Functions
