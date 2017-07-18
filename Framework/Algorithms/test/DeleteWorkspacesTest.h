//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/DeleteWorkspaces.h"
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/WorkspaceGroup.h"

#include <cxxtest/TestSuite.h>

class DeleteWorkspacesTest : public CxxTest::TestSuite {

public:
  void test_That_An_Existing_Workspace_Is_Deleted_After_Execution() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;

    // Need a test workspace registered within the ADS
    const int yLength = 20;
    AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
    const size_t storeSizeAtStart(dataStore.size());
    const std::string testName1 = "DeleteWorkspaces_testWS1";
    const std::string testName2 = "DeleteWorkspaces_testWS2";
    const std::string testName3 = "DeleteWorkspaces_testWS3";
    createAndStoreWorkspace(testName1);
    createAndStoreWorkspace(testName2);
    createAndStoreWorkspace(testName3, yLength);
    TS_ASSERT_EQUALS(dataStore.size(), storeSizeAtStart + 3);

    Mantid::Algorithms::DeleteWorkspaces alg;
    alg.initialize();
    alg.setPropertyValue("WorkspaceList", testName1 + ", " + testName2);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_EQUALS(dataStore.size(), storeSizeAtStart + 1);
    // Check that what is left is correct
    MatrixWorkspace_sptr wsRemain =
        boost::dynamic_pointer_cast<MatrixWorkspace>(
            dataStore.retrieve(testName3));
    TS_ASSERT(wsRemain);
    if (!wsRemain)
      TS_FAIL("Unable to retrieve remaining workspace.");

    TS_ASSERT_EQUALS(wsRemain->getNumberHistograms(), yLength);
    // Tidy up after test
    dataStore.remove(testName3);
  }

  void test_deleting_group_deletes_its_members() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;

    // Need a test workspace registered within the ADS
    AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
    dataStore.clear();

    const std::string testName1 = "DeleteWorkspaces_testWS1";
    const std::string testName2 = "DeleteWorkspaces_testWS2";

    createAndStoreWorkspace(testName1);
    createAndStoreWorkspace(testName2);

    auto group = WorkspaceGroup_sptr(new WorkspaceGroup);
    dataStore.add("group", group);
    group->add(testName1);
    group->add(testName2);

    TS_ASSERT_EQUALS(dataStore.size(), 3);

    Mantid::Algorithms::DeleteWorkspaces alg;
    alg.initialize();
    alg.setPropertyValue("WorkspaceList", "group");
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_EQUALS(dataStore.size(), 0);

    dataStore.clear();
  }

  void test_ignore_group_if_workspaces_inside_get_deleted_first() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;

    // Need a test workspace registered within the ADS
    AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
    const size_t storeSizeAtStart(dataStore.size());
    const std::string testName1 = "DeleteWorkspaces_testWS1";
    const std::string testName2 = "DeleteWorkspaces_testWS2";
    createAndStoreWorkspace(testName1);
    createAndStoreWorkspace(testName2);
    const std::string groupName = "DeleteWorkspaces_testGroup";
    Mantid::Algorithms::GroupWorkspaces groupingAlg;
    groupingAlg.initialize();
    groupingAlg.setPropertyValue("InputWorkspaces",
                                 testName1 + "," + testName2);
    groupingAlg.setPropertyValue("OutputWorkspace", groupName);
    groupingAlg.execute();
    TS_ASSERT_EQUALS(dataStore.size(), storeSizeAtStart + 3);

    Mantid::Algorithms::DeleteWorkspaces alg;
    alg.initialize();
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "WorkspaceList", testName1 + ", " + testName2 + ", " + groupName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_EQUALS(dataStore.size(), storeSizeAtStart);
  }

  void createAndStoreWorkspace(std::string name, int ylength = 10) {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;

    // create a test workspace registered within the ADS
    Workspace2D_sptr testWS1 =
        WorkspaceCreationHelper::create2DWorkspace(ylength, 10);
    AnalysisDataServiceImpl &dataStore = AnalysisDataService::Instance();
    dataStore.add(name, testWS1);
  }
};
