// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/RenameWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidKernel/Exception.h"

namespace Mantid::Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(RenameWorkspace)

using namespace Kernel;
using namespace API;

/** Initialisation method.
 *
 */
void RenameWorkspace::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("InputWorkspace", "", Direction::Input));
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("OutputWorkspace", "", Direction::Output));
  declareProperty<bool>("RenameMonitors", false,
                        "If true, and monitor workspace found attached"
                        " to the source workspace, the monitors workspace is renamed too.\n"
                        "The monitor workspace name is created from the new workspace "
                        "name: NewWSName by adding the _monitors suffix"
                        " (e.g.: NewWSName_monitors)",
                        Direction::Input);
  // Set to default true to maintain compatibility with existing scripts
  // as this just allowed overriding by default
  declareProperty<bool>("OverwriteExisting", true,
                        "If true any existing workspaces with the output name will be"
                        " overwritten. Defaults to true to maintain backwards compatibility.",
                        Direction::Input);
}

/**
 * Tests that the inputs are all valid
 * @return A map containing the incorrect workspace
 * properties and an error message
 */
std::map<std::string, std::string> RenameWorkspace::validateInputs() {
  using namespace std;
  map<string, string> errorList;

  // get the output workspace name
  std::string outputwsName = getPropertyValue("OutputWorkspace");

  // First check input and output names are different
  if (getPropertyValue("InputWorkspace") == getPropertyValue("OutputWorkspace")) {
    errorList["InputWorkspace"] = "Input and output workspace"
                                  " names must be different";
    errorList["OutputWorkspace"] = "Input and output workspace"
                                   " names must be different";
  }

  // Test to see if the output already exists
  if (AnalysisDataService::Instance().doesExist(outputwsName)) {
    // check if we are overriding existing workspaces
    bool overrideWorkspaces = getProperty("OverwriteExisting");
    // Output name already exists - either remove or error
    if (!overrideWorkspaces) {
      // If we try to delete the workspace here a subtle bug is introduced
      // Where the workspace group handle is deleted if we are renaming
      // Its last workspace member, then when we add (or rename) that member
      // undefined behavior happens usually Python Unit tests breaking
      errorList["OutputWorkspace"] = "The workspace " + outputwsName + " already exists";
      errorList["OverwriteExisting"] = "Set OverwriteExisting to true"
                                       " to overwrite the existing workspace";
    }
  }

  return errorList;
}

/** Executes the algorithm
 *
 *  @throw runtime_error Thrown if algorithm cannot execute
 */
void RenameWorkspace::exec() {
  auto &ADS = AnalysisDataService::Instance();

  // Get the input workspace
  Workspace_sptr inputWS = getProperty("InputWorkspace");

  WorkspaceGroup_sptr inputGroup = std::dynamic_pointer_cast<WorkspaceGroup>(inputWS);

  // get the workspace name
  std::string inputwsName = inputWS->getName();
  // get the output workspace name
  std::string outputwsName = getPropertyValue("OutputWorkspace");

  // Assign it to the output workspace property
  setProperty("OutputWorkspace", inputWS);

  // rename the input workspace using the rename method
  ADS.rename(inputwsName, outputwsName);

  const bool renameMonitors = getProperty("RenameMonitors");
  if (!renameMonitors)
    return;

  // Deal with attached monitor workspace if any.
  auto matInputWS = std::dynamic_pointer_cast<MatrixWorkspace>(inputWS);
  if (!matInputWS) // its some kind workspaces which may not have possibility
    return;        // to attach monitors to it
  auto monWS = matInputWS->monitorWorkspace();
  if (monWS) {
    std::string monWSName = monWS->getName();
    // rename the monitor workspace accordingly
    if (monWSName.empty()) {
      // workspace will always have name after added to ADS, so apparently not
      // the case
      ADS.add(outputwsName + "_monitors", monWS);
    } else {
      try {
        ADS.rename(monWSName, outputwsName + "_monitors");
      } catch (Kernel::Exception::NotFoundError &) { // it may be deleted
        ADS.add(monWSName, monWS);
        ADS.rename(monWSName, outputwsName + "_monitors");
      }
    }
  }
}

bool RenameWorkspace::processGroups() {
  // Get the input & output workspace names
  Workspace_sptr inputWS = getProperty("InputWorkspace");
  const std::string inputwsName = inputWS->getName();
  std::string outputwsName = getPropertyValue("OutputWorkspace");

  if (inputwsName == outputwsName) {
    throw std::invalid_argument("The input and output workspace names must be different");
  }

  // Cast the input to a group
  WorkspaceGroup_sptr inputGroup = std::dynamic_pointer_cast<WorkspaceGroup>(inputWS);
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

        auto alg = API::AlgorithmManager::Instance().createUnmanaged(this->name(), this->version());
        alg->initialize();
        alg->setPropertyValue("InputWorkspace", names[i]);
        alg->setPropertyValue("OutputWorkspace", wsName);
        alg->setChild(true);
        alg->enableHistoryRecordingForChild(false);
        alg->execute();
      } catch (Kernel::Exception::NotFoundError &ex) {
        // Will wind up here if group has somehow got messed up and a member
        // doesn't exist. Should't be possible!
        g_log.error() << ex.what() << '\n';
      }
    }
  }

  setProperty("OutputWorkspace", inputGroup);

  // We finished successfully.
  g_log.notice() << name() << " successful\n";

  return true;
}

} // namespace Mantid::Algorithms
