//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidAlgorithms/DeleteWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

class DeleteWorkspaceTest : public CxxTest::TestSuite
{

public:

  void test_That_An_Existing_Workspace_Is_Deleted_After_Execution()
  {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;

    // Need a test workspace registered within the ADS
    const int yLength1 = 10;
    Workspace2D_sptr testWS1 = WorkspaceCreationHelper::Create2DWorkspace(yLength1, 10);
    const int yLength2 = 20;
    Workspace2D_sptr testWS2 = WorkspaceCreationHelper::Create2DWorkspace(yLength2, 10);
    AnalysisDataServiceImpl & dataStore = AnalysisDataService::Instance();
    const size_t storeSizeAtStart(dataStore.size());
    const std::string testName1 = "DeleteWorkspace_testWS1";
    const std::string testName2 = "DeleteWorkspace_testWS2";
    dataStore.add(testName1, testWS1);
    dataStore.add(testName2, testWS2);
    TS_ASSERT_EQUALS(dataStore.size(), storeSizeAtStart + 2);

    Mantid::Algorithms::DeleteWorkspace alg;
    alg.initialize();
    alg.setPropertyValue("Workspace", testName1);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    TS_ASSERT_EQUALS(dataStore.size(), storeSizeAtStart + 1);
    // Check that what is left is correct
    MatrixWorkspace_sptr wsTwo = boost::dynamic_pointer_cast<MatrixWorkspace>(dataStore.retrieve(testName2));
    TS_ASSERT(wsTwo);
    if( !wsTwo ) TS_FAIL("Unable to retrieve remaining workspace.");

    TS_ASSERT_EQUALS(wsTwo->getNumberHistograms(), yLength2);
    // Tidy up after test
    dataStore.remove(testName2);
  }

};
