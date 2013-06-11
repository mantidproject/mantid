#ifndef MANTID_MDALGORITHMS_MINUSMDTEST_H_
#define MANTID_MDALGORITHMS_MINUSMDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDAlgorithms/MinusMD.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using namespace Mantid;
using namespace Mantid::MDEvents;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;

class MinusMDTest : public CxxTest::TestSuite
{
public:
  void test_Init()
  {
    MinusMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_histo_histo()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("MinusMD", "histo_A", "histo_B", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), -1.0, 1e-5);
  }

  void test_histo_scalar()
  {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("MinusMD", "histo_A", "scalar", "out");
    TS_ASSERT_DELTA( out->getSignalAt(0), -1.0, 1e-5);
    BinaryOperationMDTestHelper::doTest("MinusMD", "scalar", "histo_A", "out", false /*fails*/);
  }




  void do_test(bool lhs_file, bool rhs_file, int inPlace)
  {
    AnalysisDataService::Instance().clear();
    // Make two input workspaces
    MDEventWorkspace3Lean::sptr lhs = MDEventsTestHelper::makeFileBackedMDEW("MinusMDTest_lhs", lhs_file);
    MDEventWorkspace3Lean::sptr rhs = MDEventsTestHelper::makeFileBackedMDEW("MinusMDTest_rhs", rhs_file);
    std::string outWSName = "MinusMDTest_out";
    if (inPlace == 1)
      outWSName = "MinusMDTest_lhs";
    else if (inPlace == 2)
      outWSName = "MinusMDTest_rhs";

    MinusMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("LHSWorkspace", "MinusMDTest_lhs") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("RHSWorkspace", "MinusMDTest_rhs") );
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

    IMDIterator * it = ws->createIterator();
    while (it->next())
    {
      // Signal of all boxes is zero since they got subtracted
      TS_ASSERT_DELTA( it->getSignal(), 0.0, 1e-5);
      // But errors are not zero, since they get summed
      TS_ASSERT_LESS_THAN( 0, it->getError());
    }

    TSM_ASSERT("If the workspace is file-backed, then it needs updating.", ws->fileNeedsUpdating() );
    //cleanup
    std::string realFile;
    if ((inPlace==1)&&rhs->isFileBacked())
    {
        realFile = rhs->getBoxController()->getFileIO()->getFileName();
        rhs->clearFileBacked(false);
        MDEventsTestHelper::checkAndDeleteFile(realFile);
    }
    if ((inPlace==2)&&lhs->isFileBacked())
    {
        realFile = lhs->getBoxController()->getFileIO()->getFileName();
        lhs->clearFileBacked(false);
        MDEventsTestHelper::checkAndDeleteFile(realFile);
    }
    if (ws->isFileBacked())
    {
        realFile = ws->getBoxController()->getFileIO()->getFileName();
        ws->clearFileBacked(false);
        MDEventsTestHelper::checkAndDeleteFile(realFile);
    }
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_mem_minus_mem()
  { do_test(false, false, 0); }

  void test_mem_minus_mem_inPlace()
  { do_test(false, false, 1); }

  void test_file_minus_mem()
  { do_test(true, false, 0); }

  void test_file_minus_mem_inPlace()
  { do_test(true, false, 1); }

  void test_mem_minus_file_inPlace()
  { do_test(false, true, 1); }

  void test_file_minus_file()
  { do_test(true, true, 0); }

  void test_file_minus_file_inPlace()
  { do_test(true, true, 1); }



};


#endif /* MANTID_MDALGORITHMS_MINUSMDTEST_H_ */
