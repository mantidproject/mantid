#ifndef MANTID_ALGORITHMS_FILTEREVENTSTEST_H_
#define MANTID_ALGORITHMS_FILTEREVENTSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/FilterEvents.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSplitter.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

using namespace std;

class FilterEventsTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FilterEventsTest *createSuite() { return new FilterEventsTest(); }
  static void destroySuite( FilterEventsTest *suite ) { delete suite; }


  //----------------------------------------------------------------------------------------------
  /** Test initialization
    */
  void test_Initialization()
  {
    FilterEvents alg;
    alg.initialize();

    TS_ASSERT(alg.isInitialized());
  }

  //----------------------------------------------------------------------------------------------
  /** Test create event workspace and splitters
    * In all the tests below:
    * (1) 10 detectors
    * (2) Run starts @ 20000000000 seconds
    * (3) Pulse length = 100*1000*1000 seconds
    * (4) Within one pulse, two consecutive events/neutrons is apart for 10*1000*1000 seconds
    * (5) "Experiment": 5 pulse times.  10 events in each pulse
    */
  void test_CreatedEventWorskpaceAndSplitter()
  {
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100*1000*1000;
    int64_t tofdt = 10*1000*1000;
    size_t numpulses = 5;

    DataObjects::EventWorkspace_sptr eventws = createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);
    //eventws->setName("Test01");

    TS_ASSERT_EQUALS(eventws->getNumberEvents(), 500);

    DataObjects::EventList elist = eventws->getEventList(0);
    TS_ASSERT_EQUALS(elist.getNumberEvents(), 50);
    TS_ASSERT(elist.hasDetectorID(1));

    DataObjects::SplittersWorkspace_sptr splittersws = createSplitter(runstart_i64, pulsedt, tofdt);
    TS_ASSERT_EQUALS(splittersws->getNumberSplitters(), 5);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /**  Filter events without any correction
    *  Event workspace:
    * (1) 10 detectors
    * (2) Run starts @ 20000000000 seconds
    * (3) Pulse length = 100*1000*1000 seconds
    * (4) Within one pulse, two consecutive events/neutrons is apart for 10*1000*1000 seconds
    * (5) "Experiment": 5 pulse times.  10 events in each pulse
    *
    * In this test
   *  (1) Leave correction table workspace empty
   *  (2) Count events in each output including "-1", the excluded/unselected events
   */
  void test_FilterWOCorrection()
  {
    // 1. Create EventWorkspace and SplittersWorkspace    
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100*1000*1000;
    int64_t tofdt = 10*1000*1000;
    size_t numpulses = 5;

    DataObjects::EventWorkspace_sptr inpWS = createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("Test02", inpWS);

    DataObjects::SplittersWorkspace_sptr splws = createSplitter(runstart_i64, pulsedt, tofdt);
    AnalysisDataService::Instance().addOrReplace("Splitter02", splws);

    FilterEvents filter;
    filter.initialize();

    // 2. Set properties
    filter.setProperty("InputWorkspace", "Test02");
    filter.setProperty("OutputWorkspaceBaseName", "FilteredWS01");
    filter.setProperty("SplitterWorkspace", "Splitter02");

    // 3. Execute
    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    // 4. Get output
    int numsplittedws = filter.getProperty("NumberOutputWorkspace");
    TS_ASSERT_EQUALS(numsplittedws, 4);

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

    DataObjects::EventList elist3 = filteredws2->getEventList(3);
    elist3.sortPulseTimeTOF();

    DataObjects::TofEvent eventmin = elist3.getEvent(0);
    TS_ASSERT_EQUALS(eventmin.pulseTime().totalNanoseconds(), runstart_i64+pulsedt*2);
    TS_ASSERT_DELTA(eventmin.tof(), 0, 1.0E-4);

    DataObjects::TofEvent eventmax = elist3.getEvent(20);
    TS_ASSERT_EQUALS(eventmax.pulseTime().totalNanoseconds(), runstart_i64+pulsedt*4);
    TS_ASSERT_DELTA(eventmax.tof(), static_cast<double>(tofdt*6/1000), 1.0E-4);

    // 5. Clean up
    AnalysisDataService::Instance().remove("Test02");
    AnalysisDataService::Instance().remove("Splitter02");
    AnalysisDataService::Instance().remove("FilteredWS01_unfiltered");
    AnalysisDataService::Instance().remove("FilteredWS01_0");
    AnalysisDataService::Instance().remove("FilteredWS01_1");
    AnalysisDataService::Instance().remove("FilteredWS01_2");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /**  Filter events without any correction and test for user-specified workspace starting value
    *  Event workspace:
    * (1) 10 detectors
    * (2) Run starts @ 20000000000 seconds
    * (3) Pulse length = 100*1000*1000 seconds
    * (4) Within one pulse, two consecutive events/neutrons is apart for 10*1000*1000 seconds
    * (5) "Experiment": 5 pulse times.  10 events in each pulse
    *
    * In this test
   *  (1) Leave correction table workspace empty
   *  (2) Count events in each output including "-1", the excluded/unselected events
   */
  void test_FilterWOCorrection2()
  {
    // Create EventWorkspace and SplittersWorkspace
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100*1000*1000;
    int64_t tofdt = 10*1000*1000;
    size_t numpulses = 5;

    DataObjects::EventWorkspace_sptr inpWS = createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("Test02", inpWS);

    DataObjects::SplittersWorkspace_sptr splws = createSplitter(runstart_i64, pulsedt, tofdt);
    AnalysisDataService::Instance().addOrReplace("Splitter02", splws);

    FilterEvents filter;
    filter.initialize();

    // Set properties
    filter.setProperty("InputWorkspace", "Test02");
    filter.setProperty("OutputWorkspaceBaseName", "FilteredWS01");
    filter.setProperty("SplitterWorkspace", "Splitter02");
    filter.setProperty("OutputWorkspaceIndexedFrom1", true);

    // Execute
    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    // Get output
    int numsplittedws = filter.getProperty("NumberOutputWorkspace");
    TS_ASSERT_EQUALS(numsplittedws, 3);

    // 4.1 Workspace group 0
    DataObjects::EventWorkspace_sptr filteredws0 = boost::dynamic_pointer_cast
        <DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS01_1"));
    TS_ASSERT(filteredws0);
    TS_ASSERT_EQUALS(filteredws0->getNumberHistograms(), 10);
    TS_ASSERT_EQUALS(filteredws0->getEventList(0).getNumberEvents(), 4);

    // 4.2 Workspace group 1
    DataObjects::EventWorkspace_sptr filteredws1 = boost::dynamic_pointer_cast
        <DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS01_2"));
    TS_ASSERT(filteredws1);
    TS_ASSERT_EQUALS(filteredws1->getEventList(1).getNumberEvents(), 16);

    // 4.3 Workspace group 2
    DataObjects::EventWorkspace_sptr filteredws2 = boost::dynamic_pointer_cast
        <DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS01_3"));
    TS_ASSERT(filteredws2);
    TS_ASSERT_EQUALS(filteredws2->getEventList(1).getNumberEvents(), 21);

    DataObjects::EventList elist3 = filteredws2->getEventList(3);
    elist3.sortPulseTimeTOF();

    DataObjects::TofEvent eventmin = elist3.getEvent(0);
    TS_ASSERT_EQUALS(eventmin.pulseTime().totalNanoseconds(), runstart_i64+pulsedt*2);
    TS_ASSERT_DELTA(eventmin.tof(), 0, 1.0E-4);

    DataObjects::TofEvent eventmax = elist3.getEvent(20);
    TS_ASSERT_EQUALS(eventmax.pulseTime().totalNanoseconds(), runstart_i64+pulsedt*4);
    TS_ASSERT_DELTA(eventmax.tof(), static_cast<double>(tofdt*6/1000), 1.0E-4);

    // 5. Clean up
    AnalysisDataService::Instance().remove("Test02");
    AnalysisDataService::Instance().remove("Splitter02");
    AnalysisDataService::Instance().remove("FilteredWS01_1");
    AnalysisDataService::Instance().remove("FilteredWS01_2");
    AnalysisDataService::Instance().remove("FilteredWS01_3");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /**  Filter test with TOF correction
    */
  void test_FilterWithCorrection()
  {
    // 1. Create EventWorkspace and SplittersWorkspace
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100*1000*1000;
    int64_t tofdt = 10*1000*1000;
    size_t numpulses = 5;

    DataObjects::EventWorkspace_sptr inpWS = createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("EventData", inpWS);

    DataObjects::SplittersWorkspace_sptr splws = createFastFreqLogSplitter(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("SplitterTableX", splws);
    TS_ASSERT_EQUALS(splws->rowCount(), static_cast<size_t>(numpulses)*2);

    TableWorkspace_sptr timecorrws = createTimeCorrectionTable(inpWS);
    AnalysisDataService::Instance().addOrReplace("TimeCorrectionTableX", timecorrws);
    TS_ASSERT_EQUALS(timecorrws->rowCount(), inpWS->getNumberHistograms());

    FilterEvents filter;
    filter.initialize();

    // 2. Set properties
    TS_ASSERT_THROWS_NOTHING(filter.setProperty("InputWorkspace", "EventData"));
    TS_ASSERT_THROWS_NOTHING(filter.setProperty("OutputWorkspaceBaseName", "SplittedDataX"));
    TS_ASSERT_THROWS_NOTHING(filter.setProperty("DetectorTOFCorrectionWorkspace", "TimeCorrectionTableX"));
    TS_ASSERT_THROWS_NOTHING(filter.setProperty("SplitterWorkspace", splws));

    // 3. Execute
    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    // 4. Get output
    // 4.1 Workspace group 0
    DataObjects::EventWorkspace_sptr filteredws0 = boost::dynamic_pointer_cast
        <DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve("SplittedDataX_0"));
    TS_ASSERT(filteredws0);
    TS_ASSERT_EQUALS(filteredws0->getNumberHistograms(), 10);
    TS_ASSERT_EQUALS(filteredws0->getEventList(0).getNumberEvents(), 15);
    TS_ASSERT_EQUALS(filteredws0->getEventList(9).getNumberEvents(), 15);

    // 4.2 Workspace group 1
    DataObjects::EventWorkspace_sptr filteredws1 = boost::dynamic_pointer_cast
        <DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve("SplittedDataX_1"));
    TS_ASSERT(filteredws1);
    TS_ASSERT_EQUALS(filteredws1->getEventList(1).getNumberEvents(), 10);

    // 4.3 Some individual events
    DataObjects::EventList elist3 = filteredws1->getEventList(3);
    elist3.sortPulseTimeTOF();

    DataObjects::TofEvent eventmin = elist3.getEvent(0);
    TS_ASSERT_EQUALS(eventmin.pulseTime().totalNanoseconds(), runstart_i64);
    TS_ASSERT_DELTA(eventmin.tof(), 80*1000, 1.0E-4);

    // 5. Clean
    AnalysisDataService::Instance().remove("EventData");
    AnalysisDataService::Instance().remove("TimeCorrectionTableX");
    AnalysisDataService::Instance().remove("SplitterTableX");
    AnalysisDataService::Instance().remove("SplittedDataX_0");
    AnalysisDataService::Instance().remove("SplittedDataX_1");

    return;
  }


  //----------------------------------------------------------------------------------------------
  /** Create an EventWorkspace.  This workspace has
    * @param runstart_i64 : absolute run start time in int64_t format with unit nanosecond
    * @param pulsedt : pulse length in int64_t format with unit nanosecond
    * @param todft : time interval between 2 adjacent event in same pulse in int64_t format of unit nanosecond
    * @param numpulses : number of pulses in the event workspace
   */
  DataObjects::EventWorkspace_sptr createEventWorkspace(int64_t runstart_i64, int64_t pulsedt, int64_t tofdt,
                                                        size_t numpulses)
  {
    // 1. Create an EventWorkspace with 10 detectors
    DataObjects::EventWorkspace_sptr eventWS =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(10, 1, true);

    Kernel::DateAndTime runstart(runstart_i64);

    // 2. Set run_start time
    eventWS->mutableRun().addProperty("run_start", runstart.toISO8601String(), true);

    for (size_t i = 0; i < eventWS->getNumberHistograms(); i ++)
    {
      DataObjects::EventList* elist = eventWS->getEventListPtr(i);

      for (int64_t pid = 0; pid < static_cast<int64_t>(numpulses); pid ++)
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


  //----------------------------------------------------------------------------------------------
  /** Create a  Splitter for output
   *  Region:
   * 0: pulse 0: 0 ~ 3+
   * 1: pulse 0: 3+ ~ pulse 1: 9+
   * 2: from pulse 2: 0 ~ 6+
   * -1: from pulse 2: 6+ ~ 9+
    * @param runstart_i64 : absolute run start time in int64_t format with unit nanosecond
    * @param pulsedt : pulse length in int64_t format with unit nanosecond
    * @param todft : time interval between 2 adjacent event in same pulse in int64_t format of unit nanosecond
    * @param numpulses : number of pulses in the event workspace
   */
  DataObjects::SplittersWorkspace_sptr createSplitter(int64_t runstart_i64, int64_t pulsedt, int64_t tofdt)
  {
    DataObjects::SplittersWorkspace_sptr splitterws =
        boost::shared_ptr<DataObjects::SplittersWorkspace>(new DataObjects::SplittersWorkspace);


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

  //----------------------------------------------------------------------------------------------
  /** Create a Splitter for fast fequency log for output
    * The splitter is within every pulse.  2 groups of splitters are created.  In each pulse
    * 1. group 0: 0.2 dT ~ 0.4 dT    dT = pulsedt
    * 2. group 1: 0.6 dT ~ 0.8 dT
    *
    * @param runstart_i64 : absolute run start time in int64_t format with unit nanosecond
    * @param pulsedt : pulse length in int64_t format with unit nanosecond
    * @param todft : time interval between 2 adjacent event in same pulse in int64_t format of unit nanosecond
    * @param numpulses : number of pulses in the event workspace
   */
  SplittersWorkspace_sptr createFastFreqLogSplitter(int64_t runstart_i64, int64_t pulsedt,
                                                    int64_t tofdt, size_t numpulses)
  {

    UNUSED_ARG(tofdt);

    // 1. Create an empty splitter workspace
    SplittersWorkspace_sptr splitterws =
        boost::shared_ptr<DataObjects::SplittersWorkspace>(new DataObjects::SplittersWorkspace);

    // 2. Create splitters
    for (size_t i = 0; i < numpulses; ++i)
    {
      int64_t t0a = runstart_i64 + static_cast<int64_t>(i)*pulsedt + static_cast<int64_t>(static_cast<double>(pulsedt)*0.2);
      int64_t tfa = runstart_i64 + static_cast<int64_t>(i)*pulsedt + static_cast<int64_t>(static_cast<double>(pulsedt)*0.4);
      Kernel::SplittingInterval interval_a(t0a, tfa, 0);


      int64_t t0b = runstart_i64 + static_cast<int64_t>(i)*pulsedt + static_cast<int64_t>(static_cast<double>(pulsedt)*0.6);
      int64_t tfb = runstart_i64 + static_cast<int64_t>(i)*pulsedt + static_cast<int64_t>(static_cast<double>(pulsedt)*0.8);
      Kernel::SplittingInterval interval_b(t0b, tfb, 1);

      splitterws->addSplitter(interval_a);
      splitterws->addSplitter(interval_b);
    }

    return splitterws;
  }


  //----------------------------------------------------------------------------------------------
  /** Create the time correction table
    */
  TableWorkspace_sptr createTimeCorrectionTable(MatrixWorkspace_sptr inpws)
  {
    // 1. Generate an empty table
    TableWorkspace_sptr corrtable(new TableWorkspace());
    corrtable->addColumn("int", "DetectorID");
    corrtable->addColumn("double", "Correction");

    // 2. Add rows
    Instrument_const_sptr instrument = inpws->getInstrument();
    vector<int> detids = instrument->getDetectorIDs();
    for (size_t i = 0; i < detids.size(); ++i)
    {
      int detid = detids[i];
      double factor = 0.75;
      TableRow newrow = corrtable->appendRow();
      newrow << detid << factor;
    }

    return corrtable;
  }


};


#endif /* MANTID_ALGORITHMS_FILTEREVENTSTEST_H_ */
































