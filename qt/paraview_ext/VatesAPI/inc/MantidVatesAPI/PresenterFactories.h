// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_VATES_PRESENTER_FACTORIES_H
#define MANTID_VATES_PRESENTER_FACTORIES_H

#include "MantidAPI/IMDWorkspace.h"


namespace Mantid {
namespace VATES {

// Forward Decalaration
class MDLoadingView;
class WorkspaceProvider;

class DLLExport EmptyWorkspaceNamePolicy {
protected:
  const std::string &
  getWorkspaceName(const Mantid::API::IMDWorkspace & /*workspace*/);
};

class DLLExport NonEmptyWorkspaceNamePolicy {
protected:
  const std::string &
  getWorkspaceName(const Mantid::API::IMDWorkspace &workspace) {
    return workspace.getName();
  }
};

/**
 * This templated function sets up an in memory loading presenter.
 * @param view: the loading view type
 * @param wsName: the name of the workspace which is to be displayed
 * @param worksapceProvider: a worksapce provider
 * @returns a new in memory loading presenter.
 */
template <class Presenter, class WorkspaceNamePolicy>
class DLLExport InMemoryPresenterFactory : private WorkspaceNamePolicy {
  using WorkspaceNamePolicy::getWorkspaceName;

public:
  std::unique_ptr<Presenter>
  create(std::unique_ptr<MDLoadingView> view,
         Mantid::API::IMDWorkspace_sptr workspace,
         std::unique_ptr<WorkspaceProvider> workspaceProvider) {
    return std::make_unique<Presenter>(std::move(view),
                                                  workspaceProvider.release(),
                                                  getWorkspaceName(*workspace));
  }
};
} // namespace VATES
} // namespace Mantid
#endif
