// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "MantidCurveFitting/Functions/TeixeiraWaterSQE.h"
// Mantid Headers from the same project
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
// Mantid headers from other projects
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
// third party library headers
// standard library headers
#include <cmath>
#include <limits>

using BConstraint = Mantid::CurveFitting::Constraints::BoundaryConstraint;

namespace {
Mantid::Kernel::Logger g_log("TeixeiraWaterSQE");
}

namespace Mantid {
namespace CurveFitting {
namespace Functions {

DECLARE_FUNCTION(TeixeiraWaterSQE)

/**
 * @brief Constructor where parameters and attributes are declared
 */
TeixeiraWaterSQE::TeixeiraWaterSQE() {
  this->declareParameter("Height", 1.0, "scaling factor");
  this->declareParameter("DiffCoeff", 2.3,
                         "Diffusion coefficient (10^(-5)cm^2/s)");
  this->declareParameter("Tau", 1.25, "Residence time (ps)");
  this->declareParameter("Centre", 0.0, "Shift along the X-axis");
  // Momentum transfer Q, an attribute (not a fitting parameter)
  this->declareAttribute("Q", API::IFunction::Attribute(0.3));
}

/**
 * @brief Set constraints on fitting parameters
 */
void TeixeiraWaterSQE::init() {
  // Ensure positive values for Height, Length, and Tau
  auto HeightConstraint = new BConstraint(
      this, "Height", std::numeric_limits<double>::epsilon(), true);
  this->addConstraint(HeightConstraint);
  auto DiffCoeffConstraint = new BConstraint(
      this, "DiffCoeff", std::numeric_limits<double>::epsilon(), true);
  this->addConstraint(DiffCoeffConstraint);
  auto TauConstraint = new BConstraint(
      this, "Tau", std::numeric_limits<double>::epsilon(), true);
  this->addConstraint(TauConstraint);
}

/**
 * @brief Calculate function values on an energy domain
 * @param out array to store function values
 * @param xValues energy domain where function is evaluated
 * @param nData size of the energy domain
 */
void TeixeiraWaterSQE::function1D(double *out, const double *xValues,
                                  const size_t nData) const {
  double hbar(0.658211626); // ps*meV
  auto H = this->getParameter("Height");
  auto D = this->getParameter("DiffCoeff");
  auto T = this->getParameter("Tau");
  auto C = this->getParameter("Centre");
  auto Q = this->getAttribute("Q").asDouble();

  // Penalize negative parameters, just in case they show up
  // when calculating the numeric derivative
  if (H < std::numeric_limits<double>::epsilon() ||
      D < std::numeric_limits<double>::epsilon() ||
      T < std::numeric_limits<double>::epsilon()) {
    for (size_t j = 0; j < nData; j++) {
      out[j] = std::numeric_limits<double>::infinity();
    }
    return;
  }

  // Lorentzian intensities and HWHM
  D *= 0.10; // conversion from 10^{-5}cm^2/s to Angstrom^2/ps, the internal
             // units used
  auto G = hbar * D * Q * Q / (1 + D * Q * Q * T);
  for (size_t j = 0; j < nData; j++) {
    auto E = xValues[j] - C;
    out[j] += H * G / (G * G + E * E) / M_PI;
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
