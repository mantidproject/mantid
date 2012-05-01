/*WIKI* 
Calculate the EQSANS detector sensitivity. This workflow algorithm uses the
reduction parameters found in the property manager object passed as the
ReductionProperties parameter to load the given data file, apply all the
necessary corrections to it and compute the sensitivity correction.

Setting the PatchWorkspace property allows you to patch areas of the
detector. All masked pixels in the patch workspace will be patched.
The value assigned to a patched pixel is the average of all unmasked
pixels in this patched pixel's tube.
*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/ComputeSensitivity.h"
#include "MantidAPI/FileProperty.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidWorkflowAlgorithms/EQSANSInstrument.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"

namespace Mantid
{
namespace WorkflowAlgorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ComputeSensitivity)

/// Sets documentation strings for this algorithm
void ComputeSensitivity::initDocs()
{
  this->setWikiSummary("Workflow to calculate EQSANS sensitivity correction.");
  this->setOptionalMessage("Workflow to calculate EQSANS sensitivity correction.");
}

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void ComputeSensitivity::init()
{
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load, "_event.nxs"),
      "Flood field or sensitivity file.");
  declareProperty(new WorkspaceProperty<>("PatchWorkspace", "", Direction::Input, PropertyMode::Optional),
      "Workspace defining the area of the detector to be patched. All masked pixels in this workspace will be patched.");
  declareProperty("ReductionProperties", "__eqsans_reduction_properties", Direction::Input);
  declareProperty(new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "Workspace containing the sensitivity correction.");
  declareProperty("OutputMessage", "", Direction::Output);
}

void ComputeSensitivity::exec()
{
  std::string outputMessage = "";

  // Reduction property manager
  const std::string reductionManagerName = getProperty("ReductionProperties");
  boost::shared_ptr<PropertyManager> reductionManager;
  if (PropertyManagerDataService::Instance().doesExist(reductionManagerName))
  {
    reductionManager = PropertyManagerDataService::Instance().retrieve(reductionManagerName);
  }
  else
  {
    g_log.notice() << "Could not find property manager" << std::endl;
    reductionManager = boost::make_shared<PropertyManager>();
    PropertyManagerDataService::Instance().addOrReplace(reductionManagerName, reductionManager);
  }

  const std::string outputWS = getPropertyValue("OutputWorkspace");
  const std::string fileName = getPropertyValue("Filename");

  // Find beam center
  double center_x = EMPTY_DBL();
  double center_y = EMPTY_DBL();
  if (reductionManager->existsProperty("LatestBeamCenterX") &&
      reductionManager->existsProperty("LatestBeamCenterY"))
  {
    center_x = reductionManager->getProperty("LatestBeamCenterX");
    center_y = reductionManager->getProperty("LatestBeamCenterY");
    g_log.notice() << "No beam center provided: taking last position " << center_x << ", " << center_y << std::endl;
  }

  if (reductionManager->existsProperty("BeamCenterAlgorithm") &&
      reductionManager->existsProperty("BeamCenterFile"))
  {
    progress(0.1, "Starting beam finder");
    // Load direct beam file
    const std::string beamCenterFile = reductionManager->getProperty("BeamCenterFile");
    IAlgorithm_sptr loadAlg;
    if (reductionManager->existsProperty("LoadAlgorithm"))
    {
      loadAlg = reductionManager->getProperty("LoadAlgorithm");
      loadAlg->setChild(true);
      loadAlg->setProperty("Filename", beamCenterFile);
      loadAlg->execute();
      MatrixWorkspace_sptr beamCenterWS = loadAlg->getProperty("OutputWorkspace");
      const std::string outMsg = loadAlg->getPropertyValue("OutputMessage");
      outputMessage += outMsg;

      IAlgorithm_sptr centerAlg = reductionManager->getProperty("BeamCenterAlgorithm");
      centerAlg->setChild(true);
      centerAlg->setProperty("InputWorkspace", beamCenterWS);
      centerAlg->execute();
      std::vector<double> centerOfMass = centerAlg->getProperty("CenterOfMass");
      EQSANSInstrument::getPixelFromCoordinate(centerOfMass[0], centerOfMass[1], beamCenterWS, center_x, center_y);
    }
  }
  else if (isEmpty(center_x) || isEmpty(center_y))
  {
    g_log.notice() << "WARNING! No beam center information found!" << std::endl;
  }

  // Set patch information so that the SANS sensitivity algorithm can
  // patch the sensitivity workspace
  const std::string patchWSName = getPropertyValue("PatchWorkspace");
  if (patchWSName.size()>0)
  {
    progress(0.2, "Patch sensitivity parameters found");
    IAlgorithm_sptr patchAlg = createSubAlgorithm("EQSANSPatchSensitivity");
    patchAlg->setPropertyValue("PatchWorkspace", patchWSName);
    reductionManager->declareProperty(new AlgorithmProperty("SensitivityPatchAlgorithm"));
    reductionManager->setProperty("SensitivityPatchAlgorithm", patchAlg);
  }

  progress(0.3, "Computing sensitivity");
  if (reductionManager->existsProperty("SensitivityAlgorithm"))
  {
    IAlgorithm_sptr effAlg = reductionManager->getProperty("SensitivityAlgorithm");
    effAlg->setChild(true);
    effAlg->setProperty("Filename", fileName);
    effAlg->setProperty("BeamCenterX", center_x);
    effAlg->setProperty("BeamCenterY", center_y);
    effAlg->setPropertyValue("OutputSensitivityWorkspace", outputWS);
    effAlg->execute();
    MatrixWorkspace_sptr effWS = effAlg->getProperty("OutputSensitivityWorkspace");
    std::string outMsg2 = effAlg->getPropertyValue("OutputMessage");
    setProperty("OutputWorkspace", effWS);
    outputMessage += outMsg2;
    setProperty("OutputMessage", outputMessage);
  } else {
    g_log.error() << "Could not find sensitivity algorithm" << std::endl;
  }
}

} // namespace WorkflowAlgorithms
} // namespace Mantid

