#include "MantidAlgorithms/SpecularReflectionPositionCorrect2.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SpecularReflectionPositionCorrect2)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SpecularReflectionPositionCorrect2::name() const {
  return "SpecularReflectionPositionCorrect";
}

/// Algorithm's summary. @see Algorithm::summary
const std::string SpecularReflectionPositionCorrect2::summary() const {
  return "Corrects a detector component's position based on TwoTheta.";
}

/// Algorithm's version for identification. @see Algorithm::version
int SpecularReflectionPositionCorrect2::version() const { return 2; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SpecularReflectionPositionCorrect2::category() const {
  return "Reflectometry";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
*/
void SpecularReflectionPositionCorrect2::init() {

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input workspace to correct.");

  auto thetaValidator = boost::make_shared<CompositeValidator>();
  thetaValidator->add(boost::make_shared<MandatoryValidator<double>>());
  thetaValidator->add(
      boost::make_shared<BoundedValidator<double>>(0, 90, true));
  declareProperty(
      make_unique<PropertyWithValue<double>>("TwoTheta", Mantid::EMPTY_DBL(),
                                             thetaValidator, Direction::Input),
      "Angle used to correct the detector component.");

  const std::vector<std::string> correctionType{"VerticalShift",
                                                "RotateAroundSample"};
  auto correctionTypeValidator = boost::make_shared<CompositeValidator>();
  correctionTypeValidator->add(
      boost::make_shared<MandatoryValidator<std::string>>());
  correctionTypeValidator->add(
      boost::make_shared<StringListValidator>(correctionType));
  declareProperty(
      "DetectorCorrectionType", correctionType[0], correctionTypeValidator,
      "Whether detectors should be shifted vertically or rotated around the "
      "sample position.",
      Direction::Input);

  declareProperty(
      Mantid::Kernel::make_unique<PropertyWithValue<std::string>>(
          "DetectorComponentName", "",
          boost::make_shared<MandatoryValidator<std::string>>(),
          Direction::Input),
      "Name of the detector component to correct, i.e. point-detector");

  declareProperty(
      Mantid::Kernel::make_unique<PropertyWithValue<std::string>>(
          "SampleComponentName", "some-surface-holder", Direction::Input),
      "Name of the sample component, i.e. some-surface-holder");

  declareProperty(
      Mantid::Kernel::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "OutputWorkspace", "", Direction::Output),
      "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
*/
void SpecularReflectionPositionCorrect2::exec() {

  MatrixWorkspace_sptr inWS = this->getProperty("InputWorkspace");

  auto cloneWS = createChildAlgorithm("CloneWorkspace");
  cloneWS->initialize();
  cloneWS->setProperty("InputWorkspace", inWS);
  cloneWS->execute();
  Workspace_sptr tmp = cloneWS->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr outWS =
      boost::dynamic_pointer_cast<MatrixWorkspace>(tmp);

  const double twoThetaIn = getProperty("TwoTheta");
  const double twoThetaInRad = twoThetaIn * (M_PI / 180.0);

  auto inst = outWS->getInstrument();

  // Detector
  const std::string detectorName = getProperty("DetectorComponentName");
  if (!inst->getComponentByName(detectorName))
    throw std::runtime_error("Detector component not found.");
  IComponent_const_sptr detector = inst->getComponentByName(detectorName);
  const V3D detectorPosition = detector->getPos();

  // Sample
  const std::string sampleName = getProperty("SampleComponentName");
  if (!inst->getComponentByName(sampleName))
    throw std::runtime_error("Sample component not found.");
  IComponent_const_sptr sample = inst->getComponentByName(sampleName);
  const V3D samplePosition = sample->getPos();

  // Type of movement (vertical shift or rotation around the sample)
  const std::string correctionType = getProperty("DetectorCorrectionType");

  // Sample-to-detector
  const V3D sampleToDetector = detectorPosition - samplePosition;
  // Reference frame
  auto referenceFrame = inst->getReferenceFrame();
  auto beamAxis = referenceFrame->pointingAlongBeamAxis();
  auto horizontalAxis = referenceFrame->pointingHorizontalAxis();
  auto upAxis = referenceFrame->pointingUpAxis();

  // Get the offset from the sample in the beam direction
  double beamOffset = 0.0;
  const double beamOffsetOld =
      sampleToDetector.scalar_prod(referenceFrame->vecPointingAlongBeam());

  if (correctionType == "VerticalShift") {
    // Only shifting vertically, so the beam offset remains the same
    beamOffset = beamOffsetOld;
  } else if (correctionType == "RotateAroundSample") {
    // The radius for the rotation is the distance from the sample to
    // the detector in the Beam-Vertical plane
    const double upOffsetOld =
        sampleToDetector.scalar_prod(referenceFrame->vecPointingUp());
    const double radius =
        std::sqrt(beamOffsetOld * beamOffsetOld + upOffsetOld * upOffsetOld);
    beamOffset = radius * std::cos(twoThetaInRad);
  } else {
    // Shouldn't get here unless there's been a coding error
    std::ostringstream message;
    message << "Invalid correction type '" << correctionType;
    throw std::runtime_error(message.str());
  }

  // Calculate the offset in the vertical direction, and the total
  // offset in the beam direction
  const double upOffset = (beamOffset * std::tan(twoThetaInRad));
  const double beamOffsetFromOrigin =
      beamOffset +
      samplePosition.scalar_prod(referenceFrame->vecPointingAlongBeam());

  auto moveAlg = createChildAlgorithm("MoveInstrumentComponent");
  moveAlg->initialize();
  moveAlg->setProperty("Workspace", outWS);
  moveAlg->setProperty("ComponentName", detectorName);
  moveAlg->setProperty("RelativePosition", false);
  moveAlg->setProperty(beamAxis, beamOffsetFromOrigin);
  moveAlg->setProperty(horizontalAxis, 0.0);
  moveAlg->setProperty(upAxis, upOffset);
  moveAlg->execute();

  setProperty("OutputWorkspace", outWS);
}

} // namespace Algorithms
} // namespace Mantid
