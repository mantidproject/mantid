// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidHistogramData/Interpolate.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/Fast_Exponential.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid::Algorithms {

using namespace API;
using namespace Geometry;
using HistogramData::interpolateLinearInplace;
using namespace Kernel;
using Kernel::DeltaEMode;
using namespace Mantid::PhysicalConstants;
using namespace Mantid::DataObjects;
using PhysicalConstants::E_mev_toNeutronWavenumberSq;

namespace {
// the maximum number of elements to combine at once in the pairwise summation
constexpr size_t MAX_INTEGRATION_LENGTH{1000};

const std::string CALC_SAMPLE = "Sample";
const std::string CALC_CONTAINER = "Container";
const std::string CALC_ENVIRONMENT = "Environment";

inline size_t findMiddle(const size_t start, const size_t stop) {
  auto half = static_cast<size_t>(floor(.5 * (static_cast<double>(stop - start))));
  return start + half;
}

// wavelength = 2 pi / k
double energyToWavelength(const double energyFixed) {
  return 2. * M_PI * std::sqrt(E_mev_toNeutronWavenumberSq / energyFixed);
}

} // namespace

AbsorptionCorrection::AbsorptionCorrection()
    : API::Algorithm(), m_inputWS(), m_sampleObject(nullptr), m_L1s(), m_elementVolumes(), m_elementPositions(),
      m_numVolumeElements(0), m_sampleVolume(0.0), m_linearCoefTotScatt(0), m_num_lambda(0), m_xStep(0),
      m_emode(Kernel::DeltaEMode::Undefined), m_lambdaFixed(0.), EXPONENTIAL() {}

void AbsorptionCorrection::init() {

  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, wsValidator),
                  "The X values for the input workspace must be in units of wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "Output workspace name");

  // AbsorbedBy
  std::vector<std::string> scatter_options{CALC_SAMPLE, CALC_CONTAINER, CALC_ENVIRONMENT};
  declareProperty("ScatterFrom", CALC_SAMPLE, std::make_shared<StringListValidator>(std::move(scatter_options)),
                  "The component to calculate the absorption for (default: Sample)");

  auto mustBePositive = std::make_shared<BoundedValidator<double>>();
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

  auto positiveInt = std::make_shared<BoundedValidator<int64_t>>();
  positiveInt->setLower(1);
  declareProperty("NumberOfWavelengthPoints", static_cast<int64_t>(EMPTY_INT()), positiveInt,
                  "The number of wavelength points for which the numerical integral is\n"
                  "calculated (default: all points)");

  std::vector<std::string> exp_options{"Normal", "FastApprox"};
  declareProperty("ExpMethod", "Normal", std::make_shared<StringListValidator>(exp_options),
                  "Select the method to use to calculate exponentials, normal or a\n"
                  "fast approximation (default: Normal)");

  std::vector<std::string> propOptions{"Elastic", "Direct", "Indirect"};
  declareProperty("EMode", "Elastic", std::make_shared<StringListValidator>(propOptions),
                  "The energy mode (default: elastic)");
  declareProperty("EFixed", 0.0, mustBePositive,
                  "The value of the initial or final energy, as appropriate, in meV.\n"
                  "Will be taken from the instrument definition file, if available.");

  // Call the virtual method for concrete algorithm to define any other
  // properties
  defineProperties();
}

std::map<std::string, std::string> AbsorptionCorrection::validateInputs() {
  std::map<std::string, std::string> result;

  // verify that the container/environment information is there if requested
  const std::string scatterFrom = getProperty("ScatterFrom");
  if (scatterFrom != CALC_SAMPLE) {
    API::MatrixWorkspace_const_sptr wksp = getProperty("InputWorkspace");
    const auto &sample = wksp->sample();
    if (sample.hasEnvironment()) {
      const auto numComponents = sample.getEnvironment().nelements();
      // first element is assumed to be the container
      if (scatterFrom == CALC_CONTAINER && numComponents == 0) {
        result["ScatterFrom"] = "Sample does not have a container defined";
      } else if (scatterFrom == CALC_ENVIRONMENT) {
        if (numComponents < 2) {
          result["ScatterFrom"] = "Sample does not have an environment defined";
        } else if (numComponents > 2) {
          std::stringstream msg;
          msg << "Do not know how to calculate absorption from multiple "
                 "component sample environment. Encountered "
              << numComponents << " components";
          result["ScatterFrom"] = msg.str();
        }
      }
    } else { // customize error message based on selection
      if (scatterFrom == CALC_CONTAINER)
        result["ScatterFrom"] = "Sample does not have a container defined";
      else if (scatterFrom == CALC_ENVIRONMENT)
        result["ScatterFrom"] = "Sample does not have an environment defined";
    }
  }

  return result;
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
  correctionFactors->setDistribution(true); // The output of this is a distribution
  correctionFactors->setYUnit("");          // Need to explicitly set YUnit to nothing
  correctionFactors->setYUnitLabel("Attenuation factor");

  constructSample(correctionFactors->mutableSample());

  const auto numHists = static_cast<int64_t>(m_inputWS->getNumberHistograms());
  const auto specSize = static_cast<int64_t>(m_inputWS->blocksize());

  // If the number of wavelength points has not been given, use them all
  if (isEmpty(m_num_lambda))
    m_num_lambda = specSize;
  m_xStep = specSize / m_num_lambda; // Bin step between points to calculate

  if (m_xStep == 0) // Number of wavelength points >number of histogram points
    m_xStep = 1;

  std::ostringstream message;
  message << "Numerical integration performed every " << m_xStep << " wavelength points";
  g_log.information(message.str());
  message.str("");

  // Calculate the cached values of L1, element volumes, and geometry size
  initialiseCachedDistances();
  if (m_L1s.empty()) {
    throw std::runtime_error("Failed to define any initial scattering gauge volume for geometry");
  }

  const auto &spectrumInfo = m_inputWS->spectrumInfo();
  Progress prog(this, 0.0, 1.0, numHists);
  // Loop over the spectra
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS, *correctionFactors))
  for (int64_t i = 0; i < int64_t(numHists); ++i) {
    PARALLEL_START_INTERRUPT_REGION
    // Copy over bins
    correctionFactors->setSharedX(i, m_inputWS->sharedX(i));

    if (!spectrumInfo.hasDetectors(i)) {
      g_log.information() << "Spectrum " << i << " does not have a detector defined for it\n";
      continue;
    }
    if (spectrumInfo.isMasked(i)) {
      continue;
    }
    const auto &det = spectrumInfo.detector(i);

    std::vector<double> L2s(m_numVolumeElements);
    calculateDistances(det, L2s);

    // If an indirect instrument, see if there's an efixed in the parameter map
    double lambdaFixed = m_lambdaFixed;
    if (m_emode == DeltaEMode::Indirect) {
      try {
        Parameter_sptr par = pmap.get(&det, "Efixed");
        if (par) {
          lambdaFixed = energyToWavelength(par->value<double>());
        }
      } catch (std::runtime_error &) { /* Throws if a DetectorGroup, use single
                                          provided value */
      }
    }

    // calculate the absorption coefficient for fixed wavelength
    const double linearCoefAbsFixed = -m_material.linearAbsorpCoef(lambdaFixed);
    const auto wavelengths = m_inputWS->points(i);
    // these need to have the minus sign applied still
    const auto linearCoefAbs = m_material.linearAbsorpCoef(wavelengths.cbegin(), wavelengths.cend());

    // Get a reference to the Y's in the output WS for storing the factors
    auto &Y = correctionFactors->mutableY(i);

    // Loop through the bins in the current spectrum every m_xStep
    for (int64_t j = 0; j < specSize; j = j + m_xStep) {
      if (m_emode == DeltaEMode::Elastic) {
        Y[j] = this->doIntegration(-linearCoefAbs[j], L2s, 0, L2s.size());
      } else if (m_emode == DeltaEMode::Direct) {
        Y[j] = this->doIntegration(linearCoefAbsFixed, -linearCoefAbs[j], L2s, 0, L2s.size());
      } else if (m_emode == DeltaEMode::Indirect) {
        Y[j] = this->doIntegration(-linearCoefAbs[j], linearCoefAbsFixed, L2s, 0, L2s.size());
      } else { // should never happen
        throw std::runtime_error("AbsorptionCorrection doesn't have a known DeltaEMode defined");
      }
      Y[j] /= m_sampleVolume; // Divide by total volume of the shape

      // Make certain that last point is calculated
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

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  g_log.information() << "Total number of elements in the integration was " << m_L1s.size() << '\n';
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
  const std::string scatterFrom = getProperty("ScatterFrom");

  bool createMaterial = !(isEmpty(rho) && isEmpty(sigma_s) && isEmpty(sigma_atten));
  // get the material from the correct component
  const auto &sampleObj = m_inputWS->sample();
  if (scatterFrom == CALC_SAMPLE) {
    m_material = sampleObj.getShape().material();
  } else if (scatterFrom == CALC_CONTAINER) {
    m_material = sampleObj.getEnvironment().getContainer().material();
  } else if (scatterFrom == CALC_ENVIRONMENT) {
    m_material = sampleObj.getEnvironment().getComponent(1).material();
  }

  if (createMaterial) {
    // get values from the existing material
    if (isEmpty(rho))
      rho = m_material.numberDensityEffective();
    if (isEmpty(sigma_s))
      sigma_s = m_material.totalScatterXSection();
    if (isEmpty(sigma_atten))
      sigma_atten = m_material.absorbXSection(NeutronAtom::ReferenceLambda);

    // create the new material
    NeutronAtom neutron(0, 0, 0.0, 0.0, sigma_s, 0.0, sigma_s, sigma_atten);

    // Save input in Sample with wrong atomic number and name
    auto shape = std::shared_ptr<IObject>(
        m_inputWS->sample().getShape().cloneWithMaterial(Material("SetInAbsorptionCorrection", neutron, rho)));
    m_inputWS->mutableSample().setShape(shape);

    // get the material back
    m_material = m_inputWS->sample().getShape().material();
  }

  // NOTE: the angstrom^-2 to barns and the angstrom^-1 to cm^-1
  // will cancel for mu to give units: cm^-1
  m_linearCoefTotScatt = -m_material.totalScatterXSection() * m_material.numberDensityEffective() * 100;

  m_num_lambda = getProperty("NumberOfWavelengthPoints");

  std::string exp_string = getProperty("ExpMethod");
  if (exp_string == "Normal") // Use the system exp function
    EXPONENTIAL = exp;
  else if (exp_string == "FastApprox") // Use the compact approximation
    EXPONENTIAL = fast_exp;

  // Get the energy mode
  const std::string emodeStr = getProperty("EMode");
  // Convert back to an integer representation
  if (emodeStr == "Direct")
    m_emode = DeltaEMode::Direct;
  else if (emodeStr == "Indirect")
    m_emode = DeltaEMode::Indirect;
  else if (emodeStr == "Elastic")
    m_emode = DeltaEMode::Elastic;
  else {
    std::stringstream msg;
    msg << "Unknown EMode \"" << emodeStr << "\"";
    throw std::runtime_error(msg.str());
  }

  // If inelastic, get the fixed energy and convert it to a wavelength
  if (m_emode != DeltaEMode::Elastic) {
    const double efixed = getProperty("Efixed");
    m_lambdaFixed = energyToWavelength(efixed);
  }

  // Call the virtual function for any further properties
  retrieveProperties();
}

/// Create the sample object using the Geometry classes, or use the existing one
void AbsorptionCorrection::constructSample(API::Sample &sample) {
  const std::string xmlstring = sampleXML();
  const std::string scatterFrom = getProperty("ScatterFrom");
  if (xmlstring.empty()) {
    // Get the shape from the proper object
    if (scatterFrom == CALC_SAMPLE)
      m_sampleObject = &sample.getShape();
    else if (scatterFrom == CALC_CONTAINER)
      m_sampleObject = &(sample.getEnvironment().getContainer());
    else if (scatterFrom == CALC_ENVIRONMENT)
      m_sampleObject = &(sample.getEnvironment().getComponent(1));
    else
      throw std::runtime_error("Somebody forgot to fill in an if/else tree");

    // Check there is one, and fail if not
    if (!m_sampleObject->hasValidShape()) {
      const std::string mess("No shape has been defined for the sample in the input workspace");
      g_log.error(mess);
      throw std::invalid_argument(mess);
    }
  } else if (scatterFrom != CALC_SAMPLE) { // should never be in this case
    std::stringstream msg;
    msg << "Cannot use geometry xml for ScatterFrom=" << scatterFrom;
    throw std::runtime_error(msg.str());
  } else { // create a geometry from the sample object
    std::shared_ptr<IObject> shape = ShapeFactory().createShape(xmlstring);
    sample.setShape(shape);
    m_sampleObject = &sample.getShape();

    g_log.information("Successfully constructed the sample object");
  }
}

/// Calculate the distances traversed by the neutrons within the sample
/// @param detector :: The detector we are working on
/// @param L2s :: A vector of the sample-detector distance for  each segment of
/// the sample
void AbsorptionCorrection::calculateDistances(const IDetector &detector, std::vector<double> &L2s) const {
  V3D detectorPos(detector.getPos());
  if (detector.nDets() > 1) {
    // We need to make sure this is right for grouped detectors - should use
    // average theta & phi
    detectorPos.spherical(detectorPos.norm(), detector.getTwoTheta(V3D(), V3D(0, 0, 1)) * 180.0 / M_PI,
                          detector.getPhi() * 180.0 / M_PI);
  }

  for (size_t i = 0; i < m_numVolumeElements; ++i) {
    // Create track for distance in cylinder between scattering point and
    // detector
    const V3D direction = normalize(detectorPos - m_elementPositions[i]);
    Track outgoing(m_elementPositions[i], direction);
    m_sampleObject->interceptSurface(outgoing);
    L2s[i] = outgoing.totalDistInsideObject();
  }
}

// the integrations are done using pairwise summation to reduce
// issues from adding lots of little numbers together
// https://en.wikipedia.org/wiki/Pairwise_summation

/// Carries out the numerical integration over the sample for elastic
/// instruments
double AbsorptionCorrection::doIntegration(const double linearCoefAbs, const std::vector<double> &L2s,
                                           const size_t startIndex, const size_t endIndex) const {
  if (endIndex - startIndex > MAX_INTEGRATION_LENGTH) {
    size_t middle = findMiddle(startIndex, endIndex);

    return doIntegration(linearCoefAbs, L2s, startIndex, middle) + doIntegration(linearCoefAbs, L2s, middle, endIndex);
  } else {
    double integral = 0.0;

    // Iterate over all the elements, summing up the integral
    for (size_t i = startIndex; i < endIndex; ++i) {
      const double exponent = (linearCoefAbs + m_linearCoefTotScatt) * (m_L1s[i] + L2s[i]);
      integral += (EXPONENTIAL(exponent) * (m_elementVolumes[i]));
    }

    return integral;
  }
}

/// Carries out the numerical integration over the sample for inelastic
/// instruments
double AbsorptionCorrection::doIntegration(const double linearCoefAbsL1, const double linearCoefAbsL2,
                                           const std::vector<double> &L2s, const size_t startIndex,
                                           const size_t endIndex) const {

  // L2s is initiliazed by m_numVolumeElements which is set to 0 in this class but it is overriden in subclasses
  // like FlatAbsorptionCorrection
  // cppcheck-suppress knownConditionTrueFalse
  if (endIndex - startIndex > MAX_INTEGRATION_LENGTH) {
    size_t middle = findMiddle(startIndex, endIndex);

    return doIntegration(linearCoefAbsL1, linearCoefAbsL2, L2s, startIndex, middle) +
           doIntegration(linearCoefAbsL1, linearCoefAbsL2, L2s, middle, endIndex);
  } else {
    double integral = 0.0;

    // Iterate over all the elements, summing up the integral
    for (size_t i = startIndex; i < endIndex; ++i) {
      double exponent = (linearCoefAbsL1 + m_linearCoefTotScatt) * m_L1s[i];
      exponent += (linearCoefAbsL2 + m_linearCoefTotScatt) * L2s[i];
      integral += (EXPONENTIAL(exponent) * (m_elementVolumes[i]));
    }

    return integral;
  }
}

} // namespace Mantid::Algorithms
