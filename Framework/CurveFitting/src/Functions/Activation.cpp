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
  declareParameter("Height", 1.0, "coefficient for height");
  declareParameter("Lifetime", 1.0, "coefficient for Lifetime");
}

void Activation::function1D(double *out, const double *xValues, const size_t nData) const {
  beforeFunctionSet();

  const double height = getParameter("Height");
  const double lifetime = getParameter("Lifetime");
  const double meVConv = getMeVConv();

  for (size_t i = 0; i < nData; i++) {
    out[i] = height * exp(-(meVConv * lifetime) / xValues[i]);
  }
}

void Activation::functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) {
  beforeFunctionSet();

  const double height = getParameter("Height");
  const double lifetime = getParameter("Lifetime");
  const double meVConv = getMeVConv();

  for (size_t i = 0; i < nData; i++) {
    double diffHeight = exp(-(meVConv * lifetime) / xValues[i]);
    double diffLifetime = (height * meVConv * (exp(-(meVConv * lifetime) / xValues[i]))) / xValues[i];
    out->set(i, 0, diffHeight);
    out->set(i, 1, diffLifetime);
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
  const double meVConv = PhysicalConstants::meVtoKelvin;

  if (unit.compare("k") == 0) {
    return 1.0;
  }

  if (unit.compare("mev") == 0) {
    return meVConv;
  }
}

} // namespace Mantid::CurveFitting::Functions