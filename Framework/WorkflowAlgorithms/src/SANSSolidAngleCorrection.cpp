// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidWorkflowAlgorithms/SANSSolidAngleCorrection.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"

namespace Mantid {
namespace WorkflowAlgorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SANSSolidAngleCorrection)

namespace {

/// Returns the angle between the sample-to-pixel vector and its
/// projection on the X-Z plane.
static double getYTubeAngle(const SpectrumInfo &spectrumInfo, size_t i) {
  const V3D samplePos = spectrumInfo.samplePosition();

  // Get the vector from the sample position to the detector pixel
  V3D sampleDetVec = spectrumInfo.position(i) - samplePos;

  // Get the projection of that vector on the X-Z plane
  V3D inPlane = V3D(sampleDetVec);
  inPlane.setY(0.0);

  // This is the angle between the sample-to-detector vector
  // and its project on the X-Z plane.
  return sampleDetVec.angle(inPlane);
}

} // namespace

//----------------------------------------------------------------------------------------------
void SANSSolidAngleCorrection::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<HistogramValidator>();
  declareProperty(std::make_unique<WorkspaceProperty<>>(
      "InputWorkspace", "", Direction::Input, wsValidator));
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                        Direction::Output));
  declareProperty("DetectorTubes", false,
                  "If true, the algorithm will assume "
                  "that the detectors are tubes in the "
                  "Y direction.");
  declareProperty("DetectorWing", false,
                  "If true, the algorithm will assume "
                  "that the detector is curved around the sample. E.g. BIOSANS "
                  "Wing detector.");
  declareProperty("OutputMessage", "", Direction::Output);
  declareProperty("ReductionProperties", "__sans_reduction_properties",
                  Direction::Input);
}

void SANSSolidAngleCorrection::exec() {
  // Reduction property manager
  const std::string reductionManagerName = getProperty("ReductionProperties");
  boost::shared_ptr<PropertyManager> reductionManager;
  if (PropertyManagerDataService::Instance().doesExist(reductionManagerName)) {
    reductionManager =
        PropertyManagerDataService::Instance().retrieve(reductionManagerName);
  } else {
    reductionManager = boost::make_shared<PropertyManager>();
    PropertyManagerDataService::Instance().addOrReplace(reductionManagerName,
                                                        reductionManager);
  }

  // If the solid angle algorithm isn't in the reduction properties, add it
  if (!reductionManager->existsProperty("SANSSolidAngleCorrection")) {
    auto algProp =
        std::make_unique<AlgorithmProperty>("SANSSolidAngleCorrection");
    algProp->setValue(toString());
    reductionManager->declareProperty(std::move(algProp));
  }

  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  DataObjects::EventWorkspace_const_sptr inputEventWS =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (inputEventWS)
    return execEvent();

  // Now create the output workspace
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != inputWS) {
    outputWS = WorkspaceFactory::Instance().create(inputWS);
    outputWS->setDistribution(true);
    outputWS->setYUnit("");
    outputWS->setYUnitLabel("Steradian");
    setProperty("OutputWorkspace", outputWS);
  }

  const auto numHists = static_cast<int>(inputWS->getNumberHistograms());
  Progress progress(this, 0.0, 1.0, numHists);

  // Number of X bins
  const auto xLength = static_cast<int>(inputWS->y(0).size());

  const auto &spectrumInfo = inputWS->spectrumInfo();
  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS, *inputWS))
  for (int i = 0; i < numHists; ++i) {
    PARALLEL_START_INTERUPT_REGION
    outputWS->setSharedX(i, inputWS->sharedX(i));

    if (!spectrumInfo.hasDetectors(i)) {
      g_log.warning() << "Workspace index " << i
                      << " has no detector assigned to it - discarding\n";
      continue;
    }

    // Skip if we have a monitor or if the detector is masked.
    if (spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i))
      continue;

    const auto &YIn = inputWS->y(i);
    const auto &EIn = inputWS->e(i);

    auto &YOut = outputWS->mutableY(i);
    auto &EOut = outputWS->mutableE(i);

    // Compute solid angle correction factor
    const bool is_tube = getProperty("DetectorTubes");
    const bool is_wing = getProperty("DetectorWing");

    const double tanTheta = tan(spectrumInfo.twoTheta(i));
    const double theta_term = sqrt(tanTheta * tanTheta + 1.0);
    double corr;
    if (is_tube || is_wing) {
      const double tanAlpha = tan(getYTubeAngle(spectrumInfo, i));
      const double alpha_term = sqrt(tanAlpha * tanAlpha + 1.0);
      if (is_tube)
        corr = alpha_term * theta_term * theta_term;
      else { // is_wing
        corr = alpha_term * alpha_term * alpha_term;
      }
    } else {
      corr = theta_term * theta_term * theta_term;
    }

    // Correct data for all X bins
    for (int j = 0; j < xLength; j++) {
      YOut[j] = YIn[j] * corr;
      EOut[j] = fabs(EIn[j] * corr);
    }
    progress.report("Solid Angle Correction");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
  setProperty("OutputMessage", "Solid angle correction applied");
}

void SANSSolidAngleCorrection::execEvent() {
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");

  // generate the output workspace pointer
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != inputWS) {
    outputWS = inputWS->clone();
    setProperty("OutputWorkspace", outputWS);
  }
  auto outputEventWS = boost::dynamic_pointer_cast<EventWorkspace>(outputWS);

  const auto numberOfSpectra =
      static_cast<int>(outputEventWS->getNumberHistograms());
  Progress progress(this, 0.0, 1.0, numberOfSpectra);
  progress.report("Solid Angle Correction");

  const auto &spectrumInfo = outputEventWS->spectrumInfo();
  PARALLEL_FOR_IF(Kernel::threadSafe(*outputEventWS))
  for (int i = 0; i < numberOfSpectra; i++) {
    PARALLEL_START_INTERUPT_REGION

    if (!spectrumInfo.hasDetectors(i)) {
      g_log.warning() << "Workspace index " << i
                      << " has no detector assigned to it - discarding\n";
      continue;
    }

    // Skip if we have a monitor or if the detector is masked.
    if (spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i))
      continue;

    // Compute solid angle correction factor
    const bool is_tube = getProperty("DetectorTubes");
    const double tanTheta = tan(spectrumInfo.twoTheta(i));
    const double theta_term = sqrt(tanTheta * tanTheta + 1.0);
    double corr;
    if (is_tube) {
      const double tanAlpha = tan(getYTubeAngle(spectrumInfo, i));
      const double alpha_term = sqrt(tanAlpha * tanAlpha + 1.0);
      corr = alpha_term * theta_term * theta_term;
    } else {
      corr = theta_term * theta_term * theta_term;
    }
    EventList &el = outputEventWS->getSpectrum(i);
    el *= corr;
    progress.report("Solid Angle Correction");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputMessage", "Solid angle correction applied");
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
