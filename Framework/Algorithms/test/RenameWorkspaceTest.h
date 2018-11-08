// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef RENAMEWORKSPACETEST_H_
#define RENAMEWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmHistory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAlgorithms/RenameWorkspace.h"
#include "MantidKernel/Exception.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

class RenameWorkspaceTest : public CxxTest::TestSuite {
public:
  void testName() { TS_ASSERT_EQUALS(alg.name(), "RenameWorkspace"); }

  void testVersion() { TS_ASSERT_EQUALS(alg.version(), 1); }

  void testInit() {
    Mantid::Algorithms::RenameWorkspace alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());

    const std::vector<Property *> props = alg2.getProperties();
    TS_ASSERT_EQUALS(props.size(), 4);

    TS_ASSERT_EQUALS(props[0]->name(), "InputWorkspace");
    TS_ASSERT(props[0]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<Workspace> *>(props[0]));

    TS_ASSERT_EQUALS(props[1]->name(), "OutputWorkspace");
    TS_ASSERT(props[1]->isDefault());
    TS_ASSERT(dynamic_cast<WorkspaceProperty<Workspace> *>(props[1]));
  }

  void testExec() {
    MatrixWorkspace_sptr inputWS = createWorkspace();
    AnalysisDataService::Instance().add("InputWS", inputWS);

    Mantid::Algorithms::RenameWorkspace alg3;
    alg3.initialize();
    TS_ASSERT_THROWS_NOTHING(
        alg3.setPropertyValue("InputWorkspace", "InputWS"));
    TS_ASSERT_THROWS_NOTHING(
        alg3.setPropertyValue("OutputWorkspace", "WSRenamed"));

    TS_ASSERT_THROWS_NOTHING(alg3.execute());
    TS_ASSERT(alg3.isExecuted());

    Workspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result =
            AnalysisDataService::Instance().retrieveWS<Workspace>("WSRenamed"));
    TS_ASSERT(result);

    TS_ASSERT_THROWS_ANYTHING(
        result =
            AnalysisDataService::Instance().retrieveWS<Workspace>("InputWS"));

    AnalysisDataService::Instance().remove("WSRenamed");
  }

  void testExecSameNames() {
    AnalysisDataService::Instance().clear();
    MatrixWorkspace_sptr inputWS = createWorkspace();
    AnalysisDataService::Instance().add("InputWS", inputWS);

    Mantid::Algorithms::RenameWorkspace alg3;
    alg3.initialize();
    TS_ASSERT_THROWS_NOTHING(
        alg3.setPropertyValue("InputWorkspace", "InputWS"));
    TS_ASSERT_THROWS_NOTHING(
        alg3.setPropertyValue("OutputWorkspace", "InputWS"));

    TS_ASSERT_THROWS(alg3.execute(), std::runtime_error);
    TS_ASSERT(!alg3.isExecuted());

    Workspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result =
            AnalysisDataService::Instance().retrieveWS<Workspace>("InputWS"));
    TS_ASSERT(result);

    AnalysisDataService::Instance().remove("InputWS");
  }

  void testExecNameAlreadyExists() {
    // Tests renaming a workspace to a name which is already used
    AnalysisDataService::Instance().clear();
    MatrixWorkspace_sptr inputWs = createWorkspace();
    AnalysisDataService::Instance().add("ExistingWorkspace", inputWs);
    // Create a workspace to rename
    MatrixWorkspace_sptr toRename = createWorkspace();
    AnalysisDataService::Instance().add("WorkspaceToRename", toRename);

    // First test it fails with override existing set to false
    Mantid::Algorithms::RenameWorkspace renameAlgorithm;
    renameAlgorithm.initialize();

    TS_ASSERT_THROWS_NOTHING(renameAlgorithm.setPropertyValue(
        "InputWorkspace", "WorkspaceToRename"));
    TS_ASSERT_THROWS_NOTHING(renameAlgorithm.setPropertyValue(
        "OutputWorkspace", "ExistingWorkspace"));
    TS_ASSERT_THROWS_NOTHING(
        renameAlgorithm.setProperty("OverwriteExisting", false));

    // Try to rename it should throw exception
    renameAlgorithm.setRethrows(true);
    TS_ASSERT_THROWS(renameAlgorithm.execute(), std::runtime_error);
    TS_ASSERT_EQUALS(renameAlgorithm.isExecuted(), false);

    TS_ASSERT_THROWS_NOTHING(
        renameAlgorithm.setProperty("OverwriteExisting", true));
    TS_ASSERT_THROWS_NOTHING(renameAlgorithm.execute());
    TS_ASSERT(renameAlgorithm.isExecuted());
  }

  void testGroup() {
    const std::string groupName = "oldName";
    AnalysisDataServiceImpl &ads = AnalysisDataService::Instance();
    WorkspaceGroup_sptr group(new WorkspaceGroup);
    ads.add(groupName, group);
    MatrixWorkspace_sptr member1 = createWorkspace();
    ads.add("oldName_1", member1);
    group->add("oldName_1");
    MatrixWorkspace_sptr member2 = createWorkspace();
    ads.add("oldName_2", member2);
    group->add("oldName_2");

    Mantid::Algorithms::RenameWorkspace renamer;
    renamer.initialize();
    TS_ASSERT_THROWS_NOTHING(
        renamer.setPropertyValue("InputWorkspace", groupName))
    TS_ASSERT_THROWS_NOTHING(
        renamer.setPropertyValue("OutputWorkspace", "newName"));
    TS_ASSERT(renamer.execute())

    Workspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(result = ads.retrieve("newName"))
    WorkspaceGroup_sptr resultGroup;
    TS_ASSERT(resultGroup = boost::dynamic_pointer_cast<WorkspaceGroup>(result))
    // It should actually be the same workspace as the input
    TS_ASSERT(resultGroup == group)
    // The output group should have the same workspaces in, with new names of
    // course
    auto item1 = resultGroup->getItem(0);
    auto item2 = resultGroup->getItem(1);
    TS_ASSERT_EQUALS(resultGroup->size(), 2)
    TS_ASSERT_EQUALS(item1, member1)
    TS_ASSERT_EQUALS(item1->getName(), "newName_1")
    TS_ASSERT_EQUALS(item2, member2)
    TS_ASSERT_EQUALS(item2->getName(), "newName_2")
    // Test history for the group is passed properly
    auto item1History = item1->history();
    TS_ASSERT_EQUALS(item1History.size(), 1)
    TS_ASSERT_EQUALS(item1History, item2->history())
    // Test that the history has only got the original group command and not
    // child commands
    TS_ASSERT_EQUALS(item1History[0]->name(), "RenameWorkspace")
    TS_ASSERT_EQUALS(item1History[0]->getPropertyValue("InputWorkspace"),
                     groupName)

    // The old ones should not be in the ADS
    TS_ASSERT_THROWS(ads.retrieve("oldName"),
                     Mantid::Kernel::Exception::NotFoundError)
    TS_ASSERT_THROWS(ads.retrieve("oldName_1"),
                     Mantid::Kernel::Exception::NotFoundError)
    TS_ASSERT_THROWS(ads.retrieve("oldName_2"),
                     Mantid::Kernel::Exception::NotFoundError)
    // The new ones should be
    TS_ASSERT_THROWS_NOTHING(ads.retrieve("newName_1"))
    TS_ASSERT_THROWS_NOTHING(ads.retrieve("newName_1"))

    // Clean up
    ads.clear();
  }

  void testRenameMonitorWS() {
    AnalysisDataService::Instance().clear();
    MatrixWorkspace_sptr inputWS = createWorkspace();
    AnalysisDataService::Instance().add("InputWS", inputWS);
    MatrixWorkspace_sptr monWS = createWorkspace();
    inputWS->setMonitorWorkspace(monWS);

    Mantid::Algorithms::RenameWorkspace alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "InputWS"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "WS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("RenameMonitors", true));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    Workspace_sptr result;
    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<Workspace>("WS"));
    TS_ASSERT(result);
    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<Workspace>(
            "WS_monitors"));
    TS_ASSERT(result);

    AnalysisDataService::Instance().remove("WS_monitors");
    TS_ASSERT_THROWS(
        AnalysisDataService::Instance().retrieveWS<Workspace>("WS_monitors"),
        Exception::NotFoundError);
    // monitors resurrected
    alg.setPropertyValue("InputWorkspace", "WS");
    alg.setPropertyValue("OutputWorkspace", "WS1");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<Workspace>("WS1"));
    TS_ASSERT(result);
    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<Workspace>(
            "WS1_monitors"));
    TS_ASSERT(result);
    // monitors renamed
    alg.setPropertyValue("InputWorkspace", "WS1");
    alg.setPropertyValue("OutputWorkspace", "WS2");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<Workspace>("WS2"));
    TS_ASSERT(result);
    TS_ASSERT_THROWS_NOTHING(
        result = AnalysisDataService::Instance().retrieveWS<Workspace>(
            "WS2_monitors"));
    TS_ASSERT(result);

    AnalysisDataService::Instance().remove("WS2");
    AnalysisDataService::Instance().remove("WS2_monitors");
  }
  MatrixWorkspace_sptr createWorkspace() {
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceBinned(4, 4, 0.5);

    return inputWS;
  }

private:
  Mantid::Algorithms::RenameWorkspace alg;
};

#endif /*RENAMEWORKSPACETEST_H_*/
