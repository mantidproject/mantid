// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadNexusMonitors.h"

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"

using Mantid::API::WorkspaceGroup;
using Mantid::API::Workspace_sptr;

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(LoadNexusMonitors)

/// Initialization method.
void LoadNexusMonitors::init() {
  declareProperty(
      std::make_unique<API::FileProperty>("Filename", "",
                                          API::FileProperty::Load, ".nxs"),
      "The name (including its full or relative path) of the NeXus file to "
      "attempt to load. The file extension must either be .nxs or .NXS");

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::Workspace>>(
          "OutputWorkspace", "", Kernel::Direction::Output),
      "The name of the output workspace in which to load the NeXus monitors.");
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
          std::make_unique<API::WorkspaceProperty<API::Workspace>>(
              ssPropName.str(), ssWsName.str(), Kernel::Direction::Output),
          "Additional output workspace for multi period monitors.");
      setProperty(ssPropName.str(), ws_group->getItem(i));
    }
  }

  setProperty("OutputWorkspace", ws);
}

} // namespace DataHandling
} // namespace Mantid
