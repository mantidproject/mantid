// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidReflectometry/SpecularReflectionPositionCorrect.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
#include "MantidKernel/MandatoryValidator.h"

#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

namespace Mantid::Reflectometry {
namespace {
const std::string pointDetectorAnalysis = "PointDetectorAnalysis";

/**
 * Get the component to move for a given component: its parent, unless that
 *is the instrument itself.
 * @param componentInfo : Component info holding the component
 * @param componentIndex : Some component in the tree
 * @return : Index of the component to move.
 */
size_t getParentComponent(ComponentInfo const &componentInfo, size_t const componentIndex) {
  if (componentInfo.hasParent(componentIndex)) {
    auto const parent = componentInfo.parent(componentIndex);
    if (parent != componentInfo.root()) {
      return parent;
    }
  }
  return componentIndex;
}

/**
 * Determine if there is a common parent component.
 * @param componentInfo : Component info holding the detectors
 * @param detectorIndices : Detectors to evaluate.
 * @return True only if all detectors have the same immediate parent.
 */
bool hasCommonParent(ComponentInfo const &componentInfo, std::vector<size_t> const &detectorIndices) {
  return std::all_of(detectorIndices.cbegin(), detectorIndices.cend(), [&](size_t const detectorIndex) {
    return componentInfo.hasParent(detectorIndex) &&
           componentInfo.parent(detectorIndex) == componentInfo.parent(detectorIndices.front());
  });
}
} // namespace
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SpecularReflectionPositionCorrect)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SpecularReflectionPositionCorrect::name() const { return "SpecularReflectionPositionCorrect"; }

/// Algorithm's version for identification. @see Algorithm::version
int SpecularReflectionPositionCorrect::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SpecularReflectionPositionCorrect::category() const { return "Reflectometry"; }

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SpecularReflectionPositionCorrect::init() {
  auto thetaValidator = std::make_shared<CompositeValidator>();
  thetaValidator->add(std::make_shared<MandatoryValidator<double>>());
  thetaValidator->add(std::make_shared<BoundedValidator<double>>(0, 90, true));
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "An input workspace to correct.");

  declareProperty(
      std::make_unique<PropertyWithValue<double>>("TwoThetaIn", Mantid::EMPTY_DBL(), thetaValidator, Direction::Input),
      "Input two theta angle in degrees.");

  this->initCommonProperties();

  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SpecularReflectionPositionCorrect::exec() {

  MatrixWorkspace_sptr inWS = this->getProperty("InputWorkspace");
  const std::string analysisMode = this->getProperty("AnalysisMode");
  auto cloneWS = this->createChildAlgorithm("CloneWorkspace");
  cloneWS->initialize();
  cloneWS->setProperty("InputWorkspace", inWS);
  cloneWS->execute();
  Workspace_sptr tmp = cloneWS->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr outWS = std::dynamic_pointer_cast<MatrixWorkspace>(tmp);

  const double twoThetaIn = this->getProperty("TwoThetaIn");

  auto const detectorIndices = this->getDetectorComponentIndices(*outWS, analysisMode == pointDetectorAnalysis);
  auto const sampleIndex = this->getSurfaceSampleComponentIndex(outWS->componentInfo());

  correctPosition(outWS, twoThetaIn, sampleIndex, detectorIndices);

  setProperty("OutputWorkspace", outWS);
}

/**
 * Execute the MoveInstrumentComponent on all (named) subcomponents
 * @param toCorrect : Workspace to correct
 * @param detectorIndices : Component indices of the detector or detectors
 * @param sampleIndex : Component index of the sample
 * @param upOffset : Up offset to apply
 * @param acrossOffset : Across offset to apply
 * @param detectorPosition: Actual detector or detector group position.
 */
void SpecularReflectionPositionCorrect::moveDetectors(API::MatrixWorkspace_sptr const &toCorrect,
                                                      std::vector<size_t> const &detectorIndices,
                                                      size_t const sampleIndex, double const upOffset,
                                                      double const acrossOffset, V3D const &detectorPosition) {
  auto const &componentInfo = toCorrect->componentInfo();
  auto const samplePosition = componentInfo.position(sampleIndex);
  auto const referenceFrame = componentInfo.referenceFrame();
  // With several detectors sharing a parent, moving that parent is enough.
  // Otherwise we have to move the individual components.
  auto const toMove = detectorIndices.size() > 1 && hasCommonParent(componentInfo, detectorIndices)
                          ? std::vector<size_t>{detectorIndices.front()}
                          : detectorIndices;
  for (auto const detectorIndex : toMove) {
    // Re-fetch, as each move updates the workspace's component positions
    auto const &currentComponentInfo = toCorrect->componentInfo();
    auto moveComponentAlg = this->createChildAlgorithm("MoveInstrumentComponent");
    moveComponentAlg->initialize();
    moveComponentAlg->setProperty("Workspace", toCorrect);
    auto const root = getParentComponent(currentComponentInfo, detectorIndex);
    auto const &componentName = currentComponentInfo.name(root);
    moveComponentAlg->setProperty("ComponentName", componentName);
    moveComponentAlg->setProperty("RelativePosition", false);
    // Movements
    moveComponentAlg->setProperty(referenceFrame->pointingAlongBeamAxis(),
                                  detectorPosition.scalar_prod(referenceFrame->vecPointingAlongBeam()));
    moveComponentAlg->setProperty(referenceFrame->pointingHorizontalAxis(), acrossOffset);
    double const detectorVerticalPosition = detectorPosition.scalar_prod(referenceFrame->vecPointingUp());
    double const rootVerticalPosition =
        currentComponentInfo.position(root).scalar_prod(referenceFrame->vecPointingUp());

    double const dm = rootVerticalPosition - detectorVerticalPosition;
    moveComponentAlg->setProperty(referenceFrame->pointingUpAxis(),
                                  samplePosition.scalar_prod(referenceFrame->vecPointingUp()) + upOffset + dm);
    // Execute the movement.
    moveComponentAlg->execute();
  }
}

/**
 * Correct the position of the detectors based on the input theta value.
 * @param toCorrect : Workspace to correct detector posisitions on.
 * @param twoThetaInDeg : 2* Theta in degrees to use in correction calculations.
 * @param sampleIndex : Component index of the sample
 * @param detectorIndices : Component indices of the detector or detectors
 */
void SpecularReflectionPositionCorrect::correctPosition(API::MatrixWorkspace_sptr const &toCorrect,
                                                        double const twoThetaInDeg, size_t const sampleIndex,
                                                        std::vector<size_t> const &detectorIndices) {
  auto const &componentInfo = toCorrect->componentInfo();

  V3D const detectorPosition = meanPosition(componentInfo, detectorIndices);

  V3D const samplePosition = componentInfo.position(sampleIndex);

  V3D const sampleToDetector = detectorPosition - samplePosition;

  auto const referenceFrame = componentInfo.referenceFrame();

  double const twoThetaInRad = twoThetaInDeg * (M_PI / 180.0);

  double const acrossOffset = 0;

  double const beamOffset =
      sampleToDetector.scalar_prod(referenceFrame->vecPointingAlongBeam()); // We just recalculate beam offset.

  double const upOffset = (beamOffset * std::tan(0.5 * twoThetaInRad)); // We only correct vertical position

  // Apply the movements.
  moveDetectors(toCorrect, detectorIndices, sampleIndex, upOffset, acrossOffset, detectorPosition);
}

} // namespace Mantid::Reflectometry
