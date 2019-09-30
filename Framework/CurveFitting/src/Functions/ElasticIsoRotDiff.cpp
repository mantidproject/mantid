// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
// Mantid Coding standars <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "MantidCurveFitting/Functions/ElasticIsoRotDiff.h"
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

DECLARE_FUNCTION(ElasticIsoRotDiff)

/**
 * @brief Constructor where fitting parameters are declared
 */
ElasticIsoRotDiff::ElasticIsoRotDiff() {
  // parameter "Height" declared in parent DeltaFunction constructor
  // declareParameter("Height", 1.0);
  declareParameter("Radius", 0.98, "Radius of rotation (Angstroms)");
  declareAttribute("Q", API::IFunction::Attribute(0.3));
}

/**
 * @brief Set constraints on fitting parameters
 */
void ElasticIsoRotDiff::init() {
  // Ensure positive values for Height and Radius
  auto HeightConstraint = std::make_unique<BConstraint>(
      this, "Height", std::numeric_limits<double>::epsilon(), true);
  this->addConstraint(std::move(HeightConstraint));

  auto RadiusConstraint = std::make_unique<BConstraint>(
      this, "Radius", std::numeric_limits<double>::epsilon(), true);
  this->addConstraint(std::move(RadiusConstraint));
}

/**
 * @brief Calculate intensity of the elastic signal
 */
double ElasticIsoRotDiff::HeightPrefactor() const {
  const double R = this->getParameter("Radius");
  const double Q = this->getAttribute("Q").asDouble();

  // Penalize negative parameters
  if (R < std::numeric_limits<double>::epsilon()) {
    return std::numeric_limits<double>::infinity();
  }

  return pow(boost::math::sph_bessel(0, Q * R), 2);
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
