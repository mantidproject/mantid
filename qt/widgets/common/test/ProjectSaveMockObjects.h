#ifndef MANTIDQT_MANTIDWIDGETS_PROJECTSAVEMOCKOBJECTS_H
#define MANTIDQT_MANTIDWIDGETS_PROJECTSAVEMOCKOBJECTS_H

#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/IProjectSerialisable.h"
#include "MantidQtWidgets/Common/IProjectSaveView.h"

#include <gmock/gmock.h>

using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;

DIAG_OFF_SUGGEST_OVERRIDE

class MockProjectSaveView : public IProjectSaveView {
public:
  MOCK_METHOD0(getWindows, std::vector<IProjectSerialisable *>());
  MOCK_METHOD0(getCheckedWorkspaceNames, std::vector<std::string>());
  MOCK_METHOD0(getUncheckedWorkspaceNames, std::vector<std::string>());
  MOCK_METHOD0(getProjectPath, QString());
  MOCK_METHOD1(setProjectPath, void(const QString &));
  MOCK_METHOD1(updateWorkspacesList, void(const std::vector<WorkspaceInfo> &));
  MOCK_METHOD1(updateIncludedWindowsList,
               void(const std::vector<WindowInfo> &));
  MOCK_METHOD1(updateExcludedWindowsList,
               void(const std::vector<WindowInfo> &));
  MOCK_METHOD1(removeFromIncludedWindowsList,
               void(const std::vector<std::string> &));
  MOCK_METHOD1(removeFromExcludedWindowsList,
               void(const std::vector<std::string> &));
};

//==============================================================================

class WindowStub : public IProjectSerialisable {
private:
  std::string m_name;
  std::vector<std::string> m_wsNames;

public:
  WindowStub(const std::string &name, const std::vector<std::string> &wsNames)
      : m_name(name), m_wsNames(wsNames.cbegin(), wsNames.cend()) {}

  std::string saveToProject(ApplicationWindow *app) override {
    UNUSED_ARG(app);
    return "";
  }

  std::vector<std::string> getWorkspaceNames() override { return m_wsNames; }

  std::string getWindowName() override { return m_name; }

  std::string getWindowType() override { return "Matrix"; }
};

#endif
