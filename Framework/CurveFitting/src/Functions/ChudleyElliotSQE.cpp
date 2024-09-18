// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/ChudleyElliotSQE.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/Jacobian.h"
#include <cmath>
#include <limits>

namespace {
Mantid::Kernel::Logger g_log("ChudleyElliotSQE");
}

namespace Mantid::CurveFitting::Functions {

DECLARE_FUNCTION(ChudleyElliotSQE)

/**
 * @brief function parameters and impose constraints
 */
void ChudleyElliotSQE::declareParameters() {
  this->declareParameter("Height", 1.0, "scaling factor");
  this->declareParameter("Centre", 0.0, "Shift along the X-axis");
  this->declareParameter("Tau", 1.25, "Residence time");
  this->declareParameter("L", 1.25, "Jump length");
}

/**
 * @brief Calculate function values on an energy domain
 * @param out array to store function values
 * @param xValues energy domain where function is evaluated
 * @param nData size of the energy domain
 */
void ChudleyElliotSQE::function1D(double *out, const double *xValues, const size_t nData) const {
  double hbar(0.658211626); // ps*meV
  auto H = this->getParameter("Height");
  auto C = this->getParameter("Centre");
  auto Q = this->getAttribute("Q").asDouble();
  auto T = this->getParameter("Tau");
  auto L = this->getParameter("L");

  // Penalize negative parameters, just in case they show up
  // when calculating the numeric derivative
  if (H < std::numeric_limits<double>::epsilon() || T < std::numeric_limits<double>::epsilon()) {
    for (size_t j = 0; j < nData; j++) {
      out[j] = std::numeric_limits<double>::infinity();
    }
    return;
  }

  // Lorentzian intensities and HWHM
  auto G = hbar * (1 - (sin(Q * L) / (Q * L))) / T;
  for (size_t j = 0; j < nData; j++) {
    auto E = xValues[j] - C;
    out[j] += H * G / (G * G + E * E) / M_PI;
  }
}

/**
 * @brief analytical/numerical derivative with respect to fitting parameters
 * We carry out analytical derivative for parameter Height and numerical
 * derivatives for parameters
 * DiffCoeff, Tau, and Centre, selecting sensible steps.
 */
void ChudleyElliotSQE::functionDeriv1D(Mantid::API::Jacobian *jacobian, const double *xValues, const size_t nData) {
  const double deltaF{0.1}; // increase parameter by this fraction
  const size_t nParam = this->nParams();
  // cutoff defines the smallest change in the parameter when calculating the
  // numerical derivative
  std::map<std::string, double> cutoff;
  cutoff["Tau"] = 0.2;       // 0.2ps
  cutoff["Centre"] = 0.0001; // 0.1micro-eV
  std::vector<double> out(nData);
  this->applyTies();
  this->function1D(out.data(), xValues, nData);

  for (size_t iP = 0; iP < nParam; iP++) {
    if (this->isActive(iP)) {
      std::vector<double> derivative(nData);
      const double pVal = this->activeParameter(iP);
      const std::string pName = this->parameterName(iP);
      if (pName == "Height") {
        // exact derivative
        this->setActiveParameter(iP, 1.0);
        this->applyTies();
        this->function1D(derivative.data(), xValues, nData);
      } else {
        // numerical derivative
        double delta = cutoff[pName] > fabs(pVal * deltaF) ? cutoff[pName] : pVal * deltaF;
        this->setActiveParameter(iP, pVal + delta);
        this->applyTies();
        this->function1D(derivative.data(), xValues, nData);
        for (size_t i = 0; i < nData; i++) {
          derivative[i] = (derivative[i] - out[i]) / delta;
        }
      }
      this->setActiveParameter(iP, pVal); // restore the value of the parameter
      // fill the jacobian for this parameter
      for (size_t i = 0; i < nData; i++) {
        jacobian->set(i, iP, derivative[i]);
      }
    }
  }
}

} // namespace Mantid::CurveFitting::Functions
