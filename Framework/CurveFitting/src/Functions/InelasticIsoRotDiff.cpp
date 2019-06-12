// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "MantidCurveFitting/Functions/InelasticIsoRotDiff.h"
// Mantid Headers from the same project
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
// Mantid headers from other projects
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"

// third party library headers
#include <boost/math/special_functions/bessel.hpp>
// standard library headers
#include <cmath>
#include <limits>

using BConstraint = Mantid::CurveFitting::Constraints::BoundaryConstraint;

namespace {
Mantid::Kernel::Logger g_log("IsoRotDiff");
}

namespace Mantid {
namespace CurveFitting {
namespace Functions {

DECLARE_FUNCTION(InelasticIsoRotDiff)

/**
 * @brief Constructor where parameters and attributes are declared
 */
InelasticIsoRotDiff::InelasticIsoRotDiff() {
  this->declareParameter("Height", 1.0, "scaling factor");
  this->declareParameter("Radius", 0.98, "radius of rotation (Angstroms)");
  this->declareParameter(
      "Tau", 10.0,
      "Relaxation time, inverse of the rotational diffusion coefficient (ps)");
  this->declareParameter("Centre", 0.0, "Shift along the X-axis");

  this->declareAttribute("Q", API::IFunction::Attribute(0.3));
  this->declareAttribute("N", API::IFunction::Attribute(25));
}

/**
 * @brief Set constraints on fitting parameters
 */
void InelasticIsoRotDiff::init() {
  // Ensure positive values for Height, Radius, and Diffusion constant
  auto HeightConstraint = std::make_unique<BConstraint>(
      this, "Height", std::numeric_limits<double>::epsilon(), true);
  this->addConstraint(std::move(HeightConstraint));
  auto RadiusConstraint = std::make_unique<BConstraint>(
      this, "Radius", std::numeric_limits<double>::epsilon(), true);
  this->addConstraint(std::move(RadiusConstraint));
  auto DiffusionConstraint = std::make_unique<BConstraint>(
      this, "Tau", std::numeric_limits<double>::epsilon(), true);
  this->addConstraint(std::move(DiffusionConstraint));
}

/**
 * @brief Calculate function values on an energy domain
 * @param out array to store function values
 * @param xValues energy domain where function is evaluated
 * @param nData size of the energy domain
 */
void InelasticIsoRotDiff::function1D(double *out, const double *xValues,
                                     const size_t nData) const {
  double hbar(0.658211626); // ps*meV
  auto H = this->getParameter("Height");
  auto R = this->getParameter("Radius");
  auto T = this->getParameter("Tau");
  auto C = this->getParameter("Centre");
  auto Q = this->getAttribute("Q").asDouble();
  auto N = static_cast<size_t>(
      this->getAttribute("N").asInt()); // Number of Lorentzians

  // Penalize negative parameters
  if (R < std::numeric_limits<double>::epsilon()) {
    for (size_t j = 0; j < nData; j++) {
      out[j] = std::numeric_limits<double>::infinity();
    }
    return;
  }

  // Lorentzian intensities and HWHM
  std::vector<double> al(N);
  std::vector<double> HWHM(N);
  for (size_t i = 0; i < N; i++) {
    auto l = static_cast<unsigned int>(
        i + 1); // avoid annoying warnings from implicit type conversion
    auto ld = static_cast<double>(
        l); // avoid annoying warnings from implicit type conversion
    al[i] = (2 * ld + 1) * pow(boost::math::sph_bessel(l, Q * R), 2);
    HWHM[i] = ld * (ld + 1) * hbar / T;
  }

  for (size_t j = 0; j < nData; j++) {
    out[j] = 0.0;
    auto E = xValues[j] - C;
    for (size_t i = 0; i < N; i++) {
      auto G = HWHM[i];
      out[j] += H * al[i] * G / (G * G + E * E) / M_PI;
    }
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
