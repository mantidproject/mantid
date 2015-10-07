#include "MantidVatesAPI/BoxInfo.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidAPI/IMDEventWorkspace.h"

using namespace Mantid::API;

namespace Mantid {
namespace VATES {
boost::optional<int> findRecursionDepthForTopLevelSplitting(const std::string &workspaceName) {
  const int topLevelRecursionDepth = 1;
  boost::optional<int> recursionDepth;
  Mantid::VATES::ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace>
      workspaceProvider;
  if (workspaceProvider.canProvideWorkspace(workspaceName)) {
    auto workspace =
          boost::dynamic_pointer_cast<Mantid::API::IMDEventWorkspace>(
              workspaceProvider.fetchWorkspace(workspaceName));
    auto boxController = workspace->getBoxController();
    boost::optional<std::vector<size_t>> topLevelSplits =
        boxController->getSplitTopInto();
    if (boxController->getSplitTopInto()) {
      recursionDepth = topLevelRecursionDepth;
    }
  }
  return recursionDepth;
}
}
}
