// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/MuoniumDecouplingCurve.h"
#include "MantidAPI/FunctionFactory.h"

#include <cmath>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(MuoniumDecouplingCurve)

void MuoniumDecouplingCurve::init() {
  declareParameter("RepolarisingAsymmetry", 1.0, "coefficient for linear term");
  declareParameter("DecouplingField", 1.0, "??");
  declareParameter("BaselineAsymmetry", 0.0, "coefficient for constant term");
}

void MuoniumDecouplingCurve::function1D(double *out, const double *xValues, const size_t nData) const {
  const double a = getParameter("RepolarisingAsymmetry");
  const double b = getParameter("DecouplingField");
  const double c = getParameter("BaselineAsymmetry");

  for (size_t i = 0; i < nData; i++) {
    out[i] = a * (0.5 + pow(xValues[i] / b, 2)) / (1 + pow(xValues[i] / b, 2)) + c;
  }
}

void MuoniumDecouplingCurve::functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) {
  const double a = getParameter("RepolarisingAsymmetry");
  const double b = getParameter("DecouplingField");

  for (size_t i = 0; i < nData; i++) {
    double diffa = (0.5 * pow(b, 2) + pow(xValues[i], 2)) / (pow(b, 2) + pow(xValues[i], 2));
    double diffb = a * b * pow(xValues[i], 2) / pow(pow(b, 2) + pow(xValues[i], 2), 2);
    out->set(i, 0, diffa);
    out->set(i, 1, diffb);
    out->set(i, 2, 1);
  }
}

} // namespace Mantid::CurveFitting::Functions