#include "MantidDataHandling/LoadNexusMonitors.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"

using Mantid::API::Workspace_sptr;
using Mantid::API::WorkspaceGroup;

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(LoadNexusMonitors)

LoadNexusMonitors::LoadNexusMonitors() : Algorithm() {}

LoadNexusMonitors::~LoadNexusMonitors() {}

/// Initialization method.
void LoadNexusMonitors::init() {
  declareProperty(
      new API::FileProperty("Filename", "", API::FileProperty::Load, ".nxs"),
      "The name (including its full or relative path) of the NeXus file to "
      "attempt to load. The file extension must either be .nxs or .NXS");

  declareProperty(
      new API::WorkspaceProperty<API::Workspace>("OutputWorkspace", "",
                                                 Kernel::Direction::Output),
      "The name of the output workspace in which to load the NeXus monitors.");

  declareProperty(new Kernel::PropertyWithValue<bool>("MonitorsAsEvents", true,
                                                      Kernel::Direction::Input),
                  "If enabled (by default), load the monitors as events (into "
                  "an EventWorkspace), as long as there is event data. If "
                  "disabled, load monitors as spectra (into a Workspace2D, "
                  "regardless of whether event data is found.");

  declareProperty(
      new Kernel::PropertyWithValue<bool>("LoadCompleteWorkspaceOnMasterRank",
                                          false, Kernel::Direction::Input),
      "In a run with MPI, loads all data on master rank and none on other "
      "ranks.");
}

/**
 * Executes the algorithm. Reading in the file and creating and populating
 * the output workspace
 */
void LoadNexusMonitors::exec() {
  // Use version 2 of this algorithm
  auto alg = createChildAlgorithm("LoadNexusMonitors", -1, -1, true, 2);
  alg->setRethrows(true);

  // Forward algorithm properties
  alg->setPropertyValue("Filename", getPropertyValue("Filename"));
  alg->setPropertyValue("OutputWorkspace", getPropertyValue("OutputWorkspace"));
  alg->setPropertyValue("MonitorsAsEvents",
                        getPropertyValue("MonitorsAsEvents"));

  // Run new algorithm
  alg->execute();

  // Output workspace of new algorithm
  Workspace_sptr ws = alg->getProperty("OutputWorkspace");

  // If it's a group workspace, we need to create additional output workspaces
  // to match this version's previous behaviour. This doesn't create additional
  // workspaces in the ADS, but it does have side-effects:
  // - Rename all the children of the group to _1, _2, _3, etc
  // - Cause the return value in Python to be a tuple that contains the group
  //   workspace, followed by references to its children as siblings
  auto ws_group = boost::dynamic_pointer_cast<WorkspaceGroup>(ws);
  if (ws_group) {
    auto child_count = ws_group->size();
    for (decltype(child_count) i = 0; i < child_count; ++i) {
      // create additional output workspace property
      std::stringstream ssWsName;
      ssWsName << "_" << i + 1;
      std::stringstream ssPropName;
      ssPropName << "OutputWorkspace"
                 << "_" << i + 1;
      declareProperty(
          new API::WorkspaceProperty<API::Workspace>(
              ssPropName.str(), ssWsName.str(), Kernel::Direction::Output),
          "Additional output workspace for multi period monitors.");
      setProperty(ssPropName.str(), ws_group->getItem(i));
    }
  }

  setProperty("OutputWorkspace", ws);
}

MPI::ExecutionMode LoadNexusMonitors::getParallelExecutionMode(
    const std::map<std::string, MPI::StorageMode> &storageModes) const {
  // We have no input workspace, so we do not use the map.
  UNUSED_ARG(storageModes)
  if (getProperty("LoadCompleteWorkspaceOnMasterRank"))
    return MPI::ExecutionMode::MasterOnly;
  else
    return MPI::ExecutionMode::Distributed;
}

MPI::StorageMode LoadNexusMonitors::getStorageModeForOutputWorkspace(
    const std::string &propertyName) const {
  // We have only one output workspace, so we ignore propertyName.
  UNUSED_ARG(propertyName)
  if (getProperty("LoadCompleteWorkspaceOnMasterRank"))
    return MPI::StorageMode::MasterOnly;
  else
    return MPI::StorageMode::Distributed;
}

} // end DataHandling
} // end Mantid
