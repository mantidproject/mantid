#ifndef MANTID_DATAHANDLING_SORTTABLEWORKSPACETEST_H_
#define MANTID_DATAHANDLING_SORTTABLEWORKSPACETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SortTableWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/TableRow.h"

using Mantid::DataHandling::SortTableWorkspace;
using namespace Mantid::API;

class SortTableWorkspaceTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SortTableWorkspaceTest *createSuite() { return new SortTableWorkspaceTest(); }
  static void destroySuite( SortTableWorkspaceTest *suite ) { delete suite; }


  void test_Init()
  {
    SortTableWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {

    auto ws = WorkspaceFactory::Instance().createTable();
    ws->addColumn("int","Number1");
    ws->addColumn("int","Number2");
    ws->addColumn("int","Number3");

    TableRow row = ws->appendRow();
    row << 1 << 11 << 111;
    row = ws->appendRow();
    row << 2 << 22 << 222;
    row = ws->appendRow();
    row << 3 << 33 << 333;
    row = ws->appendRow();
    row << 4 << 44 << 444;
    row = ws->appendRow();
    row << 5 << 55 << 555;
    row = ws->appendRow();
    row << 6 << 66 << 666;
    row = ws->appendRow();
    row << 7 << 77 << 777;
    row = ws->appendRow();
    row << 8 << 88 << 888;


    // Name of the output workspace.
    std::string outWSName("SortTableWorkspaceTest_OutputWS");
  
    SortTableWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("InputWorkspace", ws) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service. TODO: Change to your desired type
    Workspace_sptr outws;
    TS_ASSERT_THROWS_NOTHING( outws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName) );
    TS_ASSERT(outws);
    if (!outws) return;
    
    // TODO: Check the results
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().clear();
  }
  
};


#endif /* MANTID_DATAHANDLING_SORTTABLEWORKSPACETEST_H_ */