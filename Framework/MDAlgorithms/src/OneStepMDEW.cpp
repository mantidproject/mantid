// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMDAlgorithms/OneStepMDEW.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
//#include "MantidNexus/LoadEventNexus.h"
//#include "MantidDataObjects/ConvertToDiffractionMDWorkspace.h"

namespace Mantid {
namespace MDAlgorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(OneStepMDEW)

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

//----------------------------------------------------------------------------------------------
/** Constructor
 */
OneStepMDEW::OneStepMDEW() {
  this->useAlgorithm("ConvertToDiffractionMDWorkspace");
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void OneStepMDEW::init() {
  this->declareProperty(
      std::make_unique<FileProperty>("Filename", "", FileProperty::Load, ".nxs"),
      "The name (including its full or relative path) of the Nexus file to\n"
      "attempt to load. The file extension must either be .nxs or .NXS");

  this->declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                            "OutputWorkspace", "", Direction::Output),
                        "Name of the output MDEventWorkspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void OneStepMDEW::exec() {
  std::string tempWsName = getPropertyValue("OutputWorkspace") + "_nxs";

  // -------- First we load the event nexus file -------------
  Algorithm_sptr loadAlg = createChildAlgorithm("LoadEventNexus", 0, 2);
  loadAlg->initialize();
  loadAlg->setPropertyValue("Filename", getPropertyValue("Filename"));
  loadAlg->setPropertyValue("OutputWorkspace", tempWsName);
  loadAlg->executeAsChildAlg();
  Workspace_sptr temp = loadAlg->getProperty("OutputWorkspace");
  IEventWorkspace_sptr tempWS =
      boost::dynamic_pointer_cast<IEventWorkspace>(temp);

  // --------- Now Convert -------------------------------

  Algorithm_sptr childAlg =
      createChildAlgorithm("ConvertToDiffractionMDWorkspace", 2, 4, true, 1);
  childAlg->setProperty("InputWorkspace", tempWS);
  childAlg->setProperty<bool>("ClearInputWorkspace", false);
  childAlg->setProperty<bool>("LorentzCorrection", true);
  childAlg->executeAsChildAlg();

  IMDEventWorkspace_sptr outWS = childAlg->getProperty("OutputWorkspace");
  setProperty<Workspace_sptr>("OutputWorkspace", outWS);
}

} // namespace MDAlgorithms
} // namespace Mantid
