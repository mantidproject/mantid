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
  declareParameter("BackgroundAsymmetry", 0.0, "coefficient for constant term");
}

void MuoniumDecouplingCurve::function1D(double *out, const double *xValues, const size_t nData) const {
  const double RepolAS = getParameter("RepolarisingAsymmetry");
  const double DecoupField = getParameter("DecouplingField");
  const double BkgdAS = getParameter("BackgroundAsymmetry");

  for (size_t i = 0; i < nData; i++) {
    out[i] = RepolAS * (0.5 + pow(xValues[i] / DecoupField, 2)) / (1 + pow(xValues[i] / DecoupField, 2)) + BkgdAS;
  }
}

void MuoniumDecouplingCurve::functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) {
  const double RepolAS = getParameter("RepolarisingAsymmetry");
  const double DecoupField = getParameter("DecouplingField");

  for (size_t i = 0; i < nData; i++) {
    double diffRepolAS = (0.5 * pow(DecoupField, 2) + pow(xValues[i], 2)) / (pow(DecoupField, 2) + pow(xValues[i], 2));
    double diffDecoupField =
        -(RepolAS * DecoupField * pow(xValues[i], 2)) / pow((pow(DecoupField, 2) + pow(xValues[i], 2)), 2);
    out->set(i, 0, diffRepolAS);
    out->set(i, 1, diffDecoupField);
    out->set(i, 2, 1);
  }
}

} // namespace Mantid::CurveFitting::Functions