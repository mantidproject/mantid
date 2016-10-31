#include "MantidAlgorithms/SpecularReflectionPositionCorrect2.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument/ComponentHelper.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/CompositeValidator.h"
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
  return "Correct detector positions vertically based on the specular "
    "reflection condition.";
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

  declareProperty(
		  make_unique<WorkspaceProperty<MatrixWorkspace>>("InputWorkspace",
								  "", Direction::Input),
                  "An input workspace to correct.");
  
  auto thetaValidator = boost::make_shared<CompositeValidator>();
  thetaValidator->add(boost::make_shared<MandatoryValidator<double>>());
  thetaValidator->add(
      boost::make_shared<BoundedValidator<double>>(0, 90, true));  declareProperty(
      make_unique<PropertyWithValue<double>>("TwoThetaIn", Mantid::EMPTY_DBL(),
                                             thetaValidator, Direction::Input),
      "Two theta angle in degrees.");

  declareProperty(
		  make_unique<PropertyWithValue<std::string>>(
							      "DetectorComponentName", 
							      "", 
							      boost::make_shared<MandatoryValidator<std::string>>(),
							      Direction::Input),
                  "Name of the detector component to correct, i.e. point-detector");
  
  declareProperty(make_unique<PropertyWithValue<std::string>>("SampleComponentName",
							      "some-surface-holder", Direction::Input),
                  "Name of the sample component, i.e. some-surface-holder");

  declareProperty(make_unique<PropertyWithValue<bool>>("StrictSpectrumChecking",
                                                       true, Direction::Input),
                  "When set to true, protects against non-sequential integers in which "
                  "spectrum numbers are not in {min, min+1, ..., max}, throwing and exception "
		  "and stopping execution.");

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
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

  const double twoThetaIn = getProperty("TwoThetaIn");
  const double twoThetaInRad = twoThetaIn * (M_PI / 180.0);

  auto inst = outWS->getInstrument();
  
  // Detector
  const std::string detectorName = getProperty("DetectorComponentName");
  IComponent_const_sptr detector = inst->getComponentByName(detectorName);
  const V3D detectorPosition = detector->getPos();

  // Sample
  const std::string sampleName = getProperty("SampleComponentName");
  IComponent_const_sptr sample = inst->getComponentByName(sampleName);
  const V3D samplePosition = sample->getPos();

  // Sample-to-detector
  const V3D sampleToDetector = detectorPosition - samplePosition;
  // Reference frame
  auto referenceFrame = inst->getReferenceFrame();
  auto beamAxis = referenceFrame->pointingAlongBeamAxis();
  auto horizontalAxis = referenceFrame->pointingHorizontalAxis();
  auto upAxis = referenceFrame->pointingUpAxis();

 // We just recalculate beam offset.
  double beamOffset = sampleToDetector.scalar_prod(
      referenceFrame
          ->vecPointingAlongBeam());
  // We only correct vertical position
  double upOffset =
    (beamOffset *
     std::tan(twoThetaInRad));

  auto moveAlg = createChildAlgorithm("MoveInstrumentComponent");
  moveAlg->initialize();
  moveAlg->setProperty("Workspace", outWS);
  moveAlg->setProperty("ComponentName", detectorName);
  moveAlg->setProperty("RelativePosition", false);
  moveAlg->setProperty(beamAxis, detectorPosition.scalar_prod(referenceFrame->vecPointingAlongBeam()));
  moveAlg->setProperty(horizontalAxis, 0.0);
  moveAlg->setProperty(upAxis, upOffset);
  moveAlg->execute();
  
  setProperty("OutputWorkspace", outWS);

}

} // namespace Algorithms
} // namespace Mantid
