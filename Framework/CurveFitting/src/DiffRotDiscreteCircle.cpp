//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DiffRotDiscreteCircle.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidGeometry/IDetector.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/UnitConversion.h"

#include <cmath>
#include <limits>

namespace {
Mantid::Kernel::Logger g_log("DiffSphere");
}

namespace Mantid {
namespace CurveFitting {

using namespace API;
using namespace Geometry;

DECLARE_FUNCTION(ElasticDiffRotDiscreteCircle)
DECLARE_FUNCTION(InelasticDiffRotDiscreteCircle)
DECLARE_FUNCTION(DiffRotDiscreteCircle)

ElasticDiffRotDiscreteCircle::ElasticDiffRotDiscreteCircle() {
  // declareParameter("Height", 1.0); //parameter "Height" already declared in
  // constructor of base class DeltaFunction
  declareParameter("Radius", 1.0, "Circle radius [Angstroms] ");
  declareAttribute("Q", API::IFunction::Attribute(0.5));
  declareAttribute("N", API::IFunction::Attribute(3));
}

void ElasticDiffRotDiscreteCircle::init() {
  // Ensure positive values for Height and Radius
  BoundaryConstraint *HeightConstraint = new BoundaryConstraint(
      this, "Height", std::numeric_limits<double>::epsilon(), true);
  addConstraint(HeightConstraint);

  BoundaryConstraint *RadiusConstraint = new BoundaryConstraint(
      this, "Radius", std::numeric_limits<double>::epsilon(), true);
  addConstraint(RadiusConstraint);
}

double ElasticDiffRotDiscreteCircle::HeightPrefactor() const {
  const double R = getParameter("Radius");
  const double Q = getAttribute("Q").asDouble();
  const int N = getAttribute("N").asInt();
  double aN = 0;
  for (int k = 1; k < N; k++) {
    double x = 2 * Q * R * sin(M_PI * k / N);
    aN += sin(x) / x; // spherical Besell function of order zero j0==sin(x)/x
  }
  aN += 1; // limit for j0 when k==N, or x==0
  return aN / N;
}

InelasticDiffRotDiscreteCircle::InelasticDiffRotDiscreteCircle()
    : m_hbar(0.658211626) {
  declareParameter("Intensity", 1.0, "scaling factor [arbitrary units]");
  declareParameter("Radius", 1.0, "Circle radius [Angstroms]");
  declareParameter("Decay", 1.0, "Inverse of transition rate, in nanoseconds "
                                 "if energy in micro-ev, or picoseconds if "
                                 "energy in mili-eV");
  declareParameter("Shift", 0.0, "Shift in domain");

  declareAttribute("Q", API::IFunction::Attribute(EMPTY_DBL()));
  declareAttribute("WorkspaceIndex", API::IFunction::Attribute(0));
  declareAttribute("N", API::IFunction::Attribute(3));
}

void InelasticDiffRotDiscreteCircle::init() {
  // Ensure positive values for Intensity, Radius, and decay
  BoundaryConstraint *IntensityConstraint = new BoundaryConstraint(
      this, "Intensity", std::numeric_limits<double>::epsilon(), true);
  addConstraint(IntensityConstraint);

  BoundaryConstraint *RadiusConstraint = new BoundaryConstraint(
      this, "Radius", std::numeric_limits<double>::epsilon(), true);
  addConstraint(RadiusConstraint);

  BoundaryConstraint *DecayConstraint = new BoundaryConstraint(
      this, "Decay", std::numeric_limits<double>::epsilon(), true);
  addConstraint(DecayConstraint);
}

void InelasticDiffRotDiscreteCircle::function1D(double *out,
                                                const double *xValues,
                                                const size_t nData) const {
  const double I = getParameter("Intensity");
  const double R = getParameter("Radius");
  const double rate = m_hbar / getParameter("Decay"); // micro-eV or mili-eV
  const int N = getAttribute("N").asInt();
  const double S = getParameter("Shift");

  double Q;
  if (getAttribute("Q").asDouble() == EMPTY_DBL()) {
    if (m_qValueCache.size() == 0) {
      throw std::runtime_error(
          "No Q attribute provided and cannot retrieve from worksapce.");
    }
    const int specIdx = getAttribute("WorkspaceIndex").asInt();
    Q = m_qValueCache[specIdx];

    g_log.debug() << "Get Q value for workspace index " << specIdx << ": " << Q
                  << std::endl;
  } else {
    Q = getAttribute("Q").asDouble();

    g_log.debug() << "Using Q attribute: " << Q << std::endl;
  }

  std::vector<double> sph(N);
  for (int k = 1; k < N; k++) {
    double x = 2 * Q * R * sin(M_PI * k / N);
    sph[k] =
        sin(x) / x; // spherical Besell function of order zero 'j0' is sin(x)/x
  }

  std::vector<double> ratel(N);
  for (int l = 1; l < N; l++) // l goes up to N-1
  {
    ratel[l] = rate * 4 * pow(sin(M_PI * l / N), 2); // notice that 0 < l/N < 1
  }

  for (size_t i = 0; i < nData; i++) {
    double w = xValues[i] - S;
    double S = 0.0;
    for (int l = 1; l < N; l++) // l goes up to N-1
    {
      double lorentzian = ratel[l] / (ratel[l] * ratel[l] + w * w);
      double al = 0.0;
      for (int k = 1; k < N; k++) // case k==N after the loop
      {
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
 * Handle seting fit workspace.
 *
 * Creates a list of Q values from each spectrum to be used with WorkspaceIndex
 * attribute.
 *
 * @param ws Pointer to workspace
 */
void InelasticDiffRotDiscreteCircle::setWorkspace(
    boost::shared_ptr<const API::Workspace> ws) {
  m_qValueCache.clear();

  auto workspace = boost::dynamic_pointer_cast<const MatrixWorkspace>(ws);
  if (!workspace)
    return;

  size_t numHist = workspace->getNumberHistograms();
  for (size_t idx = 0; idx < numHist; idx++) {
    IDetector_const_sptr det;
    try {
      det = workspace->getDetector(idx);
    } catch (Kernel::Exception::NotFoundError &) {
      m_qValueCache.clear();
      g_log.information("Cannot populate Q values from workspace");
      break;
    }

    try {
      double efixed = workspace->getEFixed(det);
      double usignTheta = workspace->detectorTwoTheta(det) / 2.0;

      double q = Mantid::Kernel::UnitConversion::run(usignTheta, efixed);

      m_qValueCache.push_back(q);
    } catch (std::runtime_error &) {
      m_qValueCache.clear();
      g_log.information("Cannot populate Q values from workspace");
      return;
    }
  }
}

/* Propagate the attribute to its member functions.
 * NOTE: we pass this->getAttribute(name) by reference, thus the same
 * object is shared by the composite function and its members.
 */
void DiffRotDiscreteCircle::trickleDownAttribute(const std::string &name) {
  for (size_t iFun = 0; iFun < nFunctions(); iFun++) {
    API::IFunction_sptr fun = getFunction(iFun);
    if (fun->hasAttribute(name))
      fun->setAttribute(name, this->getAttribute(name));
  }
}

/* Same as parent except we overwrite attributes of member functions
 * having the same name
 */
void DiffRotDiscreteCircle::declareAttribute(
    const std::string &name, const API::IFunction::Attribute &defaultValue) {
  API::ImmutableCompositeFunction::declareAttribute(name, defaultValue);
  trickleDownAttribute(name);
}

/* Same as parent except we overwrite attributes of member functions
 * having the same name
 */
void DiffRotDiscreteCircle::setAttribute(const std::string &name,
                                         const Attribute &att) {
  API::ImmutableCompositeFunction::setAttribute(name, att);
  trickleDownAttribute(name);
}

// DiffRotDiscreteCircle::DiffRotDiscreteCircle()
void DiffRotDiscreteCircle::init() {
  m_elastic = boost::dynamic_pointer_cast<ElasticDiffRotDiscreteCircle>(
      API::FunctionFactory::Instance().createFunction(
          "ElasticDiffRotDiscreteCircle"));
  addFunction(m_elastic);
  m_inelastic = boost::dynamic_pointer_cast<InelasticDiffRotDiscreteCircle>(
      API::FunctionFactory::Instance().createFunction(
          "InelasticDiffRotDiscreteCircle"));
  addFunction(m_inelastic);

  setAttributeValue("NumDeriv", true);

  declareAttribute("Q", API::IFunction::Attribute(0.5));
  declareAttribute("N", API::IFunction::Attribute(3));

  // Set the aliases
  setAlias("f1.Intensity", "Intensity");
  setAlias("f1.Radius", "Radius");
  setAlias("f1.Decay", "Decay");
  setAlias("f1.Shift", "Shift");

  // Set the ties between Elastic and Inelastic parameters
  addDefaultTies("f0.Height=f1.Intensity,f0.Radius=f1.Radius");
  applyTies();
}

} // namespace CurveFitting
} // namespace Mantid
