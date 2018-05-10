#include "MantidCurveFitting/Functions/ComptonProfile.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidCurveFitting/Algorithms/ConvertToYSpace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/PhysicalConstants.h"

#include <gsl/gsl_poly.h>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;
using namespace CurveFitting::Algorithms;

namespace {
///@cond
// const char * WSINDEX_NAME = "WorkspaceIndex";
const char *MASS_NAME = "Mass";
///@endcond
} // namespace

/**
 */
ComptonProfile::ComptonProfile()
    : API::ParamFunction(), API::IFunction1D(), m_log("ComptonProfile"),
      m_wsIndex(0), m_startX(0.0), m_endX(0.0), m_voigt(),
      m_resolutionFunction(), m_yspace(), m_modQ(), m_e0(), m_mass(0.0) {
  using namespace Mantid::API;
  m_resolutionFunction = boost::dynamic_pointer_cast<VesuvioResolution>(
      FunctionFactory::Instance().createFunction("VesuvioResolution"));
}

//-------------------------------------- Function evaluation
//-----------------------------------------

/**
 * Calculates the value of the function for each x value and stores in the given
 * output array
 * @param out An array of size nData to store the results
 * @param xValues The input X data array of size nData. It is assumed to be
 * times in microseconds
 * @param nData The length of the out & xValues arrays
 */
void ComptonProfile::function1D(double *out, const double *xValues,
                                const size_t nData) const {
  UNUSED_ARG(xValues); // Y-space values have already been pre-cached

  this->massProfile(out, nData);

  m_log.setEnabled(false);
}

/*
 * Creates the internal caches
 */
void ComptonProfile::setUpForFit() {

  using namespace Mantid::API;
  m_voigt = boost::dynamic_pointer_cast<IPeakFunction>(
      FunctionFactory::Instance().createFunction("Voigt"));
  m_resolutionFunction->setUpForFit();
}

/**
 * Also caches parameters from the instrument
 * @param workspace The workspace set as input
 * @param wsIndex A workspace index
 * @param startX Starting x-vaue (unused).
 * @param endX Ending x-vaue (unused).
 */
void ComptonProfile::setMatrixWorkspace(
    boost::shared_ptr<const API::MatrixWorkspace> workspace, size_t wsIndex,
    double startX, double endX) {
  auto inst = workspace->getInstrument();
  auto sample = inst->getSample();
  auto source = inst->getSource();
  if (!sample || !source) {
    throw std::invalid_argument(
        "ComptonProfile - Workspace has no source/sample.");
  }
  m_workspace = workspace;
  m_wsIndex = wsIndex;
  m_startX = startX;
  m_endX = endX;

  buildCaches();
}

void ComptonProfile::buildCaches() {
  const auto &spectrumInfo = m_workspace->spectrumInfo();
  if (!spectrumInfo.hasDetectors(m_wsIndex)) {
    throw std::invalid_argument("ComptonProfile - Workspace has no detector "
                                "attached to histogram at index " +
                                std::to_string(m_wsIndex));
  }

  m_resolutionFunction->setAttributeValue("Mass", m_mass);
  m_resolutionFunction->setMatrixWorkspace(m_workspace, m_wsIndex, m_startX,
                                           m_endX);

  Algorithms::DetectorParams detpar =
      ConvertToYSpace::getDetectorParameters(m_workspace, m_wsIndex);
  this->cacheYSpaceValues(m_workspace->points(m_wsIndex), detpar);
}

void ComptonProfile::cacheYSpaceValues(const HistogramData::Points &tseconds,
                                       const Algorithms::DetectorParams &detpar,
                                       const ResolutionParams &respar) {
  m_resolutionFunction->setAttributeValue("Mass", m_mass);
  m_resolutionFunction->cacheResolutionComponents(detpar, respar);
  this->cacheYSpaceValues(tseconds, detpar);
}

/**
 * @param tseconds A vector containing the time-of-flight values in seconds
 * @param detpar Structure containing detector parameters
 */
void ComptonProfile::cacheYSpaceValues(
    const HistogramData::Points &tseconds,
    const Algorithms::DetectorParams &detpar) {

  // ------ Fixed coefficients related to resolution & Y-space transforms
  // ------------------
  const double mevToK = PhysicalConstants::E_mev_toNeutronWavenumberSq;
  const double massToMeV = 0.5 * PhysicalConstants::NeutronMass /
                           PhysicalConstants::meV; // Includes factor of 1/2

  const double v1 = std::sqrt(detpar.efixed / massToMeV);
  const double k1 = std::sqrt(detpar.efixed / mevToK);

  // Calculate energy dependent factors and transform q to Y-space
  const size_t nData = tseconds.size();

  m_e0.resize(nData);
  m_modQ.resize(nData);
  m_yspace.resize(nData);
  for (size_t i = 0; i < nData; ++i) {
    const double tsec = tseconds[i];
    ConvertToYSpace::calculateY(m_yspace[i], m_modQ[i], m_e0[i], m_mass, tsec,
                                k1, v1, detpar);
  }
}

/**
 */
void ComptonProfile::declareParameters() {
  declareParameter(MASS_NAME, 0.0, "Atomic mass (amu)");
}

void ComptonProfile::setParameter(size_t i, const double &value,
                                  bool explicitlySet) {
  ParamFunction::setParameter(i, value, explicitlySet);

  // Mass parameter has changed, need to rebuild Y-space cache
  if (i == parameterIndex(MASS_NAME) && m_mass != value) {
    m_mass = value;
    m_resolutionFunction->setAttributeValue("Mass", m_mass);

    if (m_workspace)
      buildCaches();
  }
}

/**
 * Transforms the input y coordinates using a difference if Voigt functions
 * across the whole range
 * @param voigtDiff [Out] Output values (vector is expected to be of the correct
 * size)
 * @param yspace Input y coordinates
 * @param lorentzPos LorentzPos parameter
 * @param lorentzAmp LorentzAmp parameter
 * @param lorentzWidth LorentzFWHM parameter
 * @param gaussWidth GaussianFWHM parameter
 */
void ComptonProfile::voigtApproxDiff(std::vector<double> &voigtDiff,
                                     const std::vector<double> &yspace,
                                     const double lorentzPos,
                                     const double lorentzAmp,
                                     const double lorentzWidth,
                                     const double gaussWidth) const {
  double miny(DBL_MAX), maxy(-DBL_MAX);
  auto iend = yspace.end();
  for (auto itr = yspace.begin(); itr != iend; ++itr) {
    const double absy = std::abs(*itr);
    if (absy < miny)
      miny = absy;
    else if (absy > maxy)
      maxy = absy;
  }
  const double epsilon = (maxy - miny) / 1000.0;

  // Compute: V = (voigt(y+2eps,...) - voigt(y-2eps,...) - 2*voigt(y+eps,...) +
  // 2*(voigt(y-eps,...))/(2eps^3)

  std::vector<double> ypmEps(yspace.size());
  // y+2eps
  std::transform(
      yspace.begin(), yspace.end(), ypmEps.begin(),
      std::bind2nd(std::plus<double>(), 2.0 * epsilon)); // Add 2 epsilon
  m_resolutionFunction->voigtApprox(voigtDiff, ypmEps, lorentzPos, lorentzAmp,
                                    lorentzWidth, gaussWidth);
  // y-2eps
  std::transform(
      yspace.begin(), yspace.end(), ypmEps.begin(),
      std::bind2nd(std::minus<double>(), 2.0 * epsilon)); // Subtract 2 epsilon
  std::vector<double> tmpResult(yspace.size());
  m_resolutionFunction->voigtApprox(tmpResult, ypmEps, lorentzPos, lorentzAmp,
                                    lorentzWidth, gaussWidth);
  // Difference of first two terms - result is put back in voigtDiff
  std::transform(voigtDiff.begin(), voigtDiff.end(), tmpResult.begin(),
                 voigtDiff.begin(), std::minus<double>());

  // y+eps
  std::transform(yspace.begin(), yspace.end(), ypmEps.begin(),
                 std::bind2nd(std::plus<double>(), epsilon)); // Add epsilon
  m_resolutionFunction->voigtApprox(tmpResult, ypmEps, lorentzPos, lorentzAmp,
                                    lorentzWidth, gaussWidth);
  std::transform(tmpResult.begin(), tmpResult.end(), tmpResult.begin(),
                 std::bind2nd(std::multiplies<double>(), 2.0)); // times 2
  // Difference with 3rd term - result is put back in voigtDiff
  std::transform(voigtDiff.begin(), voigtDiff.end(), tmpResult.begin(),
                 voigtDiff.begin(), std::minus<double>());

  // y-eps
  std::transform(
      yspace.begin(), yspace.end(), ypmEps.begin(),
      std::bind2nd(std::minus<double>(), epsilon)); // Subtract epsilon
  m_resolutionFunction->voigtApprox(tmpResult, ypmEps, lorentzPos, lorentzAmp,
                                    lorentzWidth, gaussWidth);
  std::transform(tmpResult.begin(), tmpResult.end(), tmpResult.begin(),
                 std::bind2nd(std::multiplies<double>(), 2.0)); // times 2
  // Sum final term
  std::transform(voigtDiff.begin(), voigtDiff.end(), tmpResult.begin(),
                 voigtDiff.begin(), std::plus<double>());

  // Finally multiply by 2*eps^3
  std::transform(
      voigtDiff.begin(), voigtDiff.end(), voigtDiff.begin(),
      std::bind2nd(std::divides<double>(),
                   2.0 * std::pow(epsilon, 3))); // divided by (2eps^3)
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
