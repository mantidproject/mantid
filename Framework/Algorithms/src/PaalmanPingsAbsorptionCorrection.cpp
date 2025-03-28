// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/PaalmanPingsAbsorptionCorrection.h"
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
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Rasterize.h"
#include "MantidHistogramData/Interpolate.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(PaalmanPingsAbsorptionCorrection)

using namespace API;
using namespace Geometry;
using HistogramData::interpolateLinearInplace;
using namespace Kernel;
using namespace Mantid::DataObjects;

namespace {
// the maximum number of elements to combine at once in the pairwise summation
constexpr size_t MAX_INTEGRATION_LENGTH{1000};

inline size_t findMiddle(const size_t start, const size_t stop) {
  auto half = static_cast<size_t>(floor(.5 * (static_cast<double>(stop - start))));
  return start + half;
}

} // namespace

PaalmanPingsAbsorptionCorrection::PaalmanPingsAbsorptionCorrection()
    : API::Algorithm(), m_inputWS(), m_sampleObject(nullptr), m_containerObject(nullptr), m_sampleL1s(),
      m_sample_containerL1s(), m_sampleElementVolumes(), m_sampleElementPositions(), m_numSampleVolumeElements(0),
      m_sampleVolume(0.0), m_containerL1s(), m_container_sampleL1s(), m_containerElementVolumes(),
      m_containerElementPositions(), m_numContainerVolumeElements(0), m_containerVolume(0.0),
      m_ampleLinearCoefTotScatt(0), m_containerLinearCoefTotScatt(0), m_num_lambda(0), m_xStep(0),
      m_cubeSideSample(0.0), m_cubeSideContainer(0.0) {}

void PaalmanPingsAbsorptionCorrection::init() {

  // The input workspace must have an instrument and units of wavelength
  auto wsValidator = std::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<InstrumentValidator>();

  declareProperty(std::make_unique<WorkspaceProperty<>>("InputWorkspace", "", Direction::Input, wsValidator),
                  "The X values for the input workspace must be in units of wavelength");
  declareProperty(std::make_unique<WorkspaceProperty<WorkspaceGroup>>("OutputWorkspace", "", Direction::Output),
                  "Output workspace name");

  auto positiveInt = std::make_shared<BoundedValidator<int64_t>>();
  positiveInt->setLower(1);
  declareProperty("NumberOfWavelengthPoints", static_cast<int64_t>(EMPTY_INT()), positiveInt,
                  "The number of wavelength points for which the numerical integral is\n"
                  "calculated (default: all points)");

  auto moreThanZero = std::make_shared<BoundedValidator<double>>();
  moreThanZero->setLower(0.001);
  declareProperty("ElementSize", 1.0, moreThanZero, "The size of one side of an integration element cube in mm");

  declareProperty("ContainerElementSize", EMPTY_DBL(),
                  "The size of one side of an integration element cube in mm for container."
                  "Default to be the same as ElementSize.");
}

std::map<std::string, std::string> PaalmanPingsAbsorptionCorrection::validateInputs() {
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

void PaalmanPingsAbsorptionCorrection::exec() {
  // Retrieve the input workspace
  m_inputWS = getProperty("InputWorkspace");
  // Cache the beam direction
  m_beamDirection = m_inputWS->getInstrument()->getBeamDirection();

  // Get the input parameters
  retrieveBaseProperties();

  // Create the output workspaces

  // A_s,s - absorption factor for scattering and self-absorption in sample
  MatrixWorkspace_sptr ass = create<HistoWorkspace>(*m_inputWS);
  ass->setDistribution(true); // The output of this is a distribution
  ass->setYUnit("");          // Need to explicitly set YUnit to nothing
  ass->setYUnitLabel("Attenuation factor");
  // A_s,sc - absorption factor for scattering in sample and absorption in both
  // sample and container
  MatrixWorkspace_sptr assc = create<HistoWorkspace>(*m_inputWS);
  assc->setDistribution(true); // The output of this is a distribution
  assc->setYUnit("");          // Need to explicitly set YUnit to nothing
  assc->setYUnitLabel("Attenuation factor");
  // A_c,c - absorption factor for scattering and self-absorption in container
  MatrixWorkspace_sptr acc = create<HistoWorkspace>(*m_inputWS);
  acc->setDistribution(true); // The output of this is a distribution
  acc->setYUnit("");          // Need to explicitly set YUnit to nothing
  acc->setYUnitLabel("Attenuation factor");
  // A_c,sc - absorption factor for scattering in container and absorption in
  // both sample and container
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
  message << "Numerical integration performed every " << m_xStep << " wavelength points";
  g_log.information(message.str());
  message.str("");

  // Calculate the cached values of L1, element volumes, and geometry size
  initialiseCachedDistances();
  if (m_sampleL1s.empty() || m_containerL1s.empty()) {
    throw std::runtime_error("Failed to define any initial scattering gauge volume for geometry");
  }

  const auto &spectrumInfo = m_inputWS->spectrumInfo();
  Progress prog(this, 0.0, 1.0, numHists);
  // Loop over the spectra
  PARALLEL_FOR_IF(Kernel::threadSafe(*m_inputWS, *ass, *assc, *acc, *acsc))
  for (int64_t i = 0; i < int64_t(numHists); ++i) {
    PARALLEL_START_INTERRUPT_REGION
    // Copy over bins
    ass->setSharedX(i, m_inputWS->sharedX(i));
    assc->setSharedX(i, m_inputWS->sharedX(i));
    acc->setSharedX(i, m_inputWS->sharedX(i));
    acsc->setSharedX(i, m_inputWS->sharedX(i));

    if (!spectrumInfo.hasDetectors(i)) {
      g_log.information() << "Spectrum " << i << " does not have a detector defined for it\n";
      continue;
    }
    if (spectrumInfo.isMasked(i)) {
      continue;
    }
    const auto &det = spectrumInfo.detector(i);

    // scattering and self-absorption in sample L2 distances, used for A_s,s and
    // A_s,sc
    std::vector<double> sample_L2s(m_numSampleVolumeElements);
    // scattering in sample and absorption in both sample and container L2
    // distance, used for A_s,sc
    std::vector<double> sample_container_L2s(m_numSampleVolumeElements);
    // absorption factor for scattering and self-absorption in container L2
    // distances, used for A_c,c and A_c,sc
    std::vector<double> container_L2s(m_numContainerVolumeElements);
    // scattering in container and absorption in both sample and container L2
    // distances, used for A_c,sc
    std::vector<double> container_sample_L2s(m_numContainerVolumeElements);

    calculateDistances(det, sample_L2s, sample_container_L2s, container_L2s, container_sample_L2s);

    const auto wavelengths = m_inputWS->points(i);
    // these need to have the minus sign applied still
    const auto sampleLinearCoefAbs = m_material.linearAbsorpCoef(wavelengths.cbegin(), wavelengths.cend());
    const auto containerLinearCoefAbs = m_containerMaterial.linearAbsorpCoef(wavelengths.cbegin(), wavelengths.cend());

    // Get a reference to the Y's in the output WS for storing the factors
    auto &assY = ass->mutableY(i);
    auto &asscY = assc->mutableY(i);
    auto &accY = acc->mutableY(i);
    auto &acscY = acsc->mutableY(i);

    // Loop through the bins in the current spectrum every m_xStep
    for (int64_t j = 0; j < specSize; j = j + m_xStep) {
      double integral = 0.0;
      double crossIntegral = 0.0;

      doIntegration(integral, crossIntegral, -sampleLinearCoefAbs[j], m_ampleLinearCoefTotScatt, m_sampleElementVolumes,
                    m_sampleL1s, sample_L2s, -containerLinearCoefAbs[j], m_containerLinearCoefTotScatt,
                    m_sample_containerL1s, sample_container_L2s, 0, m_numSampleVolumeElements);
      assY[j] = integral / m_sampleVolume;       // Divide by total volume of the shape
      asscY[j] = crossIntegral / m_sampleVolume; // Divide by total volume of the shape

      integral = 0.0;
      crossIntegral = 0.0;
      doIntegration(integral, crossIntegral, -containerLinearCoefAbs[j], m_containerLinearCoefTotScatt,
                    m_containerElementVolumes, m_containerL1s, container_L2s, -sampleLinearCoefAbs[j],
                    m_ampleLinearCoefTotScatt, m_container_sampleL1s, container_sample_L2s, 0,
                    m_numContainerVolumeElements);
      accY[j] = integral / m_containerVolume;       // Divide by total volume of the shape
      acscY[j] = crossIntegral / m_containerVolume; // Divide by total volume of the shape

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

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  g_log.information() << "Total number of elements in the integration was " << m_sampleL1s.size() << '\n';

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
}

/// Calculate the distances for L1 (for both self-absorption and
/// absorption by other object) and element size for each element in
/// the sample and container
void PaalmanPingsAbsorptionCorrection::initialiseCachedDistances() {
  // First, check if a 'gauge volume' has been defined. If not, it's the same as
  // the sample.
  auto integrationVolume = std::shared_ptr<const IObject>(m_sampleObject->clone());
  if (m_inputWS->run().hasProperty("GaugeVolume")) {
    integrationVolume = constructGaugeVolume();
  }

  auto raster = Geometry::Rasterize::calculate(m_beamDirection, *integrationVolume, *m_sampleObject, m_cubeSideSample);
  m_sampleVolume = raster.totalvolume;
  if (raster.l1.size() == 0)
    throw std::runtime_error("Failed to rasterize shape");
  // move over the information
  m_numSampleVolumeElements = raster.l1.size();
  m_sampleL1s = std::move(raster.l1);
  m_sampleElementPositions = std::move(raster.position);
  m_sampleElementVolumes = std::move(raster.volume);

  // now for the container
  integrationVolume = std::shared_ptr<const IObject>(m_containerObject->clone());
  if (m_inputWS->run().hasProperty("GaugeVolume")) {
    integrationVolume = constructGaugeVolume();
  }

  raster = Geometry::Rasterize::calculate(m_beamDirection, *integrationVolume, *m_containerObject, m_cubeSideContainer);
  m_containerVolume = raster.totalvolume;
  if (raster.l1.size() == 0)
    throw std::runtime_error("Failed to rasterize shape");
  // move over the information
  m_numContainerVolumeElements = raster.l1.size();
  m_containerL1s = std::move(raster.l1);
  m_containerElementPositions = std::move(raster.position);
  m_containerElementVolumes = std::move(raster.volume);

  // Get the L1s for the cross terms

  // L1s for absorbed by the container to be scattered by the sample
  m_sample_containerL1s.reserve(m_numSampleVolumeElements);
  for (size_t i = 0; i < m_numSampleVolumeElements; ++i) {
    Track outgoing(m_sampleElementPositions[i], -m_beamDirection);
    m_containerObject->interceptSurface(outgoing);
    m_sample_containerL1s[i] = outgoing.totalDistInsideObject();
  }

  // L1s for absorbed by the sample to be scattered by the container
  m_container_sampleL1s.reserve(m_numContainerVolumeElements);
  for (size_t i = 0; i < m_numContainerVolumeElements; ++i) {
    Track outgoing(m_containerElementPositions[i], -m_beamDirection);
    m_sampleObject->interceptSurface(outgoing);
    m_container_sampleL1s[i] = outgoing.totalDistInsideObject();
  }
}

std::shared_ptr<const Geometry::IObject> PaalmanPingsAbsorptionCorrection::constructGaugeVolume() {
  g_log.information("Calculating scattering within the gauge volume defined on "
                    "the input workspace");

  // Retrieve and create the gauge volume shape
  std::shared_ptr<const Geometry::IObject> volume =
      ShapeFactory().createShape(m_inputWS->run().getProperty("GaugeVolume")->value());

  return volume;
}

/// Fetch the properties and set the appropriate member variables
void PaalmanPingsAbsorptionCorrection::retrieveBaseProperties() {

  // get the material from the correct component
  const auto &sampleObj = m_inputWS->sample();
  m_material = sampleObj.getShape().material();
  m_containerMaterial = sampleObj.getEnvironment().getContainer().material();

  // NOTE: the angstrom^-2 to barns and the angstrom^-1 to cm^-1
  // will cancel for mu to give units: cm^-1
  m_ampleLinearCoefTotScatt = -m_material.totalScatterXSection() * m_material.numberDensityEffective() * 100;
  m_containerLinearCoefTotScatt =
      -m_containerMaterial.totalScatterXSection() * m_containerMaterial.numberDensityEffective() * 100;

  m_num_lambda = getProperty("NumberOfWavelengthPoints");

  // Call the virtual function for any further properties
  m_cubeSideSample = getProperty("ElementSize"); // in mm
  m_cubeSideSample *= 0.001;                     // now in m
  // use the same elementsize for container if not specified, otherwise use the specified value
  m_cubeSideContainer = getProperty("ContainerElementSize"); // in mm
  m_cubeSideContainer = isDefault("ContainerElementSize") ? m_cubeSideSample : m_cubeSideContainer * 1e-3;
}

/// Create the sample object using the Geometry classes, or use the existing one
void PaalmanPingsAbsorptionCorrection::constructSample(API::Sample &sample) {
  m_sampleObject = &sample.getShape();
  m_containerObject = &(sample.getEnvironment().getContainer());

  // Check there is one, and fail if not
  if (!m_sampleObject->hasValidShape()) {
    const std::string mess("No shape has been defined for the sample in the input workspace");
    g_log.error(mess);
    throw std::invalid_argument(mess);
  }
  // Check there is one, and fail if not
  if (!m_containerObject->hasValidShape()) {
    const std::string mess("No shape has been defined for the container in the input workspace");
    g_log.error(mess);
    throw std::invalid_argument(mess);
  }
}

/// Calculate the distances traversed by the neutrons within the sample
void PaalmanPingsAbsorptionCorrection::calculateDistances(const IDetector &detector, std::vector<double> &sample_L2s,
                                                          std::vector<double> &sample_container_L2s,
                                                          std::vector<double> &container_L2s,
                                                          std::vector<double> &container_sample_L2s) const {
  V3D detectorPos(detector.getPos());
  if (detector.nDets() > 1) {
    // We need to make sure this is right for grouped detectors - should use
    // average theta & phi
    detectorPos.spherical(detectorPos.norm(), detector.getTwoTheta(V3D(), V3D(0, 0, 1)) * 180.0 / M_PI,
                          detector.getPhi() * 180.0 / M_PI);
  }

  for (size_t i = 0; i < m_numSampleVolumeElements; ++i) {
    // Create track for distance between scattering point in sample and detector
    const V3D direction = normalize(detectorPos - m_sampleElementPositions[i]);
    Track outgoing(m_sampleElementPositions[i], direction);

    // find distance in sample
    m_sampleObject->interceptSurface(outgoing);
    sample_L2s[i] = outgoing.totalDistInsideObject();

    outgoing.clearIntersectionResults();

    // find distance in container
    m_containerObject->interceptSurface(outgoing);
    sample_container_L2s[i] = outgoing.totalDistInsideObject();
  }

  for (size_t i = 0; i < m_numContainerVolumeElements; ++i) {
    // Create track for distance between scattering point in container and
    // detector
    const V3D direction = normalize(detectorPos - m_containerElementPositions[i]);
    Track outgoing(m_containerElementPositions[i], direction);

    // find distance in container
    m_containerObject->interceptSurface(outgoing);
    container_L2s[i] = outgoing.totalDistInsideObject();

    outgoing.clearIntersectionResults();

    // find distance in sample
    m_sampleObject->interceptSurface(outgoing);
    container_sample_L2s[i] = outgoing.totalDistInsideObject();
  }
}

// the integrations are done using pairwise summation to reduce
// issues from adding lots of little numbers together
// https://en.wikipedia.org/wiki/Pairwise_summation

/// Carries out the numerical integration over the sample for elastic
/// instruments

void PaalmanPingsAbsorptionCorrection::doIntegration(double &integral, double &crossIntegral,
                                                     const double linearCoefAbs, const double linearCoefTotScatt,
                                                     const std::vector<double> &elementVolumes,
                                                     const std::vector<double> &L1s, const std::vector<double> &L2s,
                                                     const double linearCoefAbs2, const double linearCoefTotScatt2,
                                                     const std::vector<double> &L1s2, const std::vector<double> &L2s2,
                                                     const size_t startIndex, const size_t endIndex) const {
  if (endIndex - startIndex > MAX_INTEGRATION_LENGTH) {
    size_t middle = findMiddle(startIndex, endIndex);

    doIntegration(integral, crossIntegral, linearCoefAbs, linearCoefTotScatt, elementVolumes, L1s, L2s, linearCoefAbs2,
                  linearCoefTotScatt2, L1s2, L2s2, startIndex, middle);
    doIntegration(integral, crossIntegral, linearCoefAbs, linearCoefTotScatt, elementVolumes, L1s, L2s, linearCoefAbs2,
                  linearCoefTotScatt2, L1s2, L2s2, middle, endIndex);
  } else {

    // Iterate over all the elements, summing up the integral
    for (size_t i = startIndex; i < endIndex; ++i) {
      double exponent = (linearCoefAbs + linearCoefTotScatt) * (L1s[i] + L2s[i]);
      integral += (exp(exponent) * (elementVolumes[i]));
      exponent += (linearCoefAbs2 + linearCoefTotScatt2) * (L1s2[i] + L2s2[i]);
      crossIntegral += (exp(exponent) * (elementVolumes[i]));
    }
  }
}

} // namespace Mantid::Algorithms
