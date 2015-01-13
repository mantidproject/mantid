#include "MantidMDEvents/OneStepMDEW.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
//#include "MantidNexus/LoadEventNexus.h"
//#include "MantidMDEvents/ConvertToDiffractionMDWorkspace.h"

namespace Mantid {
namespace MDEvents {

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
/** Destructor
*/
OneStepMDEW::~OneStepMDEW() {}

//----------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
*/
void OneStepMDEW::init() {
  this->declareProperty(
      new FileProperty("Filename", "", FileProperty::Load, ".nxs"),
      "The name (including its full or relative path) of the Nexus file to\n"
      "attempt to load. The file extension must either be .nxs or .NXS");

  this->declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                                         Direction::Output),
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
  IEventWorkspace_sptr tempWS = loadAlg->getProperty("OutputWorkspace");

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

} // namespace Mantid
} // namespace MDEvents
