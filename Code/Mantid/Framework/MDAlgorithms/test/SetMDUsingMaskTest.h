#ifndef MANTID_MDALGORITHMS_SETMDUSINGMASKTEST_H_
#define MANTID_MDALGORITHMS_SETMDUSINGMASKTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidMDAlgorithms/SetMDUsingMask.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

class SetMDUsingMaskTest : public CxxTest::TestSuite
{
public:
  void test_Init()
  {
    SetMDUsingMask alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void do_test(std::string InputWorkspace, std::string MaskWorkspace,
      std::string ValueWorkspace, std::string Value,
      std::string OutputWorkspace,
      double expectedSignal, double expectedError,
      bool succeeds=true)
  {
    MDHistoWorkspace_sptr histo_A = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.0, 2, 5, 10.0, 2.0);
    MDHistoWorkspace_sptr histo_B = MDEventsTestHelper::makeFakeMDHistoWorkspace(3.0, 2, 5, 10.0, 3.0);
    MDHistoWorkspace_sptr histo_diff = MDEventsTestHelper::makeFakeMDHistoWorkspace(2.0, 2, 4, 10.0, 2.0);
    MDHistoWorkspace_sptr mask_0  = MDEventsTestHelper::makeFakeMDHistoWorkspace(0.0, 2, 5, 10.0, 0.0);
    MDHistoWorkspace_sptr mask_1  = MDEventsTestHelper::makeFakeMDHistoWorkspace(1.0, 2, 5, 10.0, 0.0);
    AnalysisDataService::Instance().addOrReplace("histo_A", histo_A);
    AnalysisDataService::Instance().addOrReplace("histo_B", histo_B);
    AnalysisDataService::Instance().addOrReplace("histo_diff", histo_diff);
    AnalysisDataService::Instance().addOrReplace("mask_0", mask_0);
    AnalysisDataService::Instance().addOrReplace("mask_1", mask_1);


    SetMDUsingMask alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", InputWorkspace) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("MaskWorkspace", MaskWorkspace) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("ValueWorkspace", ValueWorkspace) );
    if (!Value.empty()) TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("Value", Value) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", OutputWorkspace) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );

    if (succeeds)
    {
      TS_ASSERT( alg.isExecuted() );
      // Retrieve the workspace from data service.
      IMDHistoWorkspace_sptr ws;
      TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<IMDHistoWorkspace>(OutputWorkspace) );
      TS_ASSERT(ws);
      if (!ws) return;
      TS_ASSERT_DELTA( ws->signalAt(0), expectedSignal, 1e-6);
      TS_ASSERT_DELTA( ws->errorSquaredAt(0), expectedError, 1e-6);
    }
    else
    {
      TS_ASSERT( !alg.isExecuted() );
    }
  }


  void test_bad_inputs()
  {
    do_test("histo_A", "histo_diff", "histo_B", "", "out", 0,0, false);
    do_test("histo_A", "mask_1", "histo_diff", "", "out", 0,0, false);
    do_test("histo_A", "histo_diff", "histo_diff", "", "out", 0,0, false);
  }

  void test_not_inplace()
  {
    do_test("histo_A", "mask_0", "histo_B", "", "out", 2.0, 2.0);
    do_test("histo_A", "mask_1", "histo_B", "", "out", 3.0, 3.0);
  }

  void test_not_inplace_double()
  {
    do_test("histo_A", "mask_0", "", "34.5", "out", 2.0, 2.0);
    do_test("histo_A", "mask_1", "", "34.5", "out", 34.5, 0.);
  }

  void test_inplace()
  {
    do_test("histo_A", "mask_0", "histo_B", "", "histo_A", 2.0, 2.0);
    do_test("histo_A", "mask_1", "histo_B", "", "histo_A", 3.0, 3.0);
  }

  void test_inplace_double()
  {
    do_test("histo_A", "mask_0", "", "34.5", "histo_A", 2.0, 2.0);
    do_test("histo_A", "mask_1", "", "34.5", "histo_A", 34.5, 0.);
  }

};


#endif /* MANTID_MDALGORITHMS_SETMDUSINGMASKTEST_H_ */
