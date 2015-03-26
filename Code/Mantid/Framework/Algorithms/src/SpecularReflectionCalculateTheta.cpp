#include "MantidAlgorithms/SpecularReflectionCalculateTheta.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include <cmath>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

namespace {
const std::string multiDetectorAnalysis = "MultiDetectorAnalysis";
const std::string lineDetectorAnalysis = "LineDetectorAnalysis";
const std::string pointDetectorAnalysis = "PointDetectorAnalysis";
}

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SpecularReflectionCalculateTheta)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SpecularReflectionCalculateTheta::SpecularReflectionCalculateTheta() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SpecularReflectionCalculateTheta::~SpecularReflectionCalculateTheta() {}

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string SpecularReflectionCalculateTheta::name() const {
  return "SpecularReflectionCalculateTheta";
}

/// Algorithm's version for identification. @see Algorithm::version
int SpecularReflectionCalculateTheta::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SpecularReflectionCalculateTheta::category() const {
  return "Reflectometry";
}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SpecularReflectionCalculateTheta::init() {
  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "",
                                             Direction::Input),
      "An Input workspace to calculate the specular relection theta on.");
  this->initCommonProperties();
  declareProperty(new PropertyWithValue<double>("TwoTheta", Mantid::EMPTY_DBL(),
                                                Direction::Output),
                  "Calculated two theta scattering angle in degrees.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SpecularReflectionCalculateTheta::exec() {
  MatrixWorkspace_sptr inWS = this->getProperty("InputWorkspace");

  const std::string analysisMode = this->getProperty("AnalysisMode");

  Instrument_const_sptr instrument = inWS->getInstrument();

  IComponent_const_sptr detector =
      this->getDetectorComponent(inWS, analysisMode == pointDetectorAnalysis);

  IComponent_const_sptr sample = this->getSurfaceSampleComponent(instrument);

  const V3D detSample = detector->getPos() - sample->getPos();

  boost::shared_ptr<const ReferenceFrame> refFrame =
      instrument->getReferenceFrame();

  const double upoffset = refFrame->vecPointingUp().scalar_prod(detSample);
  const double beamoffset =
      refFrame->vecPointingAlongBeam().scalar_prod(detSample);

  const double twoTheta = std::atan(upoffset / beamoffset) * 180 / M_PI;

  std::stringstream strstream;
  strstream << "Recalculated two theta as: " << twoTheta;

  this->g_log.information(strstream.str());

  this->setProperty("TwoTheta", twoTheta);
}

} // namespace Algorithms
} // namespace Mantid
