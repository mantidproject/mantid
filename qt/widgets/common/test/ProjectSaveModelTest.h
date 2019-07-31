// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_MANTIDWIDGETS_PROJECTSAVEMODELTEST_H
#define MANTIDQT_MANTIDWIDGETS_PROJECTSAVEMODELTEST_H

#include "MantidAPI/Workspace.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/IProjectSerialisable.h"
#include "MantidQtWidgets/Common/ProjectSaveModel.h"
#include "MantidQtWidgets/Common/ProjectSavePresenter.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "ProjectSaveMockObjects.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace MantidQt::API;
using namespace MantidQt::MantidWidgets;
using namespace testing;
using namespace Mantid::API;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

// Mock object for the model;
class MockProjectSaveModel : public ProjectSaveModel {
public:
  MockProjectSaveModel(
      std::vector<MantidQt::API::IProjectSerialisable *> windows,
      std::vector<std::string> activePythonInterfaces =
          std::vector<std::string>())
      : ProjectSaveModel(windows, activePythonInterfaces) {}
  MOCK_METHOD1(getProjectSize, size_t(const std::vector<std::string> &wsNames));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

namespace {
size_t calculateSize(std::vector<Workspace_sptr> workspaces) {
  size_t result = 0;
  for (const auto &ws : workspaces) {
    result += ws->getMemorySize();
  }
  return result;
}
} // namespace

//=====================================================================================
// Functional tests
//=====================================================================================
class ProjectSaveModelTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    auto ws1 = WorkspaceCreationHelper::create1DWorkspaceRand(10, true);
    WorkspaceCreationHelper::storeWS("ws1", ws1);
    auto ws2 = WorkspaceCreationHelper::create1DWorkspaceRand(10, true);
    WorkspaceCreationHelper::storeWS("ws2", ws2);
  }

  void tearDown() override {
    WorkspaceCreationHelper::removeWS("ws1");
    WorkspaceCreationHelper::removeWS("ws2");
  }

  void testConstructNoWorkspacesNoWindows() {
    tearDown(); // remove workspaces setup by default
    std::vector<MantidQt::API::IProjectSerialisable *> windows;
    TS_ASSERT_THROWS_NOTHING(ProjectSaveModel model(windows));
  }

  void testConstructOneWorkspaceNoWindows() {
    std::vector<MantidQt::API::IProjectSerialisable *> windows;

    ProjectSaveModel model(windows);
    TS_ASSERT(!model.hasWindows("ws1"));
    TS_ASSERT_EQUALS(model.getWindows("ws1").size(), 0);
  }

  void testGetWindowsForWorkspaceNoWindows() {
    std::vector<MantidQt::API::IProjectSerialisable *> windows;

    ProjectSaveModel model(windows);
    TS_ASSERT(!model.hasWindows("ws1"));
    TS_ASSERT_EQUALS(model.getWindows("ws1").size(), 0);
  }

  void testGetWindowsForWorkspaceOneWindow() {
    std::vector<MantidQt::API::IProjectSerialisable *> windows;
    WindowStub win1("window1", {"ws1"});
    windows.push_back(&win1);

    ProjectSaveModel model(windows);
    TS_ASSERT(model.hasWindows("ws1"));
    TS_ASSERT_EQUALS(model.getWindows("ws1").size(), 1);
  }

  void testGetWindowsForWorkspaceTwoWindows() {
    std::vector<MantidQt::API::IProjectSerialisable *> windows;
    WindowStub win1("window1", {"ws1"});
    WindowStub win2("window2", {"ws1"});
    windows.push_back(&win1);
    windows.push_back(&win2);

    ProjectSaveModel model(windows);
    TS_ASSERT(model.hasWindows("ws1"));
    TS_ASSERT_EQUALS(model.getWindows("ws1").size(), 2);
  }

  void testGetWindowsForTwoWorkspacesAndTwoWindows() {
    std::vector<MantidQt::API::IProjectSerialisable *> windows;
    WindowStub win1("window1", {"ws1"});
    WindowStub win2("window2", {"ws2"});
    windows.push_back(&win1);
    windows.push_back(&win2);

    ProjectSaveModel model(windows);
    TS_ASSERT(model.hasWindows("ws1"));
    TS_ASSERT_EQUALS(model.getWindows("ws1").size(), 1);
    TS_ASSERT(model.hasWindows("ws2"));
    TS_ASSERT_EQUALS(model.getWindows("ws2").size(), 1);
  }

  void testGetWorkspaceNames() {
    std::vector<MantidQt::API::IProjectSerialisable *> windows;

    ProjectSaveModel model(windows);
    TS_ASSERT(!model.hasWindows("ws1"));
    TS_ASSERT(!model.hasWindows("ws2"));

    auto names = model.getWorkspaceNames();
    TS_ASSERT_EQUALS(names.size(), 2);
    TS_ASSERT_EQUALS(names[0], "ws1");
    TS_ASSERT_EQUALS(names[1], "ws2");
  }

  void testGetInterfaceNames() {
    std::vector<MantidQt::API::IProjectSerialisable *> windows;
    std::vector<std::string> interfaces{"Test_Interface",
                                        "Test_Python_Interface_2"};

    ProjectSaveModel model(windows, interfaces);
    auto names = model.getAllPythonInterfaces();
    TS_ASSERT_EQUALS(2, names.size());
    TS_ASSERT_EQUALS("Test_Interface", names[0]);
    TS_ASSERT_EQUALS("Test_Python_Interface_2", names[1]);
  }

  void testGetWindowNames() {
    std::vector<MantidQt::API::IProjectSerialisable *> windows;

    WindowStub win1("window1", {"ws1"});
    WindowStub win2("window2", {"ws2"});
    WindowStub win3("window3", {"ws1", "ws2"});
    WindowStub win4("window4", {});
    windows.push_back(&win1);
    windows.push_back(&win2);
    windows.push_back(&win3);
    windows.push_back(&win4);

    ProjectSaveModel model(windows);
    auto names = model.getWindowNames({"ws1", "ws2"});
    TS_ASSERT_EQUALS(names.size(), 3);
    TS_ASSERT_EQUALS(names[0], "window1");
    TS_ASSERT_EQUALS(names[1], "window2");
    TS_ASSERT_EQUALS(names[2], "window3");

    names = model.getWindowNames({"ws1"});
    TS_ASSERT_EQUALS(names.size(), 2);
    TS_ASSERT_EQUALS(names[0], "window1");
    TS_ASSERT_EQUALS(names[1], "window3");

    names = model.getWindowNames({"ws2"});
    TS_ASSERT_EQUALS(names.size(), 2);
    TS_ASSERT_EQUALS(names[0], "window2");
    TS_ASSERT_EQUALS(names[1], "window3");
  }

  void testGetWindows() {
    std::vector<MantidQt::API::IProjectSerialisable *> windows;

    WindowStub win1("window1", {"ws1"});
    WindowStub win2("window2", {"ws2"});
    WindowStub win3("window3", {"ws1", "ws2"});
    WindowStub win4("window4", {});
    windows.push_back(&win1);
    windows.push_back(&win2);
    windows.push_back(&win3);
    windows.push_back(&win4);

    ProjectSaveModel model(windows);
    auto windowsSubset = model.getUniqueWindows({"ws1", "ws2"});
    TS_ASSERT_EQUALS(windowsSubset.size(), 3);
    TS_ASSERT_EQUALS(windowsSubset[0], &win1);
    TS_ASSERT_EQUALS(windowsSubset[1], &win2);
    TS_ASSERT_EQUALS(windowsSubset[2], &win3);

    windowsSubset = model.getUniqueWindows({"ws1"});
    TS_ASSERT_EQUALS(windowsSubset.size(), 2);
    TS_ASSERT_EQUALS(windowsSubset[0], &win1);
    TS_ASSERT_EQUALS(windowsSubset[1], &win3);

    windowsSubset = model.getUniqueWindows({"ws2"});
    TS_ASSERT_EQUALS(windowsSubset.size(), 2);
    TS_ASSERT_EQUALS(windowsSubset[0], &win2);
    TS_ASSERT_EQUALS(windowsSubset[1], &win3);
  }

  void testGetWorkspaceInformation() {
    ProjectSaveModel model({});
    auto wsInfo = model.getWorkspaceInformation();

    TS_ASSERT_EQUALS(wsInfo.size(), 2);

    TS_ASSERT_EQUALS(wsInfo[0].name, "ws1");
    TS_ASSERT_EQUALS(wsInfo[0].type, "Workspace2D");
    TS_ASSERT_EQUALS(wsInfo[0].size, "0 kB");
    TS_ASSERT_EQUALS(wsInfo[0].icon_id, "mantid_matrix_xpm");
    TS_ASSERT_EQUALS(wsInfo[0].numWindows, 0);

    TS_ASSERT_EQUALS(wsInfo[1].name, "ws2");
    TS_ASSERT_EQUALS(wsInfo[1].type, "Workspace2D");
    TS_ASSERT_EQUALS(wsInfo[1].size, "0 kB");
    TS_ASSERT_EQUALS(wsInfo[1].icon_id, "mantid_matrix_xpm");
    TS_ASSERT_EQUALS(wsInfo[1].numWindows, 0);
  }

  void testGetWorkspaceInformationWithGroup() {
    auto group =
        WorkspaceCreationHelper::createWorkspaceGroup(3, 1, 10, "ws-group");

    ProjectSaveModel model({});
    auto wsInfo = model.getWorkspaceInformation();

    TS_ASSERT_EQUALS(wsInfo.size(), 3);

    TS_ASSERT_EQUALS(wsInfo[0].name, "ws-group");
    TS_ASSERT_EQUALS(wsInfo[0].type, "WorkspaceGroup");
    TS_ASSERT_EQUALS(wsInfo[0].size, "0 kB");
    TS_ASSERT_EQUALS(wsInfo[0].icon_id, "mantid_wsgroup_xpm");
    TS_ASSERT_EQUALS(wsInfo[0].numWindows, 0);
    TS_ASSERT_EQUALS(wsInfo[0].subWorkspaces.size(), 3);

    int count = 0;
    for (auto &item : wsInfo[0].subWorkspaces) {
      TS_ASSERT_EQUALS(item.name, "ws-group_" + std::to_string(count));
      TS_ASSERT_EQUALS(item.type, "Workspace2D");
      TS_ASSERT_EQUALS(item.size, "0 kB");
      TS_ASSERT_EQUALS(item.icon_id, "mantid_matrix_xpm");
      TS_ASSERT_EQUALS(item.numWindows, 0);
      ++count;
    }

    TS_ASSERT_EQUALS(wsInfo[1].name, "ws1");
    TS_ASSERT_EQUALS(wsInfo[1].type, "Workspace2D");
    TS_ASSERT_EQUALS(wsInfo[1].size, "0 kB");
    TS_ASSERT_EQUALS(wsInfo[1].icon_id, "mantid_matrix_xpm");
    TS_ASSERT_EQUALS(wsInfo[1].numWindows, 0);
    TS_ASSERT_EQUALS(wsInfo[1].subWorkspaces.size(), 0);

    TS_ASSERT_EQUALS(wsInfo[2].name, "ws2");
    TS_ASSERT_EQUALS(wsInfo[2].type, "Workspace2D");
    TS_ASSERT_EQUALS(wsInfo[2].size, "0 kB");
    TS_ASSERT_EQUALS(wsInfo[2].icon_id, "mantid_matrix_xpm");
    TS_ASSERT_EQUALS(wsInfo[2].numWindows, 0);
    TS_ASSERT_EQUALS(wsInfo[2].subWorkspaces.size(), 0);

    WorkspaceCreationHelper::removeWS("ws-group");
  }

  void testGetWindowInformation() {
    std::vector<MantidQt::API::IProjectSerialisable *> windows;

    WindowStub win1("window1", {"ws1"});
    WindowStub win2("window2", {"ws2"});
    WindowStub win3("window3", {"ws1", "ws2"});
    WindowStub win4("window4", {});
    windows.push_back(&win1);
    windows.push_back(&win2);
    windows.push_back(&win3);
    windows.push_back(&win4);

    ProjectSaveModel model(windows);

    auto winInfo = model.getWindowInformation({"ws1"});

    TS_ASSERT_EQUALS(winInfo.size(), 2);

    TS_ASSERT_EQUALS(winInfo[0].name, "window1");
    TS_ASSERT_EQUALS(winInfo[0].type, "Matrix");
    TS_ASSERT_EQUALS(winInfo[0].icon_id, "matrix_xpm");

    TS_ASSERT_EQUALS(winInfo[1].name, "window3");
    TS_ASSERT_EQUALS(winInfo[1].type, "Matrix");
    TS_ASSERT_EQUALS(winInfo[1].icon_id, "matrix_xpm");
  }

  void test_needsSizeWarning_is_false_with_empty_workspace() {
    const std::vector<std::string> wsNames{"ws1"};
    ProjectSaveModel model({});
    TS_ASSERT_EQUALS(model.needsSizeWarning(wsNames), false);
  }

  void test_needsSizeWarning_is_true_with_large_workspace() {
    std::vector<MantidQt::API::IProjectSerialisable *> windows;
    NiceMock<MockProjectSaveModel> model(windows);

    const std::vector<std::string> wsNames{"ws1", "ws2"};
    ON_CALL(model, getProjectSize(wsNames)).WillByDefault(Return(107374182411));

    TS_ASSERT(model.needsSizeWarning(wsNames));
  }

  void test_getProjectSize_returns_correctAnswer() {
    ProjectSaveModel model({});
    const auto workspaces = model.getWorkspaces();
    size_t assumedSize = calculateSize(workspaces);
    const auto workspaceNames = model.getWorkspaceNames();

    TS_ASSERT_EQUALS(model.getProjectSize(workspaceNames), assumedSize);
  }
};

#endif // MANTIDQT_MANTIDWIDGETS_PROJECTSAVEMODELTEST_H
