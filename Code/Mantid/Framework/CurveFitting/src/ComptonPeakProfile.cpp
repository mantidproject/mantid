//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidCurveFitting/ComptonPeakProfile.h"
#include "MantidCurveFitting/ConvertToYSpace.h"
#include "MantidAPI/FunctionFactory.h"

namespace Mantid {
namespace CurveFitting {

DECLARE_FUNCTION(ComptonPeakProfile)

namespace {
///@cond
const char *WSINDEX_NAME = "WorkspaceIndex";
const char *MASS_NAME = "Mass";
const char *VOIGT_CUT_OFF = "VoigtEnergyCutOff";

const char *AMP_PARAM = "Intensity";
const char *POS_PARAM = "Position";
const char *WIDTH_PARAM = "SigmaGauss";

/// Conversion constant
const double MASS_TO_MEV =
    0.5 * PhysicalConstants::NeutronMass / PhysicalConstants::meV;
const double STDDEV_TO_FWHM = 0.5 * std::sqrt(std::log(4.0));
///@endcond
}

/**
 */
ComptonPeakProfile::ComptonPeakProfile()
    : API::ParamFunction(), API::IFunction1D(), m_wsIndex(0), m_mass(0.0),
      m_voigtCutOff(5000.), m_gauss(), m_voigt(), m_efixed(0.0),
      m_hwhmLorentz(0.0) {}

//-------------------------------------- Private methods
//-----------------------------------------

/// A string identifier for this function
std::string ComptonPeakProfile::name() const { return "ComptonPeakProfile"; }

/**
 * Calculates the value of the function for each x value and stores in the given
 * output array
 * @param out An array of size nData to store the results
 * @param xValues The input X data array of size nData.
 * @param nData The length of the out & xValues arrays
 */
void ComptonPeakProfile::function1D(double *out, const double *xValues,
                                    const size_t nData) const {
  double amp(getParameter(0)), pos(getParameter(1)),
      gaussSigma(getParameter(2));

  if (m_efixed < m_voigtCutOff) {
    double gaussFWHM(getParameter(2) * STDDEV_TO_FWHM),
        lorentzFWHM(2.0 * m_hwhmLorentz);
    m_voigt->setParameter(0, amp);
    m_voigt->setParameter(1, pos);
    m_voigt->setParameter(2, lorentzFWHM);
    m_voigt->setParameter(3, gaussFWHM);
    m_voigt->functionLocal(out, xValues, nData);
    const double norm = 1.0 / (0.5 * M_PI * lorentzFWHM);
    std::transform(out, out + nData, out,
                   std::bind2nd(std::multiplies<double>(), norm));
  } else {
    double sigmaTotalSq =
        m_hwhmLorentz * m_hwhmLorentz + gaussSigma * gaussSigma;
    // Our gaussian isn't normalised by the width. Here we set the height to
    // amp/(2*sigma^2) so that it will be normalised correctly
    m_gauss->setParameter(0, 0.5 * amp / M_PI / sigmaTotalSq);
    m_gauss->setParameter(1, pos);
    m_gauss->setParameter(2, sqrt(sigmaTotalSq));
    m_gauss->functionLocal(out, xValues, nData);
  }
}

/*
 * Creates the internal caches
 */
void ComptonPeakProfile::setUpForFit() {
  // Voigt & Gaussian
  using namespace Mantid::API;
  m_gauss = boost::dynamic_pointer_cast<IPeakFunction>(
      FunctionFactory::Instance().createFunction("Gaussian"));
  m_voigt = boost::dynamic_pointer_cast<IPeakFunction>(
      FunctionFactory::Instance().createFunction("Voigt"));
}

/**
 * Also caches parameters from the instrument
 * Throws if it is not a MatrixWorkspace
 * @param ws The workspace set as input
 */
void
ComptonPeakProfile::setWorkspace(boost::shared_ptr<const API::Workspace> ws) {
  auto workspace = boost::dynamic_pointer_cast<const API::MatrixWorkspace>(ws);
  if (!workspace) {
    throw std::invalid_argument(
        "ComptonPeakProfile expected an object of type MatrixWorkspace, type=" +
        ws->id());
  }

  DetectorParams detpar =
      ConvertToYSpace::getDetectorParameters(workspace, m_wsIndex);
  m_efixed = detpar.efixed;

  // Recoil time
  const double sth = sin(detpar.theta);
  const double distFact =
      (cos(detpar.theta) + sqrt(m_mass * m_mass - sth * sth)) / (m_mass + 1.0);
  const double ei = detpar.efixed / pow(distFact, 2.0);
  const double v1 = std::sqrt(detpar.efixed / MASS_TO_MEV);
  const double k1 =
      std::sqrt(detpar.efixed / PhysicalConstants::E_mev_toNeutronWavenumberSq);
  const double v2 = std::sqrt(ei / MASS_TO_MEV);
  const double trec = detpar.l1 / v1 + detpar.l2 / v2;

  // Compute lorentz width due to in Y due to spread in energy hwhm_lorentz
  const double dELorentz = ConvertToYSpace::getComponentParameter(
      workspace->getDetector(m_wsIndex), workspace->constInstrumentParameters(),
      "hwhm_lorentz");
  double yplus(0.0), yminus(0.0), dummy(0.0);
  detpar.efixed += dELorentz;
  ConvertToYSpace::calculateY(yplus, dummy, dummy, m_mass, trec, k1, v1,
                              detpar);
  detpar.efixed -= 2.0 * dELorentz;
  ConvertToYSpace::calculateY(yminus, dummy, dummy, m_mass, trec, k1, v1,
                              detpar);
  // lorentzian width
  m_hwhmLorentz = 0.5 * (yplus - yminus);
}

/**
 */
void ComptonPeakProfile::declareParameters() {
  declareParameter(AMP_PARAM, 1.0, "Intensity parameter");
  declareParameter(POS_PARAM, 1.0, "Peak position parameter");
  declareParameter(WIDTH_PARAM, 1.0, "Width parameter");
}

/**
 */
void ComptonPeakProfile::declareAttributes() {
  declareAttribute(WSINDEX_NAME,
                   IFunction::Attribute(static_cast<int>(m_wsIndex)));
  declareAttribute(MASS_NAME, IFunction::Attribute(m_mass));
  declareAttribute(VOIGT_CUT_OFF, IFunction::Attribute(m_voigtCutOff));
}

/**
 * @param name The name of the attribute
 * @param value The attribute's value
 */
void ComptonPeakProfile::setAttribute(const std::string &name,
                                      const Attribute &value) {
  IFunction::setAttribute(name, value); // Make sure the base-class stores it
  if (name == WSINDEX_NAME)
    m_wsIndex = static_cast<size_t>(value.asInt());
  else if (name == MASS_NAME)
    m_mass = value.asDouble();
  else if (name == VOIGT_CUT_OFF)
    m_voigtCutOff = value.asDouble();
}

} // namespace CurveFitting
} // namespace Mantid
