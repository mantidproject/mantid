#include "MantidVatesAPI/BoxInfo.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidKernel/make_unique.h"

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
    boost::optional<std::vector<size_t>> topLevelSplits =
        boxController->getSplitTopInto();
    if (boxController->getSplitTopInto()) {
      recursionDepth = topLevelRecursionDepth;
    }
  }
  return recursionDepth;
}
} // namespace VATES
} // namespace Mantid
