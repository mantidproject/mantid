// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesAPI/BoxInfo.h"
#include "MantidAPI/IMDEventWorkspace.h"


using namespace Mantid::API;

namespace Mantid {
namespace VATES {

boost::optional<int> findRecursionDepthForTopLevelSplitting(
    const std::string &workspaceName,
    const WorkspaceProvider &workspaceProvider) {
  const int topLevelRecursionDepth = 1;
  boost::optional<int> recursionDepth;
  if (workspaceProvider.canProvideWorkspace(workspaceName)) {
    auto workspace =
        boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(
            workspaceProvider.fetchWorkspace(workspaceName));
    auto boxController = workspace->getBoxController();
    if (boxController->getSplitTopInto()) {
      recursionDepth = topLevelRecursionDepth;
    }
  }
  return recursionDepth;
}
} // namespace VATES
} // namespace Mantid
