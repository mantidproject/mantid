#ifndef MANTID_MANTIDWIDGETS_WORKSPACEDOCKVIEWMOCKOBJECTS_H
#define MANTID_MANTIDWIDGETS_WORKSPACEDOCKVIEWMOCKOBJECTS_H

#include "MantidKernel/WarningSuppressions.h"
#include "MantidKernel/make_unique.h"
#include <MantidAPI/Workspace.h>
#include <MantidQtMantidWidgets/WorkspacePresenter/WorkspaceDockView.h>
#include <MantidQtMantidWidgets/WorkspacePresenter/WorkspacePresenter.h>
#include <boost/enable_shared_from_this.hpp>
#include <boost/make_shared.hpp>
#include <gmock/gmock.h>

using namespace MantidQt::MantidWidgets;

GCC_DIAG_OFF_SUGGEST_OVERRIDE

class MockWorkspaceDockView : public IWorkspaceDockView {
public:
  MockWorkspaceDockView() {}
  ~MockWorkspaceDockView() override {}

  void init() override {
    presenter = boost::make_shared<WorkspacePresenter>(shared_from_this());
    presenter->init();
  }

  MOCK_METHOD0(showLoadDialog, void());
  MOCK_METHOD0(showLiveDataDialog, void());
  MOCK_CONST_METHOD0(deleteConfirmation, bool());
  MOCK_METHOD0(deleteWorkspaces, void());
  MOCK_CONST_METHOD1(showRenameDialog, void(const StringList &names));
  MOCK_CONST_METHOD1(groupWorkspaces, void(const StringList &names));
  MOCK_CONST_METHOD1(ungroupWorkspaces, void(const StringList &names));
  MOCK_CONST_METHOD0(getSortDirection, SortDirection());
  MOCK_CONST_METHOD0(getSortCriteria, SortCriteria());
  MOCK_METHOD2(sortWorkspaces,
               void(IWorkspaceDockView::SortCriteria criteria,
                    IWorkspaceDockView::SortDirection direction));
  MOCK_CONST_METHOD0(getSaveFileType, SaveFileType());
  MOCK_METHOD1(saveWorkspace, void(SaveFileType type));
  MOCK_METHOD0(saveWorkspaces, void());
  MOCK_METHOD1(
      updateTree,
      void(const std::map<std::string, Mantid::API::Workspace_sptr> &items));

  MOCK_CONST_METHOD0(getSelectedWorkspaceNames, StringList());
  MOCK_CONST_METHOD0(getSelectedWorkspace, Mantid::API::Workspace_sptr());

  // Methods which are not to be mocked
  WorkspacePresenter_wptr getPresenterWeakPtr() override { return presenter; }
  WorkspacePresenter_sptr getPresenterSharedPtr() override { return presenter; }

private:
  WorkspacePresenter_sptr presenter;
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTID_MANTIDWIDGETS_WORKSPACEDOCKVIEWMOCKOBJECTS_H