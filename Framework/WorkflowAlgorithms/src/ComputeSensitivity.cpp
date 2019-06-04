// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidWorkflowAlgorithms/ComputeSensitivity.h"
#include "MantidAPI/AlgorithmProperty.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidWorkflowAlgorithms/EQSANSInstrument.h"

namespace Mantid {
namespace WorkflowAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ComputeSensitivity)

using namespace Kernel;
using namespace API;
using namespace DataObjects;

void ComputeSensitivity::init() {
  declareProperty(std::make_unique<API::FileProperty>(
                      "Filename", "", API::FileProperty::Load, "_event.nxs"),
                  "Flood field or sensitivity file.");
  declareProperty(std::make_unique<WorkspaceProperty<>>("PatchWorkspace", "",
                                                   Direction::Input,
                                                   PropertyMode::Optional),
                  "Workspace defining the area of the detector to be patched. "
                  "All masked pixels in this workspace will be patched.");
  declareProperty("ReductionProperties", "__eqsans_reduction_properties",
                  Direction::Input);
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "Workspace containing the sensitivity correction.");
  declareProperty("OutputMessage", "", Direction::Output);
}

void ComputeSensitivity::exec() {
  std::string outputMessage;
  progress(0.1, "Setting up sensitivity calculation");

  // Reduction property manager
  boost::shared_ptr<PropertyManager> reductionManager = getProcessProperties();

  const std::string outputWS = getPropertyValue("OutputWorkspace");

  // Find beam center
  if (reductionManager->existsProperty("SANSBeamFinderAlgorithm")) {
    // const std::string algTxt =
    // reductionManager->getPropertyValue("SANSBeamFinderAlgorithm");

    IAlgorithm_sptr ctrAlg =
        reductionManager->getProperty("SANSBeamFinderAlgorithm");
    ctrAlg->setPropertyValue("ReductionProperties",
                             getPropertyValue("ReductionProperties"));
    ctrAlg->setChild(true);
    ctrAlg->execute();
    std::string outMsg2 = ctrAlg->getPropertyValue("OutputMessage");
    outputMessage += outMsg2;
  }

  progress(0.2, "Computing sensitivity");

  // Set patch information so that the SANS sensitivity algorithm can
  // patch the sensitivity workspace
  const std::string patchWSName = getPropertyValue("PatchWorkspace");
  if (!patchWSName.empty()) {
    IAlgorithm_sptr patchAlg = createChildAlgorithm("EQSANSPatchSensitivity");
    patchAlg->setPropertyValue("PatchWorkspace", patchWSName);
    if (!reductionManager->existsProperty("SensitivityPatchAlgorithm")) {
      reductionManager->declareProperty(
          std::make_unique<AlgorithmProperty>("SensitivityPatchAlgorithm"));
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
    g_log.error() << "Could not find sensitivity algorithm\n";
  }
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
