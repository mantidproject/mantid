#ifndef MANTID_MDEVENTS_PLUSMDEWTEST_H_
#define MANTID_MDEVENTS_PLUSMDEWTEST_H_

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDAlgorithms/PlusMD.h"
#include "MantidNexusCPP/NeXusFile.hpp"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include <Poco/File.h>
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"
#include "MantidAPI/FrameworkManager.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;

class PlusMDTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    PlusMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void do_test(bool lhs_file, bool rhs_file, int inPlace, bool deleteFile=true)
  {
    AnalysisDataService::Instance().clear();
    // Make two input workspaces
    MDEventWorkspace3Lean::sptr lhs = MDEventsTestHelper::makeFileBackedMDEW("PlusMDTest_lhs", lhs_file);
    MDEventWorkspace3Lean::sptr rhs = MDEventsTestHelper::makeFileBackedMDEW("PlusMDTest_rhs", rhs_file);
    std::string outWSName = "PlusMDTest_out";
    if (inPlace == 1)
      outWSName = "PlusMDTest_lhs";
    else if (inPlace == 2)
      outWSName = "PlusMDTest_rhs";
  
    PlusMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("LHSWorkspace", "PlusMDTest_lhs") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("RHSWorkspace", "PlusMDTest_rhs") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    MDEventWorkspace3Lean::sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>(outWSName) );
    TS_ASSERT(ws); if (!ws) return;
    
    // Check the results
    if (inPlace == 1)
      { TS_ASSERT( ws == lhs); }
    else if (inPlace == 2)
      { TS_ASSERT( ws == rhs); }
    
    if ((lhs_file || rhs_file) && !((inPlace==1) && !lhs_file && rhs_file))
      { TSM_ASSERT( "If either input WS is file backed, then the output should be too.", ws->getBoxController()->isFileBacked() ); }
    TS_ASSERT_EQUALS( ws->getNPoints(), 20000);

    TSM_ASSERT("If the workspace is file-backed, then it needs updating.", ws->fileNeedsUpdating() );

    if (ws->isFileBacked())
    {

      // Run SaveMD so as to update the file in the back
      FrameworkManager::Instance().exec("SaveMD", 4,
          "InputWorkspace", outWSName.c_str(),
          "UpdateFileBackEnd", "1");

      Mantid::API::BoxController_sptr bc = ws->getBoxController();
      std::cout << bc->getDiskBuffer().getFreeSpaceMap().size() << " entries in the free space map" << std::endl;
      ::NeXus::File * file = bc->getFile();
      // The file should have an entry of 20000 points too (with some error due to the free space blocks). This means the file back-end was updated
      TS_ASSERT_DELTA(file->getInfo().dims[0], 20000, 100);

      // Close the file so you can delete it. Otherwise the following test gets confused.
      if (deleteFile)
        ws->getBoxController()->closeFile(true);
    }
    //cleanup
    if ((inPlace==1)&&rhs->isFileBacked())
    {
        rhs->getBoxController()->closeFile(true);
    }
    if ((inPlace==2)&&lhs->isFileBacked())
    {
        lhs->getBoxController()->closeFile(true);
    }
    if (ws->isFileBacked())
    {
        ws->getBoxController()->closeFile(true);
    }
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




  void test_histo_histo()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("PlusMD", "histo_A", "histo_B", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 5.0, 1e-5);
  }

  void test_histo_scalar()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("PlusMD", "histo_A", "scalar", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 5.0, 1e-5);
    out = BinaryOperationMDTestHelper::doTest("PlusMD", "scalar", "histo_A", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), 5.0, 1e-5);
  }

  void test_event_scalar_fails()
  {
    BinaryOperationMDTestHelper::doTest("PlusMD", "event_A", "scalar", "out", false /*fails*/);
    BinaryOperationMDTestHelper::doTest("PlusMD", "scalar", "event_A", "out", false /*fails*/);
  }

  void test_event_histo_fails()
  {
    BinaryOperationMDTestHelper::doTest("PlusMD", "event_A", "histo_A", "out", false /*fails*/);
    BinaryOperationMDTestHelper::doTest("PlusMD", "histo_A", "event_A", "out", false /*fails*/);
  }

};


#endif /* MANTID_MDEVENTS_PLUSMDEWTEST_H_ */
