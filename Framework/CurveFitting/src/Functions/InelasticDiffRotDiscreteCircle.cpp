// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/InelasticDiffRotDiscreteCircle.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/UnitConversion.h"

#include "MantidTypes/SpectrumDefinition.h"

#include <cmath>
#include <limits>
#include <sstream>

using BConstraint = Mantid::CurveFitting::Constraints::BoundaryConstraint;

namespace {
Mantid::Kernel::Logger g_log("InelasticDiffDiffRotDiscreteCircle");
}

namespace Mantid {
namespace CurveFitting {
namespace Functions {

DECLARE_FUNCTION(InelasticDiffRotDiscreteCircle)

/**
 * @brief Constructor. Declare fitting parameters and attributes
 */
InelasticDiffRotDiscreteCircle::InelasticDiffRotDiscreteCircle()
    : m_hbar(0.658211626) {
  this->declareParameter("Intensity", 1.0, "scaling factor [no units]");
  this->declareParameter("Radius", 1.0, "Circle radius [Angstroms]");
  this->declareParameter("Decay", 1.0,
                         "Inverse of transition rate, in nanoseconds "
                         "if energy in micro-ev, or picoseconds if "
                         "energy in mili-eV");
  this->declareParameter("Shift", 0.0, "Shift in the centre of the peak");

  this->declareAttribute("N", API::IFunction::Attribute(3));
  declareAttributes();
}

/**
 * @brief Set constraints on fitting parameters
 */
void InelasticDiffRotDiscreteCircle::init() {
  // Ensure positive values for Intensity, Radius, and decay
  auto IntensityConstraint = std::make_unique<BConstraint>(
      this, "Intensity", std::numeric_limits<double>::epsilon(), true);
  this->addConstraint(std::move(IntensityConstraint));

  auto RadiusConstraint = std::make_unique<BConstraint>(
      this, "Radius", std::numeric_limits<double>::epsilon(), true);
  this->addConstraint(std::move(RadiusConstraint));

  auto DecayConstraint = std::make_unique<BConstraint>(
      this, "Decay", std::numeric_limits<double>::epsilon(), true);
  this->addConstraint(std::move(DecayConstraint));
}

/**
 * @brief Calculate function values on an energy domain
 * @param out array to store function values
 * @param xValues energy domain where function is evaluated
 * @param nData size of the energy domain
 * @exception No Q values can be found in associated attributes
 */
void InelasticDiffRotDiscreteCircle::function1D(double *out,
                                                const double *xValues,
                                                const size_t nData) const {

  auto I = this->getParameter("Intensity");
  auto R = this->getParameter("Radius");
  auto rate = m_hbar / this->getParameter("Decay"); // micro-eV or mili-eV
  auto N = this->getAttribute("N").asInt();

  // Retrieve Q-value from the appropriate attribute
  double Q;
  if (this->getAttribute("Q").asDouble() == EMPTY_DBL()) {
    if (m_qValueCache.empty()) {
      throw std::runtime_error(
          "No Q attribute provided and cannot retrieve from workspace.");
    }
    const int specIdx = this->getAttribute("WorkspaceIndex").asInt();
    Q = m_qValueCache[specIdx];

    g_log.debug() << "Get Q value for workspace index " << specIdx << ": " << Q
                  << '\n';
  } else {
    Q = getAttribute("Q").asDouble();

    g_log.debug() << "Using Q attribute: " << Q << '\n';
  }

  std::vector<double> sph(N);
  for (int k = 1; k < N; k++) {
    double x = 2 * Q * R * sin(M_PI * k / N);
    // spherical Besell function of order zero 'j0' is sin(x)/x
    sph[k] = sin(x) / x;
  }

  std::vector<double> ratel(N);
  for (int l = 1; l < N; l++) {
    // notice that 0 < l/N < 1
    ratel[l] = rate * 4 * pow(sin(M_PI * l / N), 2);
  }

  const auto shift = this->getParameter("Shift");
  for (size_t i = 0; i < nData; i++) {
    double w = xValues[i] - shift;
    double S = 0.0;
    for (int l = 1; l < N; l++) {
      double lorentzian = ratel[l] / (ratel[l] * ratel[l] + w * w);
      double al = 0.0;
      for (int k = 1; k < N; k++) { // case k==N after the loop
        double y = 2 * M_PI * l * k / N;
        al += cos(y) * sph[k];
      }
      al += 1; // limit for j0 when k==N, or x==0
      al /= N;
      S += al * lorentzian;
    }
    out[i] = I * S / M_PI;
  }
}

/**
 * @brief Creates a list of Q values from each spectrum
 * to be used with WorkspaceIndex attribute.
 * @param ws Pointer to workspace
 */
void InelasticDiffRotDiscreteCircle::setWorkspace(
    boost::shared_ptr<const API::Workspace> ws) {
  m_qValueCache.clear();

  auto workspace = boost::dynamic_pointer_cast<const API::MatrixWorkspace>(ws);
  if (!workspace)
    return;

  const auto &spectrumInfo = workspace->spectrumInfo();
  const auto &detectorIDs = workspace->detectorInfo().detectorIDs();
  for (size_t idx = 0; idx < spectrumInfo.size(); idx++) {
    if (!spectrumInfo.hasDetectors(idx)) {
      m_qValueCache.clear();
      g_log.information(
          "Cannot populate Q values from workspace - no detectors set.");
      break;
    }

    const auto detectorIndex = spectrumInfo.spectrumDefinition(idx)[0].first;

    try {
      double efixed = workspace->getEFixed(detectorIDs[detectorIndex]);
      double usingTheta = 0.5 * spectrumInfo.twoTheta(idx);

      double q =
          Mantid::Kernel::UnitConversion::convertToElasticQ(usingTheta, efixed);

      m_qValueCache.push_back(q);
    } catch (std::runtime_error &) {
      m_qValueCache.clear();
      g_log.information("Cannot populate Q values from workspace - could not "
                        "find EFixed value.");
      return;
    }
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
