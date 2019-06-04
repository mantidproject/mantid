// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/SpecularReflectionPositionCorrect2.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace {
/// Enumerations to define the rotation plane of the detector.
enum class Plane { horizontal, vertical };

/** Return true if twoTheta increases with workspace index.
 *  @param spectrumInfo a spectrum info
 *  @return true if twoTheta increases with workspace index
 */
bool isAngleIncreasingWithIndex(const Mantid::API::SpectrumInfo &spectrumInfo) {
  const auto first = spectrumInfo.signedTwoTheta(0);
  const auto last = spectrumInfo.signedTwoTheta(spectrumInfo.size() - 1);
  return first < last;
}

/** Calculate a pixel's offset angle from detector centre.
 * @param ws a workspace
 * @param l2 sample to detector distance
 * @param linePosition pixel's workspace index
 * @return the offset angle, in radians
 */
double offsetAngleFromCentre(const MatrixWorkspace &ws, const double l2,
                             const double linePosition,
                             const double pixelSize) {
  const auto &spectrumInfo = ws.spectrumInfo();
  const size_t maxWorkspaceIndex = spectrumInfo.size() - 1;
  double const specSize = static_cast<double>(maxWorkspaceIndex);
  if (linePosition > specSize) {
    std::ostringstream msg;
    msg << "LinePosition is greater than the maximum workspace index "
        << maxWorkspaceIndex;
    throw std::runtime_error(msg.str());
  }
  auto const centreIndex = specSize / 2.;
  auto const sign = isAngleIncreasingWithIndex(spectrumInfo) ? -1. : 1.;
  double const offsetWidth = (centreIndex - linePosition) * pixelSize;
  return sign * std::atan2(offsetWidth, l2);
}
} // namespace

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
  return "Corrects a reflectometer's detector component's position.";
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

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "An input workspace to correct.");

  declareProperty(std::make_unique<PropertyWithValue<double>>(
                      "TwoTheta", Mantid::EMPTY_DBL(), Direction::Input),
                  "Angle used to correct the detector component [degrees].");

  const std::vector<std::string> correctionType{"VerticalShift",
                                                "RotateAroundSample"};
  auto correctionTypeValidator =
      boost::make_shared<StringListValidator>(correctionType);
  declareProperty(
      "DetectorCorrectionType", correctionType[0], correctionTypeValidator,
      "Whether detectors should be shifted vertically or rotated around the "
      "sample position.",
      Direction::Input);

  declareProperty(
      std::make_unique<PropertyWithValue<std::string>>(
          "DetectorComponentName", "", Direction::Input),
      "Name of the detector component to correct, for example point-detector");
  auto nonNegativeInt = boost::make_shared<Kernel::BoundedValidator<int>>();
  nonNegativeInt->setLower(0);
  declareProperty("DetectorID", EMPTY_INT(), nonNegativeInt,
                  "The ID of the detector to correct; if both "
                  "the component name and the detector ID "
                  "are set the latter will be used.");

  declareProperty(
      std::make_unique<PropertyWithValue<std::string>>(
          "SampleComponentName", "some-surface-holder", Direction::Input),
      "Name of the sample component; if the given name is not found in the "
      "instrument, the default sample is used.");

  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "OutputWorkspace", "", Direction::Output),
      "A workspace with corrected detector position.");
  declareProperty("DetectorFacesSample", false,
                  "If true, a normal vector at the centre of the detector "
                  "always points towards the sample.");
  auto nonNegativeDouble =
      boost::make_shared<Kernel::BoundedValidator<double>>();
  nonNegativeDouble->setLower(0.);
  declareProperty("LinePosition", EMPTY_DBL(), nonNegativeDouble,
                  "A fractional workspace index for the specular line centre.");
  declareProperty("DirectLinePosition", EMPTY_DBL(), nonNegativeDouble,
                  "A fractional workspace index for the direct line centre.");
  auto positiveDouble = boost::make_shared<Kernel::BoundedValidator<double>>();
  nonNegativeDouble->setLowerExclusive(0.);
  declareProperty("PixelSize", EMPTY_DBL(), positiveDouble,
                  "Size of a detector pixel, in metres.");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
          "DirectLineWorkspace", "", Direction::Input, PropertyMode::Optional),
      "A direct beam workspace for reference.");
}

/// Validate the algorithm's inputs.
std::map<std::string, std::string>
SpecularReflectionPositionCorrect2::validateInputs() {
  std::map<std::string, std::string> issues;
  if (isDefault("DetectorID") && isDefault("DetectorComponentName")) {
    issues["DetectorID"] =
        "DetectorID or DetectorComponentName has to be specified.";
  }
  if (!isDefault("TwoTheta")) {
    if (!isDefault("LinePosition") && isDefault("PixelSize")) {
      issues["PixelSize"] = "Pixel size required.";
    }
  } else {
    if (isDefault("DirectLinePosition")) {
      issues["DirectLinePosition"] =
          "Direct line position required when no TwoTheta supplied.";
    }
    if (isDefault("DirectLineWorkspace")) {
      issues["DirectLineWorkspace"] =
          "Direct beam workspace required when no TwoTheta supplied.";
    }
    if (isDefault("PixelSize")) {
      issues["PixelSize"] = "Pixel size required for direct beam calibration.";
    }
  }
  return issues;
}

/** Execute the algorithm.
 */
void SpecularReflectionPositionCorrect2::exec() {

  MatrixWorkspace_const_sptr inWS = this->getProperty("InputWorkspace");
  MatrixWorkspace_sptr outWS = this->getProperty("OutputWorkspace");
  if (outWS != inWS) {
    outWS = inWS->clone();
  }

  // Sample
  const V3D samplePosition = declareSamplePosition(*inWS);

  // Type of movement (vertical shift or rotation around the sample)
  const std::string correctionType = getProperty("DetectorCorrectionType");
  // Detector
  const auto inst = inWS->getInstrument();
  const detid_t detectorID = static_cast<int>(getProperty("DetectorID"));
  const std::string detectorName = getProperty("DetectorComponentName");
  const V3D detectorPosition =
      declareDetectorPosition(*inst, detectorName, detectorID);

  // Sample-to-detector
  const V3D sampleToDetector = detectorPosition - samplePosition;
  const double l2 = sampleToDetector.norm();
  const auto referenceFrame = inst->getReferenceFrame();
  const auto alongDir = referenceFrame->vecPointingAlongBeam();
  const double beamOffsetOld = sampleToDetector.scalar_prod(alongDir);

  const double twoThetaInRad =
      isDefault("TwoTheta")
          ? twoThetaFromDirectLine(detectorName, detectorID, samplePosition, l2,
                                   alongDir, beamOffsetOld)
          : twoThetaFromProperties(*inWS, l2);

  correctDetectorPosition(outWS, detectorName, detectorID, twoThetaInRad,
                          correctionType, *referenceFrame, samplePosition,
                          sampleToDetector, beamOffsetOld);
  setProperty("OutputWorkspace", outWS);
}

/**
 * Move and rotate the detector to its correct position
 * @param outWS the workspace to modify
 * @param detectorName name of the detector component
 * @param detectorID detector component's id
 * @param twoThetaInRad beam-detector angle in radians
 * @param correctionType type of correction
 * @param referenceFrame instrument's reference frame
 * @param samplePosition sample position
 * @param sampleToDetector a vector from sample to detector
 * @param beamOffsetOld sample detector distance on the beam axis
 */
void SpecularReflectionPositionCorrect2::correctDetectorPosition(
    MatrixWorkspace_sptr &outWS, const std::string &detectorName,
    const detid_t detectorID, const double twoThetaInRad,
    const std::string &correctionType, const ReferenceFrame &referenceFrame,
    const V3D &samplePosition, const V3D &sampleToDetector,
    const double beamOffsetOld) {
  const auto beamAxis = referenceFrame.pointingAlongBeamAxis();
  const auto horizontalAxis = referenceFrame.pointingHorizontalAxis();
  const auto upAxis = referenceFrame.pointingUpAxis();
  const auto thetaSignDir = referenceFrame.vecThetaSign();
  const auto upDir = referenceFrame.vecPointingUp();
  const auto plane = thetaSignDir.scalar_prod(upDir) == 1. ? Plane::vertical
                                                           : Plane::horizontal;

  // Get the offset from the sample in the beam direction
  double beamOffset = 0.0;

  if (correctionType == "VerticalShift") {
    // Only shifting vertically, so the beam offset remains the same
    beamOffset = beamOffsetOld;
  } else if (correctionType == "RotateAroundSample") {
    // The radius for the rotation is the distance from the sample to
    // the detector in the Beam-Vertical plane
    const double perpendicularOffsetOld =
        sampleToDetector.scalar_prod(thetaSignDir);
    const double radius = std::hypot(beamOffsetOld, perpendicularOffsetOld);
    beamOffset = radius * std::cos(twoThetaInRad);
  } else {
    // Shouldn't get here unless there's been a coding error
    std::ostringstream message;
    message << "Invalid correction type '" << correctionType << "'";
    throw std::runtime_error(message.str());
  }

  // Calculate the offset in the vertical direction, and the total
  // offset in the beam direction
  const double perpendicularOffset = beamOffset * std::tan(twoThetaInRad);
  const double beamOffsetFromOrigin =
      beamOffset +
      samplePosition.scalar_prod(referenceFrame.vecPointingAlongBeam());

  auto moveAlg = createChildAlgorithm("MoveInstrumentComponent");
  moveAlg->setProperty("Workspace", outWS);
  if (!detectorName.empty()) {
    moveAlg->setProperty("ComponentName", detectorName);
  } else {
    moveAlg->setProperty("DetectorID", detectorID);
  }
  moveAlg->setProperty("RelativePosition", false);
  moveAlg->setProperty(beamAxis, beamOffsetFromOrigin);
  if (plane == Plane::vertical) {
    moveAlg->setProperty(horizontalAxis, 0.0);
    moveAlg->setProperty(upAxis, perpendicularOffset);
  } else {
    moveAlg->setProperty(horizontalAxis, perpendicularOffset);
    moveAlg->setProperty(upAxis, 0.0);
  }
  moveAlg->execute();

  const bool rotateFace = getProperty("DetectorFacesSample");
  if (rotateFace) {
    auto rotate = createChildAlgorithm("RotateInstrumentComponent");
    rotate->setProperty("Workspace", outWS);
    if (!detectorName.empty())
      rotate->setProperty("ComponentName", detectorName);
    else
      rotate->setProperty("DetectorID", detectorID);
    if (plane == Plane::horizontal) {
      rotate->setProperty("X", upDir.X());
      rotate->setProperty("Y", upDir.Y());
      rotate->setProperty("Z", upDir.Z());
    } else {
      const V3D horizontalDir = -referenceFrame.vecPointingHorizontal();
      rotate->setProperty("X", horizontalDir.X());
      rotate->setProperty("Y", horizontalDir.Y());
      rotate->setProperty("Z", horizontalDir.Z());
    }
    rotate->setProperty("RelativeRotation", false);
    rotate->setProperty("Angle", twoThetaInRad * 180. / M_PI);
    rotate->execute();
  }
}

/**
 * Return the detector's position
 * @param inst an instrument
 * @param detectorName detector component's name
 * @param detectorID detector's id
 * @return a position
 */
Kernel::V3D SpecularReflectionPositionCorrect2::declareDetectorPosition(
    const Geometry::Instrument &inst, const std::string &detectorName,
    const detid_t detectorID) {
  // Detector
  IComponent_const_sptr detector;
  if (detectorName.empty()) {
    detector = inst.getDetector(detectorID);
  } else {
    detector = inst.getComponentByName(detectorName);
    if (!detector)
      throw Exception::NotFoundError("Detector component not found",
                                     detectorName);
  }
  return detector->getPos();
}

/**
 * Return the sample position
 * @param ws a workspace
 * @return a position
 */
V3D SpecularReflectionPositionCorrect2::declareSamplePosition(
    const MatrixWorkspace &ws) {
  V3D position;
  const std::string sampleName = getProperty("SampleComponentName");
  const auto inst = ws.getInstrument();
  IComponent_const_sptr sample = inst->getComponentByName(sampleName);
  if (sample)
    position = sample->getPos();
  else
    position = ws.spectrumInfo().samplePosition();
  return position;
}

/**
 * Return the user-given TwoTheta, augmented by LinePosition if needed
 * @param inWS the input workspace
 * @param l2 sample-to-detector distance
 * @return TwoTheta, in radians
 */
double SpecularReflectionPositionCorrect2::twoThetaFromProperties(
    const MatrixWorkspace &inWS, const double l2) {
  double twoThetaInRad =
      static_cast<double>(getProperty("TwoTheta")) * M_PI / 180.0;
  if (!isDefault("LinePosition")) {
    const double linePosition = getProperty("LinePosition");
    const double pixelSize = getProperty("PixelSize");
    const double offset =
        offsetAngleFromCentre(inWS, l2, linePosition, pixelSize);
    twoThetaInRad -= offset;
  }
  return twoThetaInRad;
}

/**
 * Return a direct beam calibrated TwoTheta
 * @param detectorName name of the detector component
 * @param detectorID detector's id
 * @param samplePosition sample position
 * @param l2 sample-to-detector distance
 * @param alongDir a unit vector pointing along the beam
 * @param beamOffset sample-to-detector distance on the beam axis
 * @return TwoTheta, in radians
 */
double SpecularReflectionPositionCorrect2::twoThetaFromDirectLine(
    const std::string &detectorName, const detid_t detectorID,
    const V3D &samplePosition, const double l2, const V3D &alongDir,
    const double beamOffset) {
  double twoThetaInRad;
  MatrixWorkspace_const_sptr directWS = getProperty("DirectLineWorkspace");
  const double directLinePosition = getProperty("DirectLinePosition");
  const double pixelSize = getProperty("PixelSize");
  const double directOffset =
      offsetAngleFromCentre(*directWS, l2, directLinePosition, pixelSize);
  const auto reflectedDetectorAngle = std::acos(beamOffset / l2);
  const auto directInst = directWS->getInstrument();
  const auto directDetPos =
      declareDetectorPosition(*directInst, detectorName, detectorID);
  const auto directSampleToDet = directDetPos - samplePosition;
  const double directBeamOffset = directSampleToDet.scalar_prod(alongDir);
  const double directL2 = directSampleToDet.norm();
  const auto directDetectorAngle = std::acos(directBeamOffset / directL2);
  twoThetaInRad = reflectedDetectorAngle - directDetectorAngle - directOffset;
  return twoThetaInRad;
}

} // namespace Algorithms
} // namespace Mantid
