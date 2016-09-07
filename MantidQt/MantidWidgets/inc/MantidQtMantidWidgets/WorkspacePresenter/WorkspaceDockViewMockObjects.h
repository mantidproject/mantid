#ifndef MANTID_MANTIDWIDGETS_WORKSPACEDOCKVIEWMOCKOBJECTS_H
#define MANTID_MANTIDWIDGETS_WORKSPACEDOCKVIEWMOCKOBJECTS_H

#include "MantidKernel/WarningSuppressions.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtMantidWidgets/WorkspacePresenter/IWorkspaceDockView.h"
#include <gmock/gmock.h>

using namespace MantidQt::MantidWidgets;

GCC_DIAG_OFF_SUGGEST_OVERRIDE

class MockWorkspaceDockView : public IWorkspaceDockView {
public:
  MockWorkspaceDockView() {}
  ~MockWorkspaceDockView() override {}

  // Methods which are not to be mocked
  WorkspacePresenter_wptr getPresenterWeakPtr() override {
    return WorkspacePresenter_wptr();
  }
};

GCC_DIAG_ON_SUGGEST_OVERRIDE

#endif //MANTID_MANTIDWIDGETS_WORKSPACEDOCKVIEWMOCKOBJECTS_H