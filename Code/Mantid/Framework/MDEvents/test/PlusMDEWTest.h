#ifndef MANTID_MDEVENTS_PLUSMDEWTEST_H_
#define MANTID_MDEVENTS_PLUSMDEWTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDEvents/PlusMDEW.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::API;

class PlusMDEWTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    PlusMDEW alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void do_test(bool lhs_file, bool rhs_file, int inPlace)
  {
    // Make two input workspaces
    MDEventWorkspace3Lean::sptr lhs = MDEventsTestHelper::makeFileBackedMDEW("PlusMDEWTest_lhs", lhs_file);
    MDEventWorkspace3Lean::sptr rhs = MDEventsTestHelper::makeFileBackedMDEW("PlusMDEWTest_rhs", rhs_file);
    std::string outWSName = "PlusMDEWTest_out";
    if (inPlace == 1)
      outWSName = "PlusMDEWTest_lhs";
    else if (inPlace == 2)
      outWSName = "PlusMDEWTest_rhs";
  
    PlusMDEW alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("LHSWorkspace", "PlusMDEWTest_lhs") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("RHSWorkspace", "PlusMDEWTest_rhs") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    MDEventWorkspace3Lean::sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(ws); if (!ws) return;
    
    // Check the results
    if (inPlace == 1)
      { TS_ASSERT( ws == lhs); }
    else if (inPlace == 2)
      { TS_ASSERT( ws == rhs); }
    
    if ((lhs_file || rhs_file) && !((inPlace==1) && !lhs_file && rhs_file))
      { TSM_ASSERT( "If either input WS is file backed, then the output should be too.", ws->getBoxController()->isFileBacked() ); }
    TS_ASSERT_EQUALS( ws->getNPoints(), 20000);

  }
  
  void test_mem_plus_mem()
  { do_test(false, false, 0); }

  void test_mem_plus_mem_inPlace()
  { do_test(false, false, 1); }

  void test_mem_plus_mem_inPlace_ofRHS()
  { do_test(false, false, 2); }

  void test_file_plus_mem()
  { do_test(true, false, 0); }

  void test_file_plus_mem_inPlace()
  { do_test(true, false, 1); }

  void test_mem_plus_file()
  { do_test(false, true, 0); }

  void test_mem_plus_file_inPlace()
  { do_test(false, true, 1); }

  void test_file_plus_file()
  { do_test(true, true, 0); }

  void test_file_plus_file_inPlace()
  { do_test(true, true, 1); }

  void test_file_plus_file_inPlace_ofRHS()
  { do_test(true, true, 2); }

  void test_file_plus_file_thenSaveMDEW()
  {
    do_test(true, true, 0);
    AlgorithmHelper::runAlgorithm("SaveMDEW", 4,
        "InputWorkspace", "PlusMDEWTest_out",
        "UpdateFileBackEnd", "1");
  }

};


#endif /* MANTID_MDEVENTS_PLUSMDEWTEST_H_ */

