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
#include "MantidAPI/WorkspaceGroup.h"
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
    : API::Algorithm(), m_inputWS(), m_sampleObject(nullptr),
      m_containerObject(nullptr), m_L1s(), m_elementVolumes(),
      m_elementPositions(), m_numVolumeElements(0), m_sampleVolume(0.0),
      m_containerL1s(), m_containerElementVolumes(),
      m_containerElementPositions(), m_containerNumVolumeElements(0),
      m_containerVolume(0.0), m_linearCoefTotScatt(0),
      m_containerLinearCoefTotScatt(0), m_num_lambda(0), m_xStep(0),
      m_cubeSide(0.0) {}

void AbsorptionCorrectionPaalmanPings::init() {

  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();

  declareProperty(
      std::make_unique<WorkspaceProperty<>>("InputWorkspace", "",
                                            Direction::Input, wsValidator),
      "The X values for the input workspace must be in units of wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>(
                      "OutputWorkspace", "", Direction::Output),
                  "Output workspace name");

  auto positiveInt = std::make_shared<BoundedValidator<int64_t>>();
  positiveInt->setLower(1);
  declareProperty(
      "NumberOfWavelengthPoints", static_cast<int64_t>(EMPTY_INT()),
      positiveInt,
      "The number of wavelength points for which the numerical integral is\n"
      "calculated (default: all points)");

  auto moreThanZero = std::make_shared<BoundedValidator<double>>();
  moreThanZero->setLower(0.001);
  declareProperty("ElementSize", 1.0, moreThanZero,
                  "The size of one side of an integration element cube in mm");
}

std::map<std::string, std::string>
AbsorptionCorrectionPaalmanPings::validateInputs() {
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
  MatrixWorkspace_sptr ass = create<HistoWorkspace>(*m_inputWS);
  ass->setDistribution(true); // The output of this is a distribution
  ass->setYUnit("");          // Need to explicitly set YUnit to nothing
  ass->setYUnitLabel("Attenuation factor");
  MatrixWorkspace_sptr assc = create<HistoWorkspace>(*m_inputWS);
  assc->setDistribution(true); // The output of this is a distribution
  assc->setYUnit("");          // Need to explicitly set YUnit to nothing
  assc->setYUnitLabel("Attenuation factor");
  MatrixWorkspace_sptr acc = create<HistoWorkspace>(*m_inputWS);
  acc->setDistribution(true); // The output of this is a distribution
  acc->setYUnit("");          // Need to explicitly set YUnit to nothing
  acc->setYUnitLabel("Attenuation factor");
  MatrixWorkspace_sptr acsc = create<HistoWorkspace>(*m_inputWS);
  acsc->setDistribution(true); // The output of this is a distribution
  acsc->setYUnit("");          // Need to explicitly set YUnit to nothing
  acsc->setYUnitLabel("Attenuation factor");

  constructSample(m_inputWS->mutableSample());

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
  if (m_L1s.empty() || m_containerL1s.empty()) {
    throw std::runtime_error(
        "Failed to define any initial scattering gauge volume for geometry");
  }

  const auto &spectrumInfo = m_inputWS->spectrumInfo();
  Progress prog(this, 0.0, 1.0, numHists);
  // Loop over the spectra
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS, *ass, *assc, *acc, *acsc))
  for (int64_t i = 0; i < int64_t(numHists); ++i) {
    PARALLEL_START_INTERUPT_REGION
    // Copy over bins
    ass->setSharedX(i, m_inputWS->sharedX(i));
    assc->setSharedX(i, m_inputWS->sharedX(i));
    acc->setSharedX(i, m_inputWS->sharedX(i));
    acsc->setSharedX(i, m_inputWS->sharedX(i));

    if (!spectrumInfo.hasDetectors(i)) {
      g_log.information() << "Spectrum " << i
                          << " does not have a detector defined for it\n";
      continue;
    }
    const auto &det = spectrumInfo.detector(i);

    std::vector<double> sample_L2s(m_numVolumeElements);
    std::vector<double> sample_container_L2s(m_numVolumeElements);
    std::vector<double> container_L2s(m_containerNumVolumeElements);
    std::vector<double> container_sample_L2s(m_containerNumVolumeElements);
    calculateDistances(det, sample_L2s, sample_container_L2s, container_L2s,
                       container_sample_L2s);

    const auto wavelengths = m_inputWS->points(i);
    // these need to have the minus sign applied still
    const auto linearCoefAbs =
        m_material.linearAbsorpCoef(wavelengths.cbegin(), wavelengths.cend());
    const auto containerLinearCoefAbs = m_containerMaterial.linearAbsorpCoef(
        wavelengths.cbegin(), wavelengths.cend());

    // Get a reference to the Y's in the output WS for storing the factors
    auto &assY = ass->mutableY(i);
    auto &asscY = assc->mutableY(i);
    auto &accY = acc->mutableY(i);
    auto &acscY = acsc->mutableY(i);

    // Loop through the bins in the current spectrum every m_xStep
    for (int64_t j = 0; j < specSize; j = j + m_xStep) {
      assY[j] = this->doIntegration(-linearCoefAbs[j], m_linearCoefTotScatt,
                                    m_elementVolumes, m_L1s, sample_L2s, 0,
                                    m_numVolumeElements);
      assY[j] /= m_sampleVolume; // Divide by total volume of the shape

      asscY[j] = this->doCrossIntegration(
          -linearCoefAbs[j], m_linearCoefTotScatt, m_elementVolumes, m_L1s,
          sample_L2s, -containerLinearCoefAbs[j], m_containerLinearCoefTotScatt,
          m_sample_containerL1s, sample_container_L2s, 0, m_numVolumeElements);
      asscY[j] /= m_sampleVolume; // Divide by total volume of the shape

      accY[j] = this->doIntegration(
          -containerLinearCoefAbs[j], m_containerLinearCoefTotScatt,
          m_containerElementVolumes, m_containerL1s, container_L2s, 0,
          m_containerNumVolumeElements);
      accY[j] /= m_containerVolume; // Divide by total volume of the shape

      acscY[j] = this->doCrossIntegration(
          -containerLinearCoefAbs[j], m_containerLinearCoefTotScatt,
          m_containerElementVolumes, m_containerL1s, container_L2s,
          -linearCoefAbs[j], m_linearCoefTotScatt, m_container_sampleL1s,
          container_sample_L2s, 0, m_containerNumVolumeElements);
      acscY[j] /= m_containerVolume; // Divide by total volume of the shape

      // Make certain that last point is calculated
      if (m_xStep > 1 && j + m_xStep >= specSize && j + 1 != specSize) {
        j = specSize - m_xStep - 1;
      }
    }

    // Interpolate linearly between points separated by m_xStep,
    // last point required
    if (m_xStep > 1) {
      auto histnew = ass->histogram(i);
      interpolateLinearInplace(histnew, m_xStep);
      ass->setHistogram(i, histnew);
      auto histnew2 = assc->histogram(i);
      interpolateLinearInplace(histnew2, m_xStep);
      assc->setHistogram(i, histnew2);
      auto histnew3 = acc->histogram(i);
      interpolateLinearInplace(histnew3, m_xStep);
      acc->setHistogram(i, histnew3);
      auto histnew4 = acsc->histogram(i);
      interpolateLinearInplace(histnew4, m_xStep);
      acsc->setHistogram(i, histnew4);
    }

    prog.report();

    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  g_log.information() << "Total number of elements in the integration was "
                      << m_L1s.size() << '\n';

  const std::string outWSName = getProperty("OutputWorkspace");

  std::vector<std::string> names;
  names.emplace_back(outWSName + "_ass");
  API::AnalysisDataService::Instance().addOrReplace(names.back(), ass);
  names.emplace_back(outWSName + "_assc");
  API::AnalysisDataService::Instance().addOrReplace(names.back(), assc);
  names.emplace_back(outWSName + "_acc");
  API::AnalysisDataService::Instance().addOrReplace(names.back(), acc);
  names.emplace_back(outWSName + "_acsc");
  API::AnalysisDataService::Instance().addOrReplace(names.back(), acsc);

  auto group = createChildAlgorithm("GroupWorkspaces");
  group->initialize();
  group->setProperty("InputWorkspaces", names);
  group->setProperty("OutputWorkspace", outWSName);
  group->execute();
  API::WorkspaceGroup_sptr outWS = group->getProperty("OutputWorkspace");

  setProperty("OutputWorkspace", outWS);

  // Now do some cleaning-up since destructor may not be called immediately
  m_L1s.clear();
  m_elementVolumes.clear();
  m_elementPositions.clear();
  m_containerL1s.clear();
  m_containerElementVolumes.clear();
  m_containerElementPositions.clear();
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

  // now for the container
  integrationVolume =
      std::shared_ptr<const IObject>(m_containerObject->clone());
  if (m_inputWS->run().hasProperty("GaugeVolume")) {
    integrationVolume = constructGaugeVolume();
  }

  raster = Geometry::Rasterize::calculate(m_beamDirection, *integrationVolume,
                                          m_cubeSide);
  m_containerVolume = raster.totalvolume;
  if (raster.l1.size() == 0)
    throw std::runtime_error("Failed to rasterize shape");
  // move over the information
  m_containerNumVolumeElements = raster.l1.size();
  m_containerL1s = std::move(raster.l1);
  m_containerElementPositions = std::move(raster.position);
  m_containerElementVolumes = std::move(raster.volume);

  // Get the L1s for the cross terms
  // L1s for passing through container to be scattered by the sample
  m_sample_containerL1s.reserve(m_numVolumeElements);
  for (size_t i = 0; i < m_numVolumeElements; ++i) {
    Track outgoing(m_elementPositions[i], -m_beamDirection);
    if (m_containerObject->interceptSurface(outgoing) > 0) {
      m_sample_containerL1s[i] = outgoing.totalDistInsideObject();
    } else {
      m_sample_containerL1s[i] = 0;
    }
  }

  // L1s for passing through sample to be scattered by the container
  m_container_sampleL1s.reserve(m_containerNumVolumeElements);
  for (size_t i = 0; i < m_containerNumVolumeElements; ++i) {
    Track outgoing(m_containerElementPositions[i], -m_beamDirection);
    if (m_sampleObject->interceptSurface(outgoing) > 0) {
      m_container_sampleL1s[i] = outgoing.totalDistInsideObject();
    } else {
      m_container_sampleL1s[i] = 0;
    }
  }
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
  m_containerLinearCoefTotScatt = -m_containerMaterial.totalScatterXSection() *
                                  m_containerMaterial.numberDensity() * 100;

  m_num_lambda = getProperty("NumberOfWavelengthPoints");

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
void AbsorptionCorrectionPaalmanPings::calculateDistances(
    const IDetector &detector, std::vector<double> &sample_L2s,
    std::vector<double> &sample_container_L2s,
    std::vector<double> &container_L2s,
    std::vector<double> &container_sample_L2s) const {
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
    // Create track for distance between scattering point in sample and detector
    const V3D direction = normalize(detectorPos - m_elementPositions[i]);
    Track outgoing(m_elementPositions[i], direction);
    if (m_sampleObject->interceptSurface(outgoing) > 0) {
      sample_L2s[i] = outgoing.totalDistInsideObject();
    } else {
      sample_L2s[i] = 0;
    }

    Track outgoing2(m_elementPositions[i], direction);
    if (m_containerObject->interceptSurface(outgoing2) > 0) {
      sample_container_L2s[i] = outgoing2.totalDistInsideObject();
    } else {
      sample_container_L2s[i] = 0;
    }
  }

  for (size_t i = 0; i < m_containerNumVolumeElements; ++i) {
    // Create track for distance between scattering point in container and
    // detector
    const V3D direction =
        normalize(detectorPos - m_containerElementPositions[i]);
    Track outgoing(m_containerElementPositions[i], direction);
    if (m_containerObject->interceptSurface(outgoing) > 0) {
      container_L2s[i] = outgoing.totalDistInsideObject();
    } else {
      container_L2s[i] = 0;
    }
    Track outgoing2(m_containerElementPositions[i], direction);
    if (m_sampleObject->interceptSurface(outgoing2) > 0) {
      container_sample_L2s[i] = outgoing2.totalDistInsideObject();
    } else {
      container_sample_L2s[i] = 0;
    }
  }
}

// the integrations are done using pairwise summation to reduce
// issues from adding lots of little numbers together
// https://en.wikipedia.org/wiki/Pairwise_summation

/// Carries out the numerical integration over the sample for elastic
/// instruments
double AbsorptionCorrectionPaalmanPings::doIntegration(
    const double linearCoefAbs, const double linearCoefTotScatt,
    const std::vector<double> &elementVolumes, const std::vector<double> &L1s,
    const std::vector<double> &L2s, const size_t startIndex,
    const size_t endIndex) const {
  if (endIndex - startIndex > MAX_INTEGRATION_LENGTH) {
    size_t middle = findMiddle(startIndex, endIndex);

    return doIntegration(linearCoefAbs, linearCoefTotScatt, elementVolumes, L1s,
                         L2s, startIndex, middle) +
           doIntegration(linearCoefAbs, linearCoefTotScatt, elementVolumes, L1s,
                         L2s, middle, endIndex);
  } else {
    double integral = 0.0;

    // Iterate over all the elements, summing up the integral
    for (size_t i = startIndex; i < endIndex; ++i) {
      const double exponent =
          (linearCoefAbs + linearCoefTotScatt) * (L1s[i] + L2s[i]);
      integral += (exp(exponent) * (elementVolumes[i]));
    }

    return integral;
  }
}

double AbsorptionCorrectionPaalmanPings::doCrossIntegration(
    const double linearCoefAbs, const double linearCoefTotScatt,
    const std::vector<double> &elementVolumes, const std::vector<double> &L1s,
    const std::vector<double> &L2s, const double linearCoefAbs2,
    const double linearCoefTotScatt2, const std::vector<double> &L1s2,
    const std::vector<double> &L2s2, const size_t startIndex,
    const size_t endIndex) const {
  if (endIndex - startIndex > MAX_INTEGRATION_LENGTH) {
    size_t middle = findMiddle(startIndex, endIndex);

    return doCrossIntegration(linearCoefAbs, linearCoefTotScatt, elementVolumes,
                              L1s, L2s, linearCoefAbs2, linearCoefTotScatt2,
                              L1s2, L2s2, startIndex, middle) +
           doCrossIntegration(linearCoefAbs, linearCoefTotScatt, elementVolumes,
                              L1s, L2s, linearCoefAbs2, linearCoefTotScatt2,
                              L1s2, L2s2, middle, endIndex);
  } else {
    double integral = 0.0;

    // Iterate over all the elements, summing up the integral
    for (size_t i = startIndex; i < endIndex; ++i) {
      double exponent =
          (linearCoefAbs + linearCoefTotScatt) * (L1s[i] + L2s[i]);
      exponent += (linearCoefAbs2 + linearCoefTotScatt2) * (L1s2[i] + L2s2[i]);
      integral += (exp(exponent) * (elementVolumes[i]));
    }

    return integral;
  }
}

} // namespace Algorithms
} // namespace Mantid
