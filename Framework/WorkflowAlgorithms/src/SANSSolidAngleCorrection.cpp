//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/SANSSolidAngleCorrection.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceUnitValidator.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/PropertyManager.h"

namespace Mantid {
namespace WorkflowAlgorithms {

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SANSSolidAngleCorrection)

/// Returns the angle between the sample-to-pixel vector and its
/// projection on the X-Z plane.
static double getYTubeAngle(IDetector_const_sptr det,
                            MatrixWorkspace_const_sptr workspace) {

  // Get the sample position
  Geometry::IComponent_const_sptr sample =
      workspace->getInstrument()->getSample();
  const V3D samplePos = sample->getPos();

  // Get the vector from the sample position to the detector pixel
  V3D sampleDetVec = det->getPos() - samplePos;

  // Get the projection of that vector on the X-Z plane
  V3D inPlane = V3D(sampleDetVec);
  inPlane.setY(0.0);

  // This is the angle between the sample-to-detector vector
  // and its project on the X-Z plane.
  return sampleDetVec.angle(inPlane);
}

void SANSSolidAngleCorrection::init() {
  auto wsValidator = boost::make_shared<CompositeValidator>();
  wsValidator->add<WorkspaceUnitValidator>("Wavelength");
  wsValidator->add<HistogramValidator>();
  declareProperty(make_unique<WorkspaceProperty<>>(
      "InputWorkspace", "", Direction::Input, wsValidator));
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output));
  declareProperty("DetectorTubes", false, "If true, the algorithm will assume "
                                          "that the detectors are tubes in the "
                                          "Y direction.");
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
  if (!reductionManager->existsProperty("SolidAngleAlgorithm")) {
    auto algProp = make_unique<AlgorithmProperty>("SolidAngleAlgorithm");
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
    outputWS->isDistribution(true);
    outputWS->setYUnit("");
    outputWS->setYUnitLabel("Steradian");
    setProperty("OutputWorkspace", outputWS);
  }

  const int numHists = static_cast<int>(inputWS->getNumberHistograms());
  Progress progress(this, 0.0, 1.0, numHists);

  // Number of X bins
  const int xLength = static_cast<int>(inputWS->readY(0).size());

  PARALLEL_FOR2(outputWS, inputWS)
  for (int i = 0; i < numHists; ++i) {
    PARALLEL_START_INTERUPT_REGION
    outputWS->dataX(i) = inputWS->readX(i);

    IDetector_const_sptr det;
    try {
      det = inputWS->getDetector(i);
    } catch (Exception::NotFoundError &) {
      g_log.warning() << "Workspace index " << i
                      << " has no detector assigned to it - discarding"
                      << std::endl;
      // Catch if no detector. Next line tests whether this happened - test
      // placed
      // outside here because Mac Intel compiler doesn't like 'continue' in a
      // catch
      // in an openmp block.
    }
    // If no detector found, skip onto the next spectrum
    if (!det)
      continue;

    // Skip if we have a monitor or if the detector is masked.
    if (det->isMonitor() || det->isMasked())
      continue;

    const MantidVec &YIn = inputWS->readY(i);
    const MantidVec &EIn = inputWS->readE(i);

    MantidVec &YOut = outputWS->dataY(i);
    MantidVec &EOut = outputWS->dataE(i);

    // Compute solid angle correction factor
    const bool is_tube = getProperty("DetectorTubes");
    const double tanTheta = tan(inputWS->detectorTwoTheta(*det));
    const double theta_term = sqrt(tanTheta * tanTheta + 1.0);
    double corr;
    if (is_tube) {
      const double tanAlpha = tan(getYTubeAngle(det, inputWS));
      const double alpha_term = sqrt(tanAlpha * tanAlpha + 1.0);
      corr = alpha_term * theta_term * theta_term;
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

  const int numberOfSpectra =
      static_cast<int>(outputEventWS->getNumberHistograms());
  Progress progress(this, 0.0, 1.0, numberOfSpectra);
  progress.report("Solid Angle Correction");

  PARALLEL_FOR1(outputEventWS)
  for (int i = 0; i < numberOfSpectra; i++) {
    PARALLEL_START_INTERUPT_REGION
    IDetector_const_sptr det;
    try {
      det = outputEventWS->getDetector(i);
    } catch (Exception::NotFoundError &) {
      g_log.warning() << "Workspace index " << i
                      << " has no detector assigned to it - discarding"
                      << std::endl;
      // Catch if no detector. Next line tests whether this happened - test
      // placed
      // outside here because Mac Intel compiler doesn't like 'continue' in a
      // catch
      // in an openmp block.
    }
    if (!det)
      continue;

    // Skip if we have a monitor or if the detector is masked.
    if (det->isMonitor() || det->isMasked())
      continue;

    // Compute solid angle correction factor
    const bool is_tube = getProperty("DetectorTubes");
    const double tanTheta = tan(outputEventWS->detectorTwoTheta(*det));
    const double theta_term = sqrt(tanTheta * tanTheta + 1.0);
    double corr;
    if (is_tube) {
      const double tanAlpha = tan(getYTubeAngle(det, inputWS));
      const double alpha_term = sqrt(tanAlpha * tanAlpha + 1.0);
      corr = alpha_term * theta_term * theta_term;
    } else {
      corr = theta_term * theta_term * theta_term;
    }
    EventList &el = outputEventWS->getEventList(i);
    el *= corr;
    progress.report("Solid Angle Correction");
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  setProperty("OutputMessage", "Solid angle correction applied");
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
