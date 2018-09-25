#ifndef MANTID_CUSTOMINTERFACES_WORKSPACENAMESFACTORY_H_
#define MANTID_CUSTOMINTERFACES_WORKSPACENAMESFACTORY_H_
#include "ReductionWorkspaces.h"
namespace MantidQt {
namespace CustomInterfaces {
namespace internal {

class WorkspaceNamesFactoryDetail {
public:
  static ReductionWorkspaces
  makeNames(std::vector<std::string> const &runNumbers,
            std::pair<std::string, std::string> const &transmissionRuns) {
    return workspaceNames(runNumbers, transmissionRuns);
  }

  static std::string makePostprocessedName(
      std::vector<std::vector<std::string> const *> const &runNumbers) {
    return postprocessedWorkspaceName(runNumbers);
  }
};
} // namespace internal

class MANTIDQT_ISISREFLECTOMETRY_DLL WorkspaceNamesFactory {
public:
  WorkspaceNamesFactory() {}

  ReductionWorkspaces
  makeNames(std::vector<std::string> const &runNumbers,
            std::pair<std::string, std::string> const &transmissionRuns) const {
    return internal::WorkspaceNamesFactoryDetail::makeNames(runNumbers,
                                                            transmissionRuns);
  }

  std::string makePostprocessedName(
      std::vector<std::vector<std::string> const *> const &runNumbers) const {
    return internal::WorkspaceNamesFactoryDetail::makePostprocessedName(
        runNumbers);
  }
};
} // namespace CustomInterfaces
} // namespace MantidQt
#endif // MANTID_CUSTOMINTERFACES_WORKSPACENAMESFACTORY_H_
