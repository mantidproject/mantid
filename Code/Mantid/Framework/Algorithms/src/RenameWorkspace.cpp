//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/RenameWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/Exception.h"

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(RenameWorkspace)

using namespace Kernel;
using namespace API;

/** Initialisation method.
 *
 */
void RenameWorkspace::init() {
  declareProperty(
      new WorkspaceProperty<Workspace>("InputWorkspace", "", Direction::Input));
  declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace", "",
                                                   Direction::Output));
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void RenameWorkspace::exec() {
  // Get the input workspace
  Workspace_sptr inputWS = getProperty("InputWorkspace");
  // get the workspace name
  std::string inputwsName = inputWS->getName();
  // get the output workspace name
  std::string outputwsName = getPropertyValue("OutputWorkspace");

  if (getPropertyValue("InputWorkspace") ==
      getPropertyValue("OutputWorkspace")) {
    throw std::invalid_argument(
        "The input and output workspace names must be different");
  }

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", inputWS);

  // rename the input workspace using the rename method
  AnalysisDataService::Instance().rename(inputwsName, outputwsName);
}

bool RenameWorkspace::processGroups() {
  // Get the input & output workspace names
  Workspace_sptr inputWS = getProperty("InputWorkspace");
  const std::string inputwsName = inputWS->name();
  std::string outputwsName = getPropertyValue("OutputWorkspace");

  if (inputwsName == outputwsName) {
    throw std::invalid_argument(
        "The input and output workspace names must be different");
  }

  // Cast the input to a group
  WorkspaceGroup_sptr inputGroup =
      boost::dynamic_pointer_cast<WorkspaceGroup>(inputWS);
  assert(inputGroup); // Should always be true

  // Decide whether we will rename the group members. Must do this before
  // renaming group itself.
  // Basically we rename if the members ALL follow the pattern GroupName_1, _2,
  // _3 etc.
  const bool renameMembers = inputGroup->areNamesSimilar();

  AnalysisDataService::Instance().rename(inputwsName, outputwsName);

  // If necessary, go through group members calling the algorithm on each one
  if (renameMembers) {
    const std::vector<std::string> names = inputGroup->getNames();

    // loop over input ws group members
    for (size_t i = 0; i < names.size(); ++i) {
      try {
        // new name of the member workspaces
        std::stringstream suffix;
        suffix << i + 1;
        std::string wsName = outputwsName + "_" + suffix.str();

        IAlgorithm *alg = API::FrameworkManager::Instance().createAlgorithm(
            this->name(), this->version());
        alg->setPropertyValue("InputWorkspace", names[i]);
        alg->setPropertyValue("OutputWorkspace", wsName);
        alg->execute();
      } catch (Kernel::Exception::NotFoundError &ex) {
        // Will wind up here if group has somehow got messed up and a member
        // doesn't exist. Should't be possible!
        g_log.error() << ex.what() << std::endl;
      }
    }
  }
  setProperty("OutputWorkspace", inputWS);

  // We finished successfully.
  setExecuted(true);
  notificationCenter().postNotification(
      new FinishedNotification(this, isExecuted()));
  g_log.notice() << name() << " successful\n";

  return true;
}

} // namespace Algorithms
} // namespace Mantid
