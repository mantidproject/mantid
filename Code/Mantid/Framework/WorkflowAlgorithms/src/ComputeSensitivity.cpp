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

namespace Mantid {
namespace WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ComputeSensitivity)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

void ComputeSensitivity::init() {
  declareProperty(new API::FileProperty("Filename", "", API::FileProperty::Load,
                                        "_event.nxs"),
                  "Flood field or sensitivity file.");
  declareProperty(new WorkspaceProperty<>("PatchWorkspace", "",
                                          Direction::Input,
                                          PropertyMode::Optional),
                  "Workspace defining the area of the detector to be patched. "
                  "All masked pixels in this workspace will be patched.");
  declareProperty("ReductionProperties", "__eqsans_reduction_properties",
                  Direction::Input);
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "Workspace containing the sensitivity correction.");
  declareProperty("OutputMessage", "", Direction::Output);
}

void ComputeSensitivity::exec() {
  std::string outputMessage = "";
  progress(0.1, "Setting up sensitivity calculation");

  // Reduction property manager
  const std::string reductionManagerName = getProperty("ReductionProperties");
  boost::shared_ptr<PropertyManager> reductionManager =
      getProcessProperties(reductionManagerName);

  const std::string outputWS = getPropertyValue("OutputWorkspace");

  // Find beam center
  if (reductionManager->existsProperty("SANSBeamFinderAlgorithm")) {
    // const std::string algTxt =
    // reductionManager->getPropertyValue("SANSBeamFinderAlgorithm");

    IAlgorithm_sptr ctrAlg =
        reductionManager->getProperty("SANSBeamFinderAlgorithm");
    ctrAlg->setPropertyValue("ReductionProperties", reductionManagerName);
    ctrAlg->setChild(true);
    ctrAlg->execute();
    std::string outMsg2 = ctrAlg->getPropertyValue("OutputMessage");
    outputMessage += outMsg2;
  }

  progress(0.2, "Computing sensitivity");

  // Set patch information so that the SANS sensitivity algorithm can
  // patch the sensitivity workspace
  const std::string patchWSName = getPropertyValue("PatchWorkspace");
  if (patchWSName.size() > 0) {
    IAlgorithm_sptr patchAlg = createChildAlgorithm("EQSANSPatchSensitivity");
    patchAlg->setPropertyValue("PatchWorkspace", patchWSName);
    if (!reductionManager->existsProperty("SensitivityPatchAlgorithm")) {
      reductionManager->declareProperty(
          new AlgorithmProperty("SensitivityPatchAlgorithm"));
    }
    reductionManager->setProperty("SensitivityPatchAlgorithm", patchAlg);
  }

  if (reductionManager->existsProperty("SensitivityAlgorithm")) {
    const std::string fileName = getPropertyValue("Filename");
    IAlgorithm_sptr effAlg =
        reductionManager->getProperty("SensitivityAlgorithm");
    effAlg->setChild(true);
    effAlg->setProperty("Filename", fileName);
    effAlg->setPropertyValue("OutputSensitivityWorkspace", outputWS);
    effAlg->execute();
    MatrixWorkspace_sptr effWS =
        effAlg->getProperty("OutputSensitivityWorkspace");
    setProperty("OutputWorkspace", effWS);
    std::string outMsg2 = effAlg->getPropertyValue("OutputMessage");
    outputMessage += outMsg2;
    setProperty("OutputMessage", outputMessage);
  } else {
    g_log.error() << "Could not find sensitivity algorithm" << std::endl;
  }
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
