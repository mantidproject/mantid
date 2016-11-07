#ifndef MANTIDQTCUSTOMINTERFACES_PROJECTSAVEMODELTEST_H
#define MANTIDQTCUSTOMINTERFACES_PROJECTSAVEMODELTEST_H

#include <cxxtest/TestSuite.h>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidQtCustomInterfaces/ProjectSaveModel.h"

using namespace MantidQt::CustomInterfaces;

//=====================================================================================
// Functional tests
//=====================================================================================
class ProjectSaveModelTest : public CxxTest::TestSuite {

public:
  void setUp() override {
    auto ws1 = WorkspaceCreationHelper::Create1DWorkspaceRand(10);
    WorkspaceCreationHelper::storeWS("ws1", ws1);
    auto ws2 = WorkspaceCreationHelper::Create1DWorkspaceRand(10);
    WorkspaceCreationHelper::storeWS("ws2", ws2);
  }

  void tearDown() override {
    WorkspaceCreationHelper::removeWS("ws1");
    WorkspaceCreationHelper::removeWS("ws2");
  }

  void testConstructNoWorkspacesNoWindows() {
    tearDown(); // remove workspaces setup by default
    std::vector<MantidQt::API::IProjectSerialisable*> windows;
    TS_ASSERT_THROWS_NOTHING( ProjectSaveModel model(windows) );
  }

  void testConstructOneWorkspaceNoWindows() {
    std::vector<MantidQt::API::IProjectSerialisable*> windows;

    ProjectSaveModel model(windows);
    TS_ASSERT(!model.hasWindows("ws1"));
    TS_ASSERT_EQUALS(model.getWindows("ws1").size(), 0);
  }

  void testGetWindowsForWorkspace() {
    std::vector<MantidQt::API::IProjectSerialisable*> windows;

    ProjectSaveModel model(windows);
    TS_ASSERT(!model.hasWindows("ws1"));
  }

  void testGetWorkspaceNames() {
    std::vector<MantidQt::API::IProjectSerialisable*> windows;

    ProjectSaveModel model(windows);
    TS_ASSERT(!model.hasWindows("ws1"));
    TS_ASSERT(!model.hasWindows("ws2"));

    auto names = model.getWorkspaceNames();
    TS_ASSERT_EQUALS(names.size(), 2);
  }

};

#endif // MANTIDQTCUSTOMINTERFACES_PROJECTSAVEMODELTEST_H
