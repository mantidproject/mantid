#include "MantidAlgorithms/ReflectometryCorrectDetectorAngle.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/MandatoryValidator.h"

using Mantid::Geometry::deg2rad;
using Mantid::Geometry::rad2deg;

namespace {
/// This namespace contains algorithm's property names.
namespace Prop {
std::string const DETECTOR_COMPONENT{"DetectorComponent"};
std::string const DIRECT_LINE_POS{"DirectLinePosition"};
std::string const DIRECT_WS{"DirectBeamWorkspace"};
std::string const LINE_POS{"LinePosition"};
std::string const INPUT_WS{"InputWorkspace"};
std::string const OUTPUT_WS{"OutputWorkspace"};
std::string const PIXEL_SIZE{"PixelSize"};
std::string const TWO_THETA{"TwoTheta"};
} // namespace Prop

/// Enumerations to define the rotation plane of the detector.
enum class RotationPlane { horizontal, vertical };

/** Return the fractional workspace index to the centre pixel.
 *  @param spectrumInfo a spectrum info
 *  @return the centre's fractional workspace index
 */
double
detectorCentreIndex(Mantid::API::SpectrumInfo const &spectrumInfo) noexcept {
  return static_cast<double>(spectrumInfo.size() - 1) / 2.;
}

/** Return true if twoTheta increases with workspace index.
 *  @param spectrumInfo a spectrum info
 *  @return true if twoTheta increases with workspace index
 */
bool isAngleIncreasingWithIndex(Mantid::API::SpectrumInfo const &spectrumInfo) {
  auto const first = spectrumInfo.signedTwoTheta(0);
  auto const last = spectrumInfo.signedTwoTheta(spectrumInfo.size() - 1);
  return first < last;
}

/** Calculate the detector position from given parameters.
 *  @param plane rotation plane of the detector
 *  @param distance sample to detector centre distance in meters
 *  @param angle an angle between the Z axis and the detector in radians
 *  @return a vector pointing to the new detector centre
 */
Mantid::Kernel::V3D detectorPosition(Mantid::API::MatrixWorkspace const &ws,
                                     RotationPlane const plane,
                                     double const distance,
                                     double const angle) {
  auto const instrument = ws.getInstrument();
  auto const referenceFrame = instrument->getReferenceFrame();
  Mantid::Kernel::V3D position;
  switch (plane) {
  case RotationPlane::horizontal:
    position[referenceFrame->pointingHorizontal()] = distance * std::sin(angle);
    position[referenceFrame->pointingUp()] = 0.;
    position[referenceFrame->pointingAlongBeam()] = distance * std::cos(angle);
    break;
  case RotationPlane::vertical:
    position[referenceFrame->pointingHorizontal()] = 0.;
    position[referenceFrame->pointingUp()] = distance * std::sin(angle);
    position[referenceFrame->pointingAlongBeam()] = distance * std::cos(angle);
    break;
  }
  return position;
}

/** Return the rotation plane of a reflectometry workspace.
 *  @param ws a workspace
 *  @return the rotation plane
 */
RotationPlane rotationPlane(Mantid::API::MatrixWorkspace const &ws) {
  auto const instrument = ws.getInstrument();
  auto const referenceFrame = instrument->getReferenceFrame();
  auto const thetaSignAxis = referenceFrame->vecThetaSign();
  auto const up = referenceFrame->vecPointingUp();
  return thetaSignAxis == up ? RotationPlane::vertical
                             : RotationPlane::horizontal;
}

/** Return the axis for the detector face rotation.
 *
 *  The detector centre normal should point to the sample in the end.
 *  @param ws a workspace
 *  @param plane the rotation plane
 *  @return the rotation axis
 */
Mantid::Kernel::V3D faceRotationAxis(Mantid::API::MatrixWorkspace const &ws,
                                     RotationPlane const plane) {
  auto const instrument = ws.getInstrument();
  auto const referenceFrame = instrument->getReferenceFrame();
  auto const sign =
      referenceFrame->getHandedness() == Mantid::Geometry::Right ? 1. : -1.;
  Mantid::Kernel::V3D rotationAxis;
  switch (plane) {
  case RotationPlane::horizontal:
    rotationAxis[referenceFrame->pointingUp()] = sign;
    break;
  case RotationPlane::vertical:
    rotationAxis[referenceFrame->pointingHorizontal()] = -sign;
    break;
  }
  return rotationAxis;
}
} // namespace

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ReflectometryCorrectDetectorAngle)

/// Algorithms name for identification. @see Algorithm::name
const std::string ReflectometryCorrectDetectorAngle::name() const {
  return "ReflectometryCorrectDetectorAngle";
}

/// Algorithm's version for identification. @see Algorithm::version
int ReflectometryCorrectDetectorAngle::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ReflectometryCorrectDetectorAngle::category() const {
  return "Reflectometry;ILL\\Reflectometry";
}

/// Return a vector of related algorithm names.
const std::vector<std::string>
ReflectometryCorrectDetectorAngle::seeAlso() const {
  return {"SpecularReflectionPositionCorrect"};
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ReflectometryCorrectDetectorAngle::summary() const {
  return "Corrects the angle of a reflectometry line detector.";
}

/** Initialize the algorithm's properties.
 */
void ReflectometryCorrectDetectorAngle::init() {
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          Prop::INPUT_WS, "", Kernel::Direction::Input),
      "A reflectometry line detector workspace.");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          Prop::OUTPUT_WS, "", Kernel::Direction::Output),
      "A detector angle corrected result.");
  auto mandatoryString =
      boost::make_shared<Kernel::MandatoryValidator<std::string>>();
  declareProperty(Prop::DETECTOR_COMPONENT, "", mandatoryString,
                  "Name of the detector component to move.");
  auto positiveDouble = boost::make_shared<Kernel::BoundedValidator<double>>();
  positiveDouble->setLower(0.);
  declareProperty(Prop::LINE_POS, EMPTY_DBL(), positiveDouble,
                  "A possibly fractional workspace index for the line centre.");
  declareProperty(Prop::TWO_THETA, EMPTY_DBL(),
                  "Angle of the detector centre "
                  "with respect to the beam "
                  "axis, in degrees.");
  declareProperty(Prop::PIXEL_SIZE, EMPTY_DBL(), positiveDouble,
                  "Size of a detector pixel, in metres.");
  declareProperty(
      Kernel::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          Prop::DIRECT_WS, "", Kernel::Direction::Input,
          API::PropertyMode::Optional),
      "A reference direct beam workspace.");
  declareProperty(
      Prop::DIRECT_LINE_POS, EMPTY_DBL(), positiveDouble,
      "A possibly fractional workspace index for the direct line centre.");
}

/** Execute the algorithm.
 */
void ReflectometryCorrectDetectorAngle::exec() {
  API::MatrixWorkspace_sptr inputWS = getProperty(Prop::INPUT_WS);
  API::MatrixWorkspace_sptr outputWS;
  if (getPropertyValue(Prop::INPUT_WS) == getPropertyValue(Prop::OUTPUT_WS)) {
    outputWS = inputWS;
  } else {
    outputWS = inputWS->clone();
  }
  ComponentPositions const positions = sampleAndDetectorPositions(*inputWS);
  auto const twoTheta = correctedTwoTheta(*inputWS, positions.l2);
  correctDetectorPosition(outputWS, positions, twoTheta);
  setProperty(Prop::OUTPUT_WS, outputWS);
}

/// Validate the algorithm's input properties.
std::map<std::string, std::string>
ReflectometryCorrectDetectorAngle::validateInputs() {
  std::map<std::string, std::string> issues;
  if (!isDefault(Prop::DIRECT_WS)) {
    if (isDefault(Prop::DIRECT_LINE_POS)) {
      issues[Prop::DIRECT_LINE_POS] = "Direct beam position has to be given "
                                      "when using a direct beam reference.";
    }
    if (isDefault(Prop::PIXEL_SIZE)) {
      issues[Prop::PIXEL_SIZE] =
          "Pixel size is needed for direct beam calibration.";
    }
  } else {
    if (isDefault(Prop::TWO_THETA)) {
      issues[Prop::TWO_THETA] =
          "An angle must be given when no direct beam reference is used.";
    }
    if (!isDefault(Prop::LINE_POS) && isDefault(Prop::PIXEL_SIZE)) {
      issues[Prop::PIXEL_SIZE] = "Pixel size is needed for angle correction.";
    }
  }
  return issues;
}

/** Move and rotate the detector around the sample.
 * @param ws a workspace
 * @param positions a ComponentPositions object
 * @param twoTheta the rotation angle, in radians
 */
void ReflectometryCorrectDetectorAngle::correctDetectorPosition(
    API::MatrixWorkspace_sptr &ws, ComponentPositions const &positions,
    double const twoTheta) {
  std::string const componentName = getProperty(Prop::DETECTOR_COMPONENT);
  auto const rotPlane = rotationPlane(*ws);
  auto const newPosition =
      detectorPosition(*ws, rotPlane, positions.l2, twoTheta);
  moveComponent(ws, componentName, newPosition + positions.sample);
  // apply a local rotation so detector centre normal points to the sample
  auto const rotationAxis = faceRotationAxis(*ws, rotPlane);
  rotateComponent(ws, componentName, rotationAxis, twoTheta);
}

/** Compute the detector rotation angle around sample.
 *  @param ws a workspace
 *  @param l2 sample to detector centre distance
 *  @return a rotation angle in radians
 */
double ReflectometryCorrectDetectorAngle::correctedTwoTheta(
    API::MatrixWorkspace const &ws, double const l2) {
  if (!isDefault(Prop::TWO_THETA)) {
    auto const twoTheta =
        static_cast<double>(getProperty(Prop::TWO_THETA)) * deg2rad;
    if (isDefault(Prop::LINE_POS)) {
      return twoTheta;
    } else {
      double const linePosition = getProperty(Prop::LINE_POS);
      double const offset = offsetAngleFromCentre(ws, l2, linePosition);
      return twoTheta - offset;
    }
  } else {
    API::MatrixWorkspace_sptr directWS = getProperty(Prop::DIRECT_WS);
    double const directLinePosition = getProperty(Prop::DIRECT_LINE_POS);
    const double directOffset =
        offsetAngleFromCentre(*directWS, l2, directLinePosition);
    m_log.debug() << "Direct beam offset angle: " << directOffset * rad2deg
                  << '\n';
    auto const reflectedDetectorAngle = signedDetectorAngle(ws);
    auto const directDetectorAngle = signedDetectorAngle(*directWS);
    auto const angle =
        reflectedDetectorAngle - directDetectorAngle - directOffset;
    m_log.debug() << "Direct beam calibrated detector angle: "
                  << angle * rad2deg << '\n';
    return angle;
  }
}

/** Move a component to given position.
 * @param ws a workspace
 * @param name component's name
 * @param position an absolute position
 */
void ReflectometryCorrectDetectorAngle::moveComponent(
    API::MatrixWorkspace_sptr &ws, std::string const &name,
    Kernel::V3D const &position) {
  auto moveComp = createChildAlgorithm("MoveInstrumentComponent");
  moveComp->setProperty("Workspace", ws);
  moveComp->setProperty("ComponentName", name);
  moveComp->setProperty("X", position.X());
  moveComp->setProperty("Y", position.Y());
  moveComp->setProperty("Z", position.Z());
  moveComp->setProperty("RelativePosition", false);
  moveComp->execute();
}

/** Calculate a pixel's offset angle from detector centre.
 * @param ws a workspace
 * @param l2 sample to detector distance
 * @param linePosition pixel's workspace index
 * @return the offset angle, in radians
 */
double ReflectometryCorrectDetectorAngle::offsetAngleFromCentre(
    API::MatrixWorkspace const &ws, double const l2,
    double const linePosition) {
  auto const &spectrumInfo = ws.spectrumInfo();
  if (linePosition > spectrumInfo.size() - 1) {
    std::ostringstream msg;
    msg << Prop::LINE_POS << " is greater than the maximum workspace index "
        << spectrumInfo.size() - 1;
    throw std::runtime_error(msg.str());
  }
  auto const centreIndex = detectorCentreIndex(spectrumInfo);
  double const pixelSize = getProperty(Prop::PIXEL_SIZE);
  auto const sign = isAngleIncreasingWithIndex(spectrumInfo) ? -1. : 1.;
  double const offsetWidth = (centreIndex - linePosition) * pixelSize;
  return sign * std::atan2(offsetWidth, l2);
}

/** Rotate the detector's face.
 * @param ws a workspace
 * @param name the detector component's name
 * @param rotationAxis a rotation axis
 * @param angle the rotation angle, in radians
 */
void ReflectometryCorrectDetectorAngle::rotateComponent(
    API::MatrixWorkspace_sptr &ws, std::string const &name,
    Kernel::V3D const &rotationAxis, double const angle) {
  auto rotate = createChildAlgorithm("RotateInstrumentComponent");
  rotate->setProperty("Workspace", ws);
  rotate->setProperty("ComponentName", name);
  rotate->setProperty("X", rotationAxis.X());
  rotate->setProperty("Y", rotationAxis.Y());
  rotate->setProperty("Z", rotationAxis.Z());
  rotate->setProperty("RelativeRotation", false);
  rotate->setProperty("Angle", angle * rad2deg);
  rotate->execute();
}

/** Return a ComponentPositions object for given workspace.
 * @param ws a workspace
 * @return a ComponentPositions object
 * @throw NotFoundError if given instrument component does not exist.
 */
ReflectometryCorrectDetectorAngle::ComponentPositions
ReflectometryCorrectDetectorAngle::sampleAndDetectorPositions(
    API::MatrixWorkspace const &ws) {
  ComponentPositions positions;
  auto const instrument = ws.getInstrument();
  std::string const componentName = getProperty(Prop::DETECTOR_COMPONENT);
  auto const detector = instrument->getComponentByName(componentName);
  if (!detector) {
    throw Kernel::Exception::NotFoundError(
        "Detector component does not exists:", componentName);
  }
  positions.detector = detector->getPos();
  auto const sample = instrument->getSample();
  positions.sample = sample->getPos();
  positions.l2 = positions.sample.distance(positions.detector);
  return positions;
}

/** Calculate the signed angle between the sample and the detector centre.
 * @param ws a workspace
 * @return the signed twoTheta angle, in radians
 */
double ReflectometryCorrectDetectorAngle::signedDetectorAngle(
    API::MatrixWorkspace const &ws) {
  auto const instrument = ws.getInstrument();
  std::string const componentName = getProperty(Prop::DETECTOR_COMPONENT);
  auto const detector = instrument->getComponentByName(componentName);
  auto const detectorPos = detector->getPos();
  auto const sample = instrument->getSample();
  auto const samplePos = sample->getPos();
  auto const beamDir = instrument->getBeamDirection();
  auto const referenceFrame = instrument->getReferenceFrame();
  auto const thetaSignAxis = referenceFrame->vecThetaSign();
  auto const detectorDirection = detectorPos - samplePos;
  auto const up = referenceFrame->vecPointingUp();
  auto const thetaSignV3DComponent = thetaSignAxis == up
                                         ? referenceFrame->pointingUp()
                                         : referenceFrame->pointingHorizontal();
  auto const angleSign =
      detectorDirection[thetaSignV3DComponent] >= 0 ? 1. : -1.;
  return angleSign * detectorDirection.angle(beamDir);
}

} // namespace Algorithms
} // namespace Mantid
