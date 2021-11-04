// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/Activation.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/PhysicalConstants.h"
#include <boost/algorithm/string.hpp>

#include <cmath>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(Activation)

void Activation::init() {
  declareAttribute("Unit", Attribute("K")); // or meV
  declareParameter("AttemptRate", 1000.0, "coefficient for attempt rate");
  declareParameter("Barrier", 1000.0, "coefficient for barrier energy");
}

void Activation::function1D(double *out, const double *xValues, const size_t nData) const {
  beforeFunctionSet();

  const double attemptRate = getParameter("AttemptRate");
  const double barrier = getParameter("Barrier");
  const double meVConv = getMeVConv();

  for (size_t i = 0; i < nData; i++) {
    out[i] = attemptRate * exp(-(meVConv * barrier) / xValues[i]);
  }
}

void Activation::functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) {
  beforeFunctionSet();

  const double attemptRate = getParameter("AttemptRate");
  const double barrier = getParameter("Barrier");
  const double meVConv = getMeVConv();

  for (size_t i = 0; i < nData; i++) {
    double diffAR = exp(-(meVConv * barrier) / xValues[i]);
    double diffBarrier = -(attemptRate * meVConv * (exp(-(meVConv * barrier) / xValues[i]))) / xValues[i];
    out->set(i, 0, diffAR);
    out->set(i, 1, diffBarrier);
  }
}

void Activation::beforeFunctionSet() const {
  auto unit = getAttribute("Unit").asString();
  boost::to_lower(unit);

  if (unit.compare("k") != 0 && unit.compare("mev") != 0) {
    throw std::invalid_argument("Activation function can only be used with K or meV as the unit");
  }
}

double Activation::getMeVConv() const {
  auto unit = getAttribute("Unit").asString();
  boost::to_lower(unit);
  double meVConv;

  if (unit.compare("k") == 0) {
    meVConv = 1.0;
  }

  if (unit.compare("mev") == 0) {
    meVConv = PhysicalConstants::meVtoKelvin;
  }
  return meVConv;
}

} // namespace Mantid::CurveFitting::Functions