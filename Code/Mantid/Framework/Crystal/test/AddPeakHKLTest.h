#ifndef MANTID_CRYSTAL_ADDPEAKHKLTEST_H_
#define MANTID_CRYSTAL_ADDPEAKHKLTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCrystal/AddPeakHKL.h"

using Mantid::Crystal::AddPeakHKL;
using namespace Mantid::API;

class AddPeakHKLTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AddPeakHKLTest *createSuite() { return new AddPeakHKLTest(); }
  static void destroySuite( AddPeakHKLTest *suite ) { delete suite; }


  void test_Init()
  {
    AddPeakHKL alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("AddPeakHKLTest_OutputWS");

    AddPeakHKL alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("REPLACE_PROPERTY_NAME_HERE!!!!", "value") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    // Retrieve the workspace from data service. TODO: Change to your desired type
    Workspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<Workspace>(outWSName) );
    TS_ASSERT(ws);
    if (!ws) return;

    // TODO: Check the results

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  
  void test_Something()
  {
    TSM_ASSERT( "You forgot to write a test!", 0);
  }


};


#endif /* MANTID_CRYSTAL_ADDPEAKHKLTEST_H_ */