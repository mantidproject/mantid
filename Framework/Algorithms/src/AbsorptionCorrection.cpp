// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/AbsorptionCorrection.h"
#include "MantidAPI/HistoWorkspace.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidHistogramData/Interpolate.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Fast_Exponential.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid {
namespace Algorithms {

using namespace API;
using namespace Geometry;
using HistogramData::interpolateLinearInplace;
using namespace Kernel;
using namespace Mantid::PhysicalConstants;
using namespace Mantid::DataObjects;

AbsorptionCorrection::AbsorptionCorrection()
    : API::Algorithm(), m_inputWS(), m_sampleObject(nullptr), m_L1s(),
      m_elementVolumes(), m_elementPositions(), m_numVolumeElements(0),
      m_sampleVolume(0.0), m_refAtten(0.0), m_scattering(0), n_lambda(0),
      m_xStep(0), m_emode(0), m_lambdaFixed(0.), EXPONENTIAL() {}

void AbsorptionCorrection::init() {

  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();

  declareProperty(
      make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input,
                                       wsValidator),
      "The X values for the input workspace must be in units of wavelength");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Output workspace name");

  auto mustBePositive = boost::make_shared<BoundedValidator<double>>();
  mustBePositive->setLower(0.0);
  declareProperty("AttenuationXSection", EMPTY_DBL(), mustBePositive,
                  "The ABSORPTION cross-section, at 1.8 Angstroms, for the "
                  "sample material in barns. Column 8 of a table generated "
                  "from http://www.ncnr.nist.gov/resources/n-lengths/.");
  declareProperty("ScatteringXSection", EMPTY_DBL(), mustBePositive,
                  "The (coherent + incoherent) scattering cross-section for "
                  "the sample material in barns. Column 7 of a table generated "
                  "from http://www.ncnr.nist.gov/resources/n-lengths/.");
  declareProperty("SampleNumberDensity", EMPTY_DBL(), mustBePositive,
                  "The number density of the sample in number of atoms per "
                  "cubic angstrom if not set with SetSampleMaterial");

  auto positiveInt = boost::make_shared<BoundedValidator<int64_t>>();
  positiveInt->setLower(1);
  declareProperty(
      "NumberOfWavelengthPoints", static_cast<int64_t>(EMPTY_INT()),
      positiveInt,
      "The number of wavelength points for which the numerical integral is\n"
      "calculated (default: all points)");

  std::vector<std::string> exp_options{"Normal", "FastApprox"};
  declareProperty(
      "ExpMethod", "Normal",
      boost::make_shared<StringListValidator>(exp_options),
      "Select the method to use to calculate exponentials, normal or a\n"
      "fast approximation (default: Normal)");

  std::vector<std::string> propOptions{"Elastic", "Direct", "Indirect"};
  declareProperty("EMode", "Elastic",
                  boost::make_shared<StringListValidator>(propOptions),
                  "The energy mode (default: elastic)");
  declareProperty(
      "EFixed", 0.0, mustBePositive,
      "The value of the initial or final energy, as appropriate, in meV.\n"
      "Will be taken from the instrument definition file, if available.");

  // Call the virtual method for concrete algorithm to define any other
  // properties
  defineProperties();
}

void AbsorptionCorrection::exec() {
  // Retrieve the input workspace
  m_inputWS = getProperty("InputWorkspace");
  // Cache the beam direction
  m_beamDirection = m_inputWS->getInstrument()->getBeamDirection();
  // Get a reference to the parameter map (used for indirect instruments)
  const ParameterMap &pmap = m_inputWS->constInstrumentParameters();

  // Get the input parameters
  retrieveBaseProperties();

  // Create the output workspace
  MatrixWorkspace_sptr correctionFactors = create<HistoWorkspace>(*m_inputWS);
  correctionFactors->setDistribution(
      true);                       // The output of this is a distribution
  correctionFactors->setYUnit(""); // Need to explicitly set YUnit to nothing
  correctionFactors->setYUnitLabel("Attenuation factor");

  constructSample(correctionFactors->mutableSample());

  const int64_t numHists =
      static_cast<int64_t>(m_inputWS->getNumberHistograms());
  const int64_t specSize = static_cast<int64_t>(m_inputWS->blocksize());

  // If the number of wavelength points has not been given, use them all
  if (isEmpty(n_lambda))
    n_lambda = specSize;
  m_xStep = specSize / n_lambda; // Bin step between points to calculate

  if (m_xStep == 0) // Number of wavelength points >number of histogram points
    m_xStep = 1;

  std::ostringstream message;
  message << "Numerical integration performed every " << m_xStep
          << " wavelength points";
  g_log.information(message.str());
  message.str("");

  // Calculate the cached values of L1 and element volumes.
  initialiseCachedDistances();
  if (m_L1s.empty()) {
    throw std::runtime_error(
        "Failed to define any initial scattering gauge volume for geometry");
  }

  const auto &spectrumInfo = m_inputWS->spectrumInfo();
  Progress prog(this, 0.0, 1.0, numHists);
  // Loop over the spectra
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS, *correctionFactors))
  for (int64_t i = 0; i < int64_t(numHists); ++i) {
    PARALLEL_START_INTERUPT_REGION

    // Copy over bins
    correctionFactors->setSharedX(i, m_inputWS->sharedX(i));

    if (!spectrumInfo.hasDetectors(i))
      continue;

    const auto &det = spectrumInfo.detector(i);

    std::vector<double> L2s(m_numVolumeElements);
    calculateDistances(det, L2s);

    // If an indirect instrument, see if there's an efixed in the parameter map
    double lambda_f = m_lambdaFixed;
    if (m_emode == 2) {
      try {
        Parameter_sptr par = pmap.get(&det, "Efixed");
        if (par) {
          Unit_const_sptr energy = UnitFactory::Instance().create("Energy");
          double factor, power;
          energy->quickConversion(*UnitFactory::Instance().create("Wavelength"),
                                  factor, power);
          lambda_f = factor * std::pow(par->value<double>(), power);
        }
      } catch (std::runtime_error &) { /* Throws if a DetectorGroup, use single
                                          provided value */
      }
    }

    const auto lambdas = m_inputWS->points(i);
    // Get a reference to the Y's in the output WS for storing the factors
    auto &Y = correctionFactors->mutableY(i);

    // Loop through the bins in the current spectrum every m_xStep
    for (int64_t j = 0; j < specSize; j = j + m_xStep) {
      const double lambda = lambdas[j];
      if (m_emode == 0) // Elastic
      {
        Y[j] = this->doIntegration(lambda, L2s);
      } else if (m_emode == 1) // Direct
      {
        Y[j] = this->doIntegration(m_lambdaFixed, lambda, L2s);
      } else if (m_emode == 2) // Indirect
      {
        Y[j] = this->doIntegration(lambda, lambda_f, L2s);
      }
      Y[j] /= m_sampleVolume; // Divide by total volume of the cylinder

      // Make certain that last point is calculates
      if (m_xStep > 1 && j + m_xStep >= specSize && j + 1 != specSize) {
        j = specSize - m_xStep - 1;
      }
    }

    // Interpolate linearly between points separated by m_xStep,
    // last point required
    if (m_xStep > 1) {
      auto histnew = correctionFactors->histogram(i);
      interpolateLinearInplace(histnew, m_xStep);
      correctionFactors->setHistogram(i, histnew);
    }

    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  g_log.information() << "Total number of elements in the integration was "
                      << m_L1s.size() << '\n';
  setProperty("OutputWorkspace", correctionFactors);

  // Now do some cleaning-up since destructor may not be called immediately
  m_L1s.clear();
  m_elementVolumes.clear();
  m_elementPositions.clear();
}

/// Fetch the properties and set the appropriate member variables
void AbsorptionCorrection::retrieveBaseProperties() {
  double sigma_atten = getProperty("AttenuationXSection"); // in barns
  double sigma_s = getProperty("ScatteringXSection");      // in barns
  double rho = getProperty("SampleNumberDensity");         // in Angstroms-3
  const Material &sampleMaterial = m_inputWS->sample().getShape().material();
  if (sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda) !=
      0.0) {
    if (rho == EMPTY_DBL())
      rho = sampleMaterial.numberDensity();
    if (sigma_s == EMPTY_DBL())
      sigma_s =
          sampleMaterial.totalScatterXSection(NeutronAtom::ReferenceLambda);
    if (sigma_atten == EMPTY_DBL())
      sigma_atten = sampleMaterial.absorbXSection(NeutronAtom::ReferenceLambda);
  } else // Save input in Sample with wrong atomic number and name
  {
    NeutronAtom neutron(0, 0, 0.0, 0.0, sigma_s, 0.0, sigma_s, sigma_atten);

    auto shape = boost::shared_ptr<IObject>(
        m_inputWS->sample().getShape().cloneWithMaterial(
            Material("SetInAbsorptionCorrection", neutron, rho)));
    m_inputWS->mutableSample().setShape(shape);
  }
  rho *= 100; // Will give right units in going from
              // mu in cm^-1 to m^-1 for mu*total flight path( in m )

  // NOTE: the angstrom^-2 to barns and the angstrom^-1 to cm^-1
  // will cancel for mu to give units: cm^-1
  m_refAtten = -sigma_atten * rho / NeutronAtom::ReferenceLambda;
  m_scattering = -sigma_s * rho;

  n_lambda = getProperty("NumberOfWavelengthPoints");

  std::string exp_string = getProperty("ExpMethod");
  if (exp_string == "Normal") // Use the system exp function
    EXPONENTIAL = exp;
  else if (exp_string == "FastApprox") // Use the compact approximation
    EXPONENTIAL = fast_exp;

  // Get the energy mode
  const std::string emodeStr = getProperty("EMode");
  // Convert back to an integer representation
  m_emode = 0;
  if (emodeStr == "Direct")
    m_emode = 1;
  else if (emodeStr == "Indirect")
    m_emode = 2;
  // If inelastic, get the fixed energy and convert it to a wavelength
  if (m_emode) {
    const double efixed = getProperty("Efixed");
    Unit_const_sptr energy = UnitFactory::Instance().create("Energy");
    double factor, power;
    energy->quickConversion(*UnitFactory::Instance().create("Wavelength"),
                            factor, power);
    m_lambdaFixed = factor * std::pow(efixed, power);
  }

  // Call the virtual function for any further properties
  retrieveProperties();
}

/// Create the sample object using the Geometry classes, or use the existing one
void AbsorptionCorrection::constructSample(API::Sample &sample) {
  const std::string xmlstring = sampleXML();
  if (xmlstring.empty()) {
    // This means that we should use the shape already defined on the sample.
    m_sampleObject = &sample.getShape();
    // Check there is one, and fail if not
    if (!m_sampleObject->hasValidShape()) {
      const std::string mess(
          "No shape has been defined for the sample in the input workspace");
      g_log.error(mess);
      throw std::invalid_argument(mess);
    }
  } else {
    boost::shared_ptr<IObject> shape = ShapeFactory().createShape(xmlstring);
    sample.setShape(shape);
    m_sampleObject = &sample.getShape();

    g_log.information("Successfully constructed the sample object");
  }
}

/// Calculate the distances traversed by the neutrons within the sample
/// @param detector :: The detector we are working on
/// @param L2s :: A vector of the sample-detector distance for  each segment of
/// the sample
void AbsorptionCorrection::calculateDistances(const IDetector &detector,
                                              std::vector<double> &L2s) const {
  V3D detectorPos(detector.getPos());
  if (detector.nDets() > 1) {
    // We need to make sure this is right for grouped detectors - should use
    // average theta & phi
    detectorPos.spherical(detectorPos.norm(),
                          detector.getTwoTheta(V3D(), V3D(0, 0, 1)) * 180.0 /
                              M_PI,
                          detector.getPhi() * 180.0 / M_PI);
  }

  for (size_t i = 0; i < m_numVolumeElements; ++i) {
    // Create track for distance in cylinder between scattering point and
    // detector
    const V3D direction = normalize(detectorPos - m_elementPositions[i]);
    Track outgoing(m_elementPositions[i], direction);
    int temp = m_sampleObject->interceptSurface(outgoing);

    /* Most of the time, the number of hits is 1. Sometime, we have more than
     * one intersection due to
     * arithmetic imprecision. If it is the case, then selecting the first
     * intersection is valid.
     * In principle, one could check the consistency of all distances if hits is
     * larger than one by doing:
     * Mantid::Geometry::Track::LType::const_iterator it=outgoing.begin();
     * and looping until outgoing.end() checking the distances with it->Dist
     */
    // Not hitting the cylinder from inside, usually means detector is badly
    // defined,
    // i.e, position is (0,0,0).
    if (temp < 1) {
      // FOR NOW AT LEAST, JUST IGNORE THIS ERROR AND USE A ZERO PATH LENGTH,
      // WHICH I RECKON WILL MAKE A
      // NEGLIGIBLE DIFFERENCE ANYWAY (ALWAYS SEEMS TO HAPPEN WITH ELEMENT RIGHT
      // AT EDGE OF SAMPLE)
      L2s[i] = 0.0;

      // std::ostringstream message;
      // message << "Problem with detector at " << detectorPos << " ID:" <<
      // detector->getID() << '\n';
      // message << "This usually means that this detector is defined inside the
      // sample cylinder";
      // g_log.error(message.str());
      // throw std::runtime_error("Problem in
      // AbsorptionCorrection::calculateDistances");
    } else // The normal situation
    {
      L2s[i] = outgoing.cbegin()->distFromStart;
    }
  }
}

/// Carries out the numerical integration over the sample for elastic
/// instruments
double
AbsorptionCorrection::doIntegration(const double &lambda,
                                    const std::vector<double> &L2s) const {
  double integral = 0.0;

  size_t el = L2s.size();
  // Iterate over all the elements, summing up the integral
  for (size_t i = 0; i < el; ++i) {
    // Equation is exponent * element volume
    // where exponent is e^(-mu * wavelength/1.8 * (L1+L2) )
    const double exponent =
        ((m_refAtten * lambda) + m_scattering) * (m_L1s[i] + L2s[i]);
    integral += (EXPONENTIAL(exponent) * (m_elementVolumes[i]));
  }

  return integral;
}

/// Carries out the numerical integration over the sample for inelastic
/// instruments
double
AbsorptionCorrection::doIntegration(const double &lambda_i,
                                    const double &lambda_f,
                                    const std::vector<double> &L2s) const {
  double integral = 0.0;

  size_t el = L2s.size();
  // Iterate over all the elements, summing up the integral
  for (size_t i = 0; i < el; ++i) {
    // Equation is exponent * element volume
    // where exponent is e^(-mu * wavelength/1.8 * (L1+L2) )
    double exponent = ((m_refAtten * lambda_i) + m_scattering) * m_L1s[i];
    exponent += ((m_refAtten * lambda_f) + m_scattering) * L2s[i];
    integral += (EXPONENTIAL(exponent) * (m_elementVolumes[i]));
  }

  return integral;
}

} // namespace Algorithms
} // namespace Mantid
