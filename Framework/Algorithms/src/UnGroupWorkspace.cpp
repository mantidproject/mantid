// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/UnGroupWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(UnGroupWorkspace)

using namespace Kernel;
using namespace API;

/// Initialisation method
void UnGroupWorkspace::init() {
  const AnalysisDataServiceImpl &data_store = AnalysisDataService::Instance();
  // Get the list of workspaces in the ADS
  auto workspaceList = data_store.getObjectNames();
  std::unordered_set<std::string> groupWorkspaceList;
  // Not iterate over, removing all those which are not group workspaces
  for (const auto &name : workspaceList) {
    WorkspaceGroup_const_sptr group =
        boost::dynamic_pointer_cast<const WorkspaceGroup>(
            data_store.retrieve(name));
    // RNT: VC returns bad pointer after erase
    // if ( !group ) workspaceList.erase(it);
    if (group) {
      groupWorkspaceList.insert(name);
    }
  }
  // Declare a text property with the list of group workspaces as its allowed
  // values
  declareProperty("InputWorkspace", "",
                  "Name of the input workspace to ungroup",
                  boost::make_shared<StringListValidator>(groupWorkspaceList));
}

/** Executes the algorithm
 *  @throw std::runtime_error If the selected workspace is not a group workspace
 */
void UnGroupWorkspace::exec() {
  const std::string inputws = getProperty("InputWorkspace");
  AnalysisDataServiceImpl &data_store =
      Mantid::API::AnalysisDataService::Instance();

  // Retrieve the input workspace
  Workspace_sptr wsSptr = data_store.retrieve(inputws);
  // Try to cast it to a WorkspaceGroup
  WorkspaceGroup_sptr wsGrpSptr =
      boost::dynamic_pointer_cast<WorkspaceGroup>(wsSptr);
  // Test the cast succeeded - it always should because of ListValidator on
  // input property
  if (!wsGrpSptr) {
    throw std::runtime_error("Selected Workspace is not a WorkspaceGroup");
  }

  // Notify observers that a WorkspaceGroup is about to be unrolled
  data_store.notificationCenter.postNotification(
      new Mantid::API::WorkspaceUnGroupingNotification(inputws, wsSptr));
  // Now remove the WorkspaceGroup from the ADS
  data_store.remove(inputws);
}
} // namespace Algorithms
} // namespace Mantid
