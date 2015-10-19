#ifndef MANTID_ALGORITHMS_ADDHISTORYNOTETEST_H_
#define MANTID_ALGORITHMS_ADDHISTORYNOTETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/AddHistoryNote.h"
#include "MantidAPI/Workspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::AddHistoryNote;
using namespace Mantid::API;

class AddHistoryNoteTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AddHistoryNoteTest *createSuite() { return new AddHistoryNoteTest(); }
  static void destroySuite( AddHistoryNoteTest *suite ) { delete suite; }


  void test_Init()
  {
    AddHistoryNote alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {
    std::string wsName = "AddHistoryNoteTest_Exec_workspace";
    // Create test input
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    AnalysisDataService::Instance().add(wsName,ws);
    //and an identical ws for comparison later
    auto ws2 = WorkspaceCreationHelper::Create2DWorkspace(10, 10);

    AddHistoryNote alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Workspace", wsName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Note", 
      "The next algorithm is doing ws equals 1/ws") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );


    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(
        outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            wsName));
    TS_ASSERT(outputWS);
    if (!outputWS)
      return;

    IAlgorithm_sptr lastAlgorithm = outputWS->getHistory().lastAlgorithm();
    
    TS_ASSERT_EQUALS(lastAlgorithm->getPropertyValue("Workspace"),
      alg.getPropertyValue("Workspace"));
    TS_ASSERT_EQUALS(lastAlgorithm->getPropertyValue("Note"),
      alg.getPropertyValue("Note"));
    std::cout << alg.getPropertyValue("Note") << std::endl;
    TSM_ASSERT("The workspace has been altered by AddHistoryNote",Mantid::API::equals(ws, ws2));
    
    AnalysisDataService::Instance().remove(wsName);
  }
  



};


#endif /* MANTID_ALGORITHMS_ADDHISTORYNOTETEST_H_ */