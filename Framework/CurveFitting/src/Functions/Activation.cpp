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
  const double expFunc = getExp();

  for (size_t i = 0; i < nData; i++) {
    out[i] = height * exp(-expFunc * (1 / xValues[i]));
  }
}

void Activation::functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) {
  beforeFunctionSet();

  const double height = getParameter("Height");
  const double lifetime = getParameter("Lifetime");
  const double expFunc = getExp();
  // auto unit = getAttribute("Unit").asString();
  // const double k_B = PhysicalConstants::BoltzmannConstant;

  for (size_t i = 0; i < nData; i++) {
    double diffHeight = exp(-expFunc * (1 / xValues[i]));
    double diffLifetimeMulti = getLifetimeMulti();
    double diffLifetime = diffLifetimeMulti * ((height * exp(-expFunc * (1 / xValues[i])) / xValues[i]));
    out->set(i, 0, diffHeight);
    out->set(i, 1, diffLifetime);
  }
}

void Activation::beforeFunctionSet() const {
  auto unit = getAttribute("Unit").asString();

  if (unit.compare("K") != 0 || unit.compare("meV") != 0) {
    throw std::invalid_argument("Activation function can only be used with K or meV as the unit");
  }
}

double Activation::getExp() const {
  auto unit = getAttribute("Unit").asString();
  const double lifetime = getParameter("Lifetime");

  if (unit.compare("K") == 0) {
    return lifetime;
  }

  if (unit.compare("meV") == 0) {
    const double k_B = PhysicalConstants::BoltzmannConstant;
    const double e = 1.0; // elementary charge

    const double unitFunc = (e * lifetime) / (1000 * k_B);
    return unitFunc;
  }
}

double Activation::getLifetimeMulti() const {
  auto unit = getAttribute("Unit").asString();
  const double lifetime = getParameter("Lifetime");

  if (unit.compare("K") == 0) {
    return 1.0;
  }

  if (unit.compare("meV") == 0) {
    const double k_B = PhysicalConstants::BoltzmannConstant;

    const double unitFunc = 1 / (1000 * k_B);
    return unitFunc;
  }
}

} // namespace Mantid::CurveFitting::Functions