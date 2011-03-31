#ifndef MANTID_ALGORITHMS_CHANGEPULSETIMETEST_H_
#define MANTID_ALGORITHMS_CHANGEPULSETIMETEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/ChangePulsetime.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class ChangePulsetimeTest : public CxxTest::TestSuite
{
public:

    
  void test_Init()
  {
    ChangePulsetime alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void do_test(std::string in_ws_name, std::string out_ws_name, std::string WorkspaceIndexList)
  {
    ChangePulsetime alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )

    EventWorkspace_sptr in_ws, out_ws;
    in_ws = WorkspaceCreationHelper::CreateEventWorkspace2(100, 100);
    AnalysisDataService::Instance().addOrReplace(in_ws_name, in_ws);

    alg.setPropertyValue("InputWorkspace", in_ws_name);
    alg.setPropertyValue("OutputWorkspace", out_ws_name);
    alg.setPropertyValue("WorkspaceIndexList", WorkspaceIndexList);
    alg.setPropertyValue("TimeOffset", "1000.0");

    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    TS_ASSERT_THROWS_NOTHING( out_ws = boost::dynamic_pointer_cast<EventWorkspace>(
        AnalysisDataService::Instance().retrieve(out_ws_name) ));
    TS_ASSERT( out_ws );
    if (!out_ws) return;

    for (size_t wi=10; wi < 20; wi++)
    {
      double secs;
      secs = DateAndTime::seconds_from_duration(out_ws->getEventList(wi).getEvent(0).pulseTime() - DateAndTime("2010-01-01"));
      TS_ASSERT_DELTA(secs, 1000.0, 1e-5);
      secs = DateAndTime::seconds_from_duration(out_ws->getEventList(wi).getEvent(2).pulseTime() - DateAndTime("2010-01-01"));
      TS_ASSERT_DELTA(secs, 1001.0, 1e-5);
    }

    // If only modifying SOME spectra, check that the others did not change
    if (WorkspaceIndexList != "")
    {
      double secs;
      secs = DateAndTime::seconds_from_duration(out_ws->getEventList(0).getEvent(2).pulseTime() - DateAndTime("2010-01-01"));
      TS_ASSERT_DELTA(secs, 1.0, 1e-5);
      secs = DateAndTime::seconds_from_duration(out_ws->getEventList(30).getEvent(2).pulseTime() - DateAndTime("2010-01-01"));
      TS_ASSERT_DELTA(secs, 1.0, 1e-5);
    }

    // If not inplace, then the original did not change
    if (in_ws_name != out_ws_name)
    {
      double secs;
      secs = DateAndTime::seconds_from_duration(in_ws->getEventList(0).getEvent(2).pulseTime() - DateAndTime("2010-01-01"));
      TS_ASSERT_DELTA(secs, 1.0, 1e-5);
    }


    AnalysisDataService::Instance().remove(in_ws_name);
    AnalysisDataService::Instance().remove(out_ws_name);

  }

  void test_exec_allSpectra_copying_the_workspace()
  {
    do_test("ChangePulsetimeTest_ws", "ChangePulsetimeTest_out_ws", "");
  }

  void test_exec_allSpectra_inplace()
  {
    do_test("ChangePulsetimeTest_ws", "ChangePulsetimeTest_ws", "");
  }

  void test_exec_someSpectra_copying_the_workspace()
  {
    do_test("ChangePulsetimeTest_ws", "ChangePulsetimeTest_out_ws", "10-20");
  }

  void test_exec_someSpectra_inplace()
  {
    do_test("ChangePulsetimeTest_ws", "ChangePulsetimeTest_ws", "10-20");
  }



};


#endif /* MANTID_ALGORITHMS_CHANGEPULSETIMETEST_H_ */

