// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/AbsorptionCorrectionPaalmanPings.h"
#include "MantidAPI/HistoWorkspace.h"
#include "MantidAPI/InstrumentValidator.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Rasterize.h"
#include "MantidHistogramData/Interpolate.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Fast_Exponential.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AbsorptionCorrectionPaalmanPings)

using namespace API;
using namespace Geometry;
using HistogramData::interpolateLinearInplace;
using namespace Kernel;
using namespace Mantid::PhysicalConstants;
using namespace Mantid::DataObjects;

namespace {
// the maximum number of elements to combine at once in the pairwise summation
constexpr size_t MAX_INTEGRATION_LENGTH{1000};

inline size_t findMiddle(const size_t start, const size_t stop) {
  auto half =
      static_cast<size_t>(floor(.5 * (static_cast<double>(stop - start))));
  return start + half;
}

} // namespace


AbsorptionCorrectionPaalmanPings::AbsorptionCorrectionPaalmanPings()
    : API::Algorithm(), m_inputWS(), m_sampleObject(nullptr), m_L1s(),
      m_elementVolumes(), m_elementPositions(), m_numVolumeElements(0),
      m_sampleVolume(0.0), m_linearCoefTotScatt(0), m_num_lambda(0), m_xStep(0),
      m_lambdaFixed(0.), EXPONENTIAL(), m_cubeSide(0.0){
}

void AbsorptionCorrectionPaalmanPings::init() {

  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();

  declareProperty(
      std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                            Direction::Input, wsValidator),
      "The X values for the input workspace must be in units of wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output),
                  "Output workspace name");

  auto positiveInt = std::make_shared<BoundedValidator<int64_t>>();
  positiveInt->setLower(1);
  declareProperty(
      "NumberOfWavelengthPoints", static_cast<int64_t>(EMPTY_INT()),
      positiveInt,
      "The number of wavelength points for which the numerical integral is\n"
      "calculated (default: all points)");

  std::vector<std::string> exp_options{"Normal", "FastApprox"};
  declareProperty(
      "ExpMethod", "Normal", std::make_shared<StringListValidator>(exp_options),
      "Select the method to use to calculate exponentials, normal or a\n"
      "fast approximation (default: Normal)");

  auto moreThanZero = std::make_shared<BoundedValidator<double>>();
  moreThanZero->setLower(0.001);
  declareProperty("ElementSize", 1.0, moreThanZero,
                  "The size of one side of an integration element cube in mm");
}

std::map<std::string, std::string> AbsorptionCorrectionPaalmanPings::validateInputs() {
  std::map<std::string, std::string> result;

  // verify that the container information is there if requested
  API::MatrixWorkspace_const_sptr wksp = getProperty("InputWorkspace");
  const auto &sample = wksp->sample();
  if (sample.hasEnvironment()) {
    const auto numComponents = sample.getEnvironment().nelements();
    // first element is assumed to be the container
    if (numComponents == 0) {
      result["InputWorkspace"] = "Sample does not have a container defined";
    }
  } else {
    result["InputWorkspace"] = "Sample does not have a container defined";
  }
  return result;
}

void AbsorptionCorrectionPaalmanPings::exec() {
  // Retrieve the input workspace
  m_inputWS = getProperty("InputWorkspace");
  // Cache the beam direction
  m_beamDirection = m_inputWS->getInstrument()->getBeamDirection();

  // Get the input parameters
  retrieveBaseProperties();

  // Create the output workspace
  MatrixWorkspace_sptr correctionFactors = create<HistoWorkspace>(*m_inputWS);
  correctionFactors->setDistribution(
      true);                       // The output of this is a distribution
  correctionFactors->setYUnit(""); // Need to explicitly set YUnit to nothing
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
  message << "Numerical integration performed every " << m_xStep
          << " wavelength points";
  g_log.information(message.str());
  message.str("");

  // Calculate the cached values of L1, element volumes, and geometry size
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

    if (!spectrumInfo.hasDetectors(i)) {
      g_log.information() << "Spectrum " << i
                          << " does not have a detector defined for it\n";
      continue;
    }
    const auto &det = spectrumInfo.detector(i);

    std::vector<double> L2s(m_numVolumeElements);
    calculateDistances(det, L2s);

    const auto wavelengths = m_inputWS->points(i);
    // these need to have the minus sign applied still
    const auto linearCoefAbs =
        m_material.linearAbsorpCoef(wavelengths.cbegin(), wavelengths.cend());

    // Get a reference to the Y's in the output WS for storing the factors
    auto &Y = correctionFactors->mutableY(i);

    // Loop through the bins in the current spectrum every m_xStep
    for (int64_t j = 0; j < specSize; j = j + m_xStep) {
      Y[j] = this->doIntegration(-linearCoefAbs[j], L2s, 0, L2s.size());
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

/// Calculate the distances for L1 and element size for each element in the
/// sample
void AbsorptionCorrectionPaalmanPings::initialiseCachedDistances() {
  // First, check if a 'gauge volume' has been defined. If not, it's the same as
  // the sample.
  auto integrationVolume =
      std::shared_ptr<const IObject>(m_sampleObject->clone());
  if (m_inputWS->run().hasProperty("GaugeVolume")) {
    integrationVolume = constructGaugeVolume();
  }

  auto raster = Geometry::Rasterize::calculate(m_beamDirection,
                                               *integrationVolume, m_cubeSide);
  m_sampleVolume = raster.totalvolume;
  if (raster.l1.size() == 0)
    throw std::runtime_error("Failed to rasterize shape");
  // move over the information
  m_numVolumeElements = raster.l1.size();
  m_L1s = std::move(raster.l1);
  m_elementPositions = std::move(raster.position);
  m_elementVolumes = std::move(raster.volume);
}

std::shared_ptr<const Geometry::IObject>
AbsorptionCorrectionPaalmanPings::constructGaugeVolume() {
  g_log.information("Calculating scattering within the gauge volume defined on "
                    "the input workspace");

  // Retrieve and create the gauge volume shape
  std::shared_ptr<const Geometry::IObject> volume = ShapeFactory().createShape(
      m_inputWS->run().getProperty("GaugeVolume")->value());

  return volume;
}

std::string AbsorptionCorrectionPaalmanPings::sampleXML() {
  // Returning an empty string signals to the base class that it should
  // use the object already attached to the sample.
  return std::string();
}
/// Fetch the properties and set the appropriate member variables
void AbsorptionCorrectionPaalmanPings::retrieveBaseProperties() {

  // get the material from the correct component
  const auto &sampleObj = m_inputWS->sample();
  m_material = sampleObj.getShape().material();
  m_containerMaterial = sampleObj.getEnvironment().getContainer().material();

  // NOTE: the angstrom^-2 to barns and the angstrom^-1 to cm^-1
  // will cancel for mu to give units: cm^-1
  m_linearCoefTotScatt =
      -m_material.totalScatterXSection() * m_material.numberDensity() * 100;

  m_num_lambda = getProperty("NumberOfWavelengthPoints");

  std::string exp_string = getProperty("ExpMethod");
  if (exp_string == "Normal") // Use the system exp function
    EXPONENTIAL = exp;
  else if (exp_string == "FastApprox") // Use the compact approximation
    EXPONENTIAL = fast_exp;

  // Call the virtual function for any further properties
  m_cubeSide = getProperty("ElementSize"); // in mm
  m_cubeSide *= 0.001;                     // now in m

}

/// Create the sample object using the Geometry classes, or use the existing one
void AbsorptionCorrectionPaalmanPings::constructSample(API::Sample &sample) {
  m_sampleObject = &sample.getShape();
  m_containerObject = &(sample.getEnvironment().getContainer());

  // Check there is one, and fail if not
  if (!m_sampleObject->hasValidShape()) {
    const std::string mess(
                           "No shape has been defined for the sample in the input workspace");
    g_log.error(mess);
    throw std::invalid_argument(mess);
  }
  // Check there is one, and fail if not
  if (!m_containerObject->hasValidShape()) {
    const std::string mess(
                           "No shape has been defined for the container in the input workspace");
    g_log.error(mess);
    throw std::invalid_argument(mess);
  }
}

/// Calculate the distances traversed by the neutrons within the sample
/// @param detector :: The detector we are working on
/// @param L2s :: A vector of the sample-detector distance for  each segment of
/// the sample
void AbsorptionCorrectionPaalmanPings::calculateDistances(const IDetector &detector,
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

// the integrations are done using pairwise summation to reduce
// issues from adding lots of little numbers together
// https://en.wikipedia.org/wiki/Pairwise_summation

/// Carries out the numerical integration over the sample for elastic
/// instruments
double AbsorptionCorrectionPaalmanPings::doIntegration(const double linearCoefAbs,
                                           const std::vector<double> &L2s,
                                           const size_t startIndex,
                                           const size_t endIndex) const {
  if (endIndex - startIndex > MAX_INTEGRATION_LENGTH) {
    size_t middle = findMiddle(startIndex, endIndex);

    return doIntegration(linearCoefAbs, L2s, startIndex, middle) +
           doIntegration(linearCoefAbs, L2s, middle, endIndex);
  } else {
    double integral = 0.0;

    // Iterate over all the elements, summing up the integral
    for (size_t i = startIndex; i < endIndex; ++i) {
      const double exponent =
          (linearCoefAbs + m_linearCoefTotScatt) * (m_L1s[i] + L2s[i]);
      integral += (EXPONENTIAL(exponent) * (m_elementVolumes[i]));
    }

    return integral;
  }
}

} // namespace Algorithms
} // namespace Mantid
