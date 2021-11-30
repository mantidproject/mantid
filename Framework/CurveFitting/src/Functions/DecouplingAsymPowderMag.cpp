// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/DecouplingAsymPowderMag.h"
#include "MantidAPI/FunctionFactory.h"

#include <cmath>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(DecouplingAsymPowderMag)

void DecouplingAsymPowderMag::init() {
  declareParameter("Asymmetry", 1.0, "a scaling parameter for the overall asymmetry");
  declareParameter("CharField", 1.0, "the characteristic field");
}

void DecouplingAsymPowderMag::function1D(double *out, const double *xValues, const size_t nData) const {
  const double asym = getParameter("Asymmetry");
  const double charField = getParameter("CharField");


  for (size_t i = 0; i < nData; i++) {
    auto A_z = getAz(xValues[i], charField);
    out[i] = asym * A_z;
  }
}

void DecouplingAsymPowderMag::functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) {
  const double asym = getParameter("Asymmetry");
  const double charField = getParameter("CharField");

  for (size_t i = 0; i < nData; i++) {
    auto A_z = getAz(xValues[i], charField);
    double diffasym = A_z;
    double diffAz = asym;
    out->set(i, 0, diffasym);
    out->set(i, 1, diffAz);
  }
}

const double DecouplingAsymPowderMag::getAz(double xValue, const double charField) const {
  const double b = xValue / charField;
  const double bSq = pow(b, 2);
  const double A_z = (3/4)-(1/(4*bSq))+(pow(bSq-1,2)/(8*pow(b,3)))*log(fabs((b+1)/(b-1)));
  return A_z;
}

} // namespace Mantid::CurveFitting::Functions