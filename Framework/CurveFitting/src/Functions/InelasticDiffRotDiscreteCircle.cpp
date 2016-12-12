// Mantid Coding standards <http://www.mantidproject.org/Coding_Standards>
// Main Module Header
#include "MantidCurveFitting/Functions/InelasticDiffRotDiscreteCircle.h"
// Mantid Headers from the same project
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
// Mantid headers from other projects
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/UnitConversion.h"
// 3rd party library headers (N/A)
// standard library headers
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

  this->declareAttribute("Q", API::IFunction::Attribute(EMPTY_DBL()));
  this->declareAttribute("WorkspaceIndex", API::IFunction::Attribute(0));
  this->declareAttribute("N", API::IFunction::Attribute(3));
}

/**
 * @brief Set constraints on fitting parameters
 */
void InelasticDiffRotDiscreteCircle::init() {
  // Ensure positive values for Intensity, Radius, and decay
  auto IntensityConstraint = new BConstraint(
      this, "Intensity", std::numeric_limits<double>::epsilon(), true);
  this->addConstraint(IntensityConstraint);

  auto RadiusConstraint = new BConstraint(
      this, "Radius", std::numeric_limits<double>::epsilon(), true);
  this->addConstraint(RadiusConstraint);

  auto DecayConstraint = new BConstraint(
      this, "Decay", std::numeric_limits<double>::epsilon(), true);
  this->addConstraint(DecayConstraint);
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
  auto S = this->getParameter("Shift");

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
  for (int l = 1; l < N; l++) { // l goes up to N-1
    // notice that 0 < l/N < 1
    ratel[l] = rate * 4 * pow(sin(M_PI * l / N), 2);
  }

  for (size_t i = 0; i < nData; i++) {
    double w = xValues[i] - S;
    double S = 0.0;
    for (int l = 1; l < N; l++) { // l goes up to N-1
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

  size_t numHist = workspace->getNumberHistograms();
  for (size_t idx = 0; idx < numHist; idx++) {
    Mantid::Geometry::IDetector_const_sptr det;
    try {
      det = workspace->getDetector(idx);
    } catch (Kernel::Exception::NotFoundError &) {
      m_qValueCache.clear();
      g_log.information("Cannot populate Q values from workspace");
      break;
    }

    try {
      double efixed = workspace->getEFixed(det);
      double usignTheta = 0.5 * workspace->detectorTwoTheta(*det);

      double q = Mantid::Kernel::UnitConversion::run(usignTheta, efixed);

      m_qValueCache.push_back(q);
    } catch (std::runtime_error &) {
      m_qValueCache.clear();
      g_log.information("Cannot populate Q values from workspace");
      return;
    }
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
