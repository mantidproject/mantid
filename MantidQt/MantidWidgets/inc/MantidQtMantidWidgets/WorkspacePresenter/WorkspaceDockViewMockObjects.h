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

  MOCK_CONST_METHOD2(askUserYesNo, bool(const std::string &caption,
                                        const std::string &message));
  MOCK_CONST_METHOD2(showCriticalUserMessage, void(const std::string &caption,
                                                   const std::string &message));

  MOCK_METHOD0(showLoadDialog, void());
  MOCK_METHOD0(showLiveDataDialog, void());
  MOCK_CONST_METHOD0(isPromptDelete, bool());
  MOCK_CONST_METHOD0(deleteConfirmation, bool());
  MOCK_METHOD1(deleteWorkspaces, void(const StringList &wsNames));
  MOCK_METHOD0(clearView, void());
  MOCK_METHOD2(recordWorkspaceRename,
               void(const std::string &oldName, const std::string &newName));
  MOCK_CONST_METHOD1(showRenameDialog, void(const StringList &wsNames));
  MOCK_CONST_METHOD2(groupWorkspaces, void(const StringList &wsNames,
                                           const std::string &groupName));
  MOCK_CONST_METHOD1(ungroupWorkspaces, void(const StringList &wsNames));
  MOCK_CONST_METHOD0(getSortDirection, SortDirection());
  MOCK_CONST_METHOD0(getSortCriteria, SortCriteria());
  MOCK_METHOD2(sortWorkspaces,
               void(IWorkspaceDockView::SortCriteria criteria,
                    IWorkspaceDockView::SortDirection direction));
  MOCK_CONST_METHOD0(getSaveFileType, SaveFileType());
  MOCK_METHOD2(saveWorkspace,
               void(const std::string &wsName, SaveFileType type));
  MOCK_METHOD1(saveWorkspaces, void(const StringList &wsNames));
  MOCK_METHOD1(
      updateTree,
      void(const std::map<std::string, Mantid::API::Workspace_sptr> &items));

  MOCK_CONST_METHOD0(getFilterText, std::string());
  MOCK_METHOD1(filterWorkspaces, void(const std::string &filterText));

  MOCK_CONST_METHOD0(getSelectedWorkspaceNames, StringList());
  MOCK_CONST_METHOD0(getSelectedWorkspace, Mantid::API::Workspace_sptr());

  // Methods which are not to be mocked
  void enableDeletePrompt(bool) override {}
  WorkspacePresenter_wptr getPresenterWeakPtr() override { return presenter; }
  WorkspacePresenter_sptr getPresenterSharedPtr() override { return presenter; }

private:
  WorkspacePresenter_sptr presenter;
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif // MANTID_MANTIDWIDGETS_WORKSPACEDOCKVIEWMOCKOBJECTS_H