#ifndef MANTID_ALGORITHMS_FILTEREVENTSTEST_H_
#define MANTID_ALGORITHMS_FILTEREVENTSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/FilterEvents.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidKernel/TimeSplitter.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class FilterEventsTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FilterEventsTest *createSuite() { return new FilterEventsTest(); }
  static void destroySuite( FilterEventsTest *suite ) { delete suite; }


  void test_Initialization()
  {
    FilterEvents alg;
    alg.initialize();

    TS_ASSERT(alg.isInitialized());
  }

  void test_CreatedEventWorskpaceAndSplitter()
  {
    DataObjects::EventWorkspace_sptr eventws = createEventWorkspace();
    eventws->setName("Test01");

    TS_ASSERT_EQUALS(eventws->getNumberEvents(), 500);

    DataObjects::EventList elist = eventws->getEventList(0);
    TS_ASSERT_EQUALS(elist.getNumberEvents(), 50);
    TS_ASSERT(elist.hasDetectorID(1));

    DataObjects::SplittersWorkspace_sptr splittersws = createSplitter();
    TS_ASSERT_EQUALS(splittersws->getNumberSplitters(), 5);

    return;
  }

  /*
   *  Filter events without any correction
   *  (1) Leave correction file empty
   *  (2) Count events in each output including "-1", the excluded/unselected events
   */
  void test_FilterWOCorrection()
  {
    // 1. Create EventWorkspace and SplittersWorkspace
    DataObjects::EventWorkspace_sptr inpWS = createEventWorkspace();
    inpWS->setName("Test02");
    inpWS->setTitle("Test02");

    DataObjects::SplittersWorkspace_sptr splws = createSplitter();
    splws->setName("Splitter02");
    splws->setTitle("Splitter02");

    FilterEvents filter;
    filter.initialize();

    // 2. Set properties
    filter.setProperty("InputWorkspace", inpWS);
    filter.setProperty("OutputWorkspaceBaseName", "FilteredWS01");
    filter.setProperty("InputSplittersWorkspace", splws);
    filter.setProperty("DetectorCalibrationFile", "");

    // 3. Execute
    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    // 4. Get output
    // 4.1 Workspace group 0
    DataObjects::EventWorkspace_sptr filteredws0 = boost::dynamic_pointer_cast
        <DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS01_0"));
    TS_ASSERT(filteredws0);
    TS_ASSERT_EQUALS(filteredws0->getNumberHistograms(), 10);
    TS_ASSERT_EQUALS(filteredws0->getEventList(0).getNumberEvents(), 4);

    // 4.2 Workspace group 1
    DataObjects::EventWorkspace_sptr filteredws1 = boost::dynamic_pointer_cast
        <DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS01_1"));
    TS_ASSERT(filteredws1);
    TS_ASSERT_EQUALS(filteredws1->getEventList(1).getNumberEvents(), 16);

    // 4.3 Workspace group 2
    DataObjects::EventWorkspace_sptr filteredws2 = boost::dynamic_pointer_cast
        <DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS01_2"));
    TS_ASSERT(filteredws2);
    TS_ASSERT_EQUALS(filteredws2->getEventList(1).getNumberEvents(), 21);

    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100*1000*1000;
    int64_t tofdt = 10*1000*1000;

    DataObjects::EventList elist3 = filteredws2->getEventList(3);
    elist3.sortPulseTimeTOF();

    DataObjects::TofEvent eventmin = elist3.getEvent(0);
    TS_ASSERT_EQUALS(eventmin.pulseTime().totalNanoseconds(), runstart_i64+pulsedt*2);
    TS_ASSERT_DELTA(eventmin.tof(), 0, 1.0E-4);

    DataObjects::TofEvent eventmax = elist3.getEvent(20);
    TS_ASSERT_EQUALS(eventmax.pulseTime().totalNanoseconds(), runstart_i64+pulsedt*4);
    TS_ASSERT_DELTA(eventmax.tof(), static_cast<double>(tofdt*6/1000), 1.0E-4);

    return;
  }

  void test_FilterWithCorrection()
  {

  }


  /*
   * Create an EventWorkspace.  This workspace will have
   * (1) Events with wall time even in time
   */
  DataObjects::EventWorkspace_sptr createEventWorkspace()
  {
    // 1. Create an EventWorkspace with 10 detectors
    DataObjects::EventWorkspace_sptr eventWS = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(10, 1, true);

    int64_t runstart_i64 = 20000000000;
    Kernel::DateAndTime runstart(runstart_i64);
    int64_t pulsedt = 100*1000*1000;
    int64_t tofdt = 10*1000*1000;

    // 2. Set run_start time
    eventWS->mutableRun().addProperty("run_start", runstart.toISO8601String(), true);

    for (size_t i = 0; i < eventWS->getNumberHistograms(); i ++)
    {
      DataObjects::EventList* elist = eventWS->getEventListPtr(i);

      for (int64_t pid = 0; pid < 5; pid ++)
      {
        int64_t pulsetime_i64 = pid*pulsedt+runstart.totalNanoseconds();
        Kernel::DateAndTime pulsetime(pulsetime_i64);
        for (size_t e = 0; e < 10; e ++)
        {
          double tof = static_cast<double>(e*tofdt/1000);
          DataObjects::TofEvent event(tof, pulsetime);
          elist->addEventQuickly(event);
        }
      } // FOR each pulse
    } // For each bank

    return eventWS;
  }


  /*
   * Create a  Splitter for output
   * Region:
   * 0: pulse 0: 0 ~ 3+
   * 1: pulse 0: 3+ ~ pulse 1: 9+
   * 2: from pulse 2: 0 ~ 6+
   * -1: from pulse 2: 6+ ~ 9+
   */
  DataObjects::SplittersWorkspace_sptr createSplitter()
  {
    DataObjects::SplittersWorkspace_sptr splitterws = boost::shared_ptr<DataObjects::SplittersWorkspace>(new DataObjects::SplittersWorkspace);

    int64_t runstart_i64 = 20000000000;

    int64_t pulsedt = 100*1000*1000;
    int64_t tofdt = 10*1000*1000;

    // 1. Splitter 0: 0 ~ 3+ (first pulse)
    int64_t t0 = runstart_i64;
    int64_t t1 = t0 + tofdt*3 + tofdt/2;
    Kernel::SplittingInterval interval0(t0, t1, 0);
    splitterws->addSplitter(interval0);

    // 2. Splitter 1: 3+ ~ 9+ (second pulse)
    t0 = t1;
    t1 = runstart_i64 + pulsedt + tofdt*9 + tofdt/2;
    Kernel::SplittingInterval interval1(t0, t1, 1);
    splitterws->addSplitter(interval1);

    // 3. Splitter 2: from 3rd pulse, 0 ~ 6+
    for (size_t i = 2; i < 5; i ++)
    {
      t0 = runstart_i64+i*pulsedt;
      t1 = runstart_i64+i*pulsedt+6*tofdt+tofdt/2;
      Kernel::SplittingInterval interval2(t0, t1, 2);
      splitterws->addSplitter(interval2);
    }

    return splitterws;
  }


};


#endif /* MANTID_ALGORITHMS_FILTEREVENTSTEST_H_ */
