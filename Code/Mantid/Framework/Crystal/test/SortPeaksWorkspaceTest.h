#ifndef MANTID_CRYSTAL_SORTPEAKSWORKSPACETEST_H_
#define MANTID_CRYSTAL_SORTPEAKSWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/SortPeaksWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataObjects/PeaksWorkspace.h"

using namespace Mantid::Crystal;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class SortPeaksWorkspaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SortPeaksWorkspaceTest *createSuite() { return new SortPeaksWorkspaceTest(); }
  static void destroySuite( SortPeaksWorkspaceTest *suite ) { delete suite; }


  void test_Init()
  {
    SortPeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("SortPeaksWorkspaceTest_OutputWS");

    PeaksWorkspace_sptr inWS = WorkspaceCreationHelper::createPeaksWorkspace();
  
    SortPeaksWorkspace alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", inWS) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("ColumnNameToSortBy", "V"));
    //TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    //TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service. TODO: Change to your desired type
    //Workspace_sptr ws;
    //TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName) );
    //TS_ASSERT(ws);
    //if (!ws) return;
    
    // TODO: Check the results
    
    // Remove workspace from the data service.
    //AnalysisDataService::Instance().remove(outWSName);
  }

};


#endif /* MANTID_CRYSTAL_SORTPEAKSWORKSPACETEST_H_ */
