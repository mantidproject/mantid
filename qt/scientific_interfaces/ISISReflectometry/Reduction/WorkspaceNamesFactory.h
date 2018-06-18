#ifndef MANTID_CUSTOMINTERFACES_WORKSPACENAMESFACTORY_H_
#define MANTID_CUSTOMINTERFACES_WORKSPACENAMESFACTORY_H_
#include "ReductionWorkspaces.h"
#include "SlicedReductionWorkspaces.h"
namespace MantidQt {
namespace CustomInterfaces {
namespace internal {

template <typename WorkspaceNames> class WorkspaceNamesFactoryDetail;

template <> class WorkspaceNamesFactoryDetail<ReductionWorkspaces> {
public:
  static ReductionWorkspaces
  makeNames(std::vector<std::string> const &runNumbers,
            std::pair<std::string, std::string> const &transmissionRuns,
            Slicing const &) {
    return workspaceNamesForUnsliced(runNumbers, transmissionRuns);
  }
};

template <> class WorkspaceNamesFactoryDetail<SlicedReductionWorkspaces> {
public:
  static SlicedReductionWorkspaces
  makeNames(std::vector<std::string> const &runNumbers,
            std::pair<std::string, std::string> const &transmissionRuns,
            Slicing const &slicing) {
    return workspaceNamesForSliced(runNumbers, transmissionRuns, slicing);
  }
};
}

class MANTIDQT_ISISREFLECTOMETRY_DLL WorkspaceNamesFactory {
public:
  WorkspaceNamesFactory(Slicing const &slicing) : m_slicing(slicing) {}

  template <typename WorkspaceNames>
  WorkspaceNames
  makeNames(std::vector<std::string> const &runNumbers,
            std::pair<std::string, std::string> const &transmissionRuns) const {
    return internal::WorkspaceNamesFactoryDetail<WorkspaceNames>::makeNames(
        runNumbers, transmissionRuns, m_slicing);
  }

private:
  Slicing const &m_slicing;
};
}
}
#endif // MANTID_CUSTOMINTERFACES_WORKSPACENAMESFACTORY_H_
