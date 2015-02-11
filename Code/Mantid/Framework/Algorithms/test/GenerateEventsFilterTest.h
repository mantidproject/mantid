#ifndef MANTID_ALGORITHMS_GENERATEEVENTSFILTERTEST_H_
#define MANTID_ALGORITHMS_GENERATEEVENTSFILTERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <cmath>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <Poco/File.h>

#include "MantidAlgorithms/GenerateEventsFilter.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidKernel/TimeSplitter.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/Column.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/Events.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

using namespace std;

class GenerateEventsFilterTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GenerateEventsFilterTest *createSuite() { return new GenerateEventsFilterTest(); }
  static void destroySuite( GenerateEventsFilterTest *suite ) { delete suite; }


  void test_Init()
  {
    GenerateEventsFilter alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test generation of splitters by time
   */
  void test_genTime1Interval()
  {
    // 1. Create input Workspace
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspace();
    AnalysisDataService::Instance().addOrReplace("TestWorkspace", eventWS);

    // 2. Init and set property
    GenerateEventsFilter alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", eventWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "Splitters01"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InformationWorkspace", "SplittersInformation"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartTime", "100"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StopTime", "1000000"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UnitOfTime", "Nanoseconds"));

    // 3. Running and get result
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 4. Check output
    DataObjects::SplittersWorkspace_sptr splittersws =
        boost::dynamic_pointer_cast<DataObjects::SplittersWorkspace>(AnalysisDataService::Instance().retrieve("Splitters01"));
    DataObjects::TableWorkspace_sptr splittersinfo =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve("SplittersInformation"));

    TS_ASSERT(splittersws);

    TS_ASSERT_EQUALS(splittersws->getNumberSplitters(), 1);
    Kernel::SplittingInterval splitter0 = splittersws->getSplitter(0);
    Kernel::DateAndTime runstart(3000000000);
    TS_ASSERT_EQUALS(splitter0.start().totalNanoseconds(), runstart.totalNanoseconds()+100);
    TS_ASSERT_EQUALS(splitter0.stop().totalNanoseconds(), runstart.totalNanoseconds()+1000000);
    TS_ASSERT_EQUALS(splitter0.index(), 0);

    TS_ASSERT_EQUALS(splittersws->rowCount(), 1);
    TS_ASSERT_EQUALS(splittersinfo->rowCount(), 1);

    AnalysisDataService::Instance().remove("Splitters01");
    AnalysisDataService::Instance().remove("SplittersInformation");
    AnalysisDataService::Instance().remove("TestWorkspace");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test generation of splitters by time
   * (1) Multiple time interval
   * (2) Default start time and stop time
   */
  void test_genTimeMultipleInterval()
  {
    // Create input Workspace
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspace();
    for (size_t i = 0; i < eventWS->getNumberHistograms(); ++i)
      std::cout << "Spectrum " << i << ": max pulse time = " << eventWS->getEventList(i).getPulseTimeMax()
                << " = " << eventWS->getEventList(i).getPulseTimeMin().totalNanoseconds() << "\n";

    int64_t timeinterval_ns = 15000;

    // 2. Init and set property
    GenerateEventsFilter alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", eventWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "Splitters01"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InformationWorkspace", "InfoWS");)
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeInterval", 15000.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UnitOfTime", "Nanoseconds"));

    // 3. Running and get result
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 4. Check output
    DataObjects::SplittersWorkspace_sptr splittersws =
        boost::dynamic_pointer_cast<DataObjects::SplittersWorkspace>(AnalysisDataService::Instance().retrieve("Splitters01"));

    TS_ASSERT(splittersws);

    size_t numintervals = 74;
    TS_ASSERT_EQUALS(splittersws->getNumberSplitters(), numintervals);

    std::string runstarttimestr = eventWS->run().getProperty("run_start")->value();
    Kernel::DateAndTime runstarttime(runstarttimestr);
    int64_t runstarttime_ns = runstarttime.totalNanoseconds();

    Kernel::TimeSeriesProperty<double> *protonchargelog =
        dynamic_cast<Kernel::TimeSeriesProperty<double>* >(eventWS->run().getProperty("proton_charge"));
    Kernel::DateAndTime runstoptime = Kernel::DateAndTime(protonchargelog->lastTime().totalNanoseconds() + 100000);

    // b) First interval
    Kernel::SplittingInterval splitter0 = splittersws->getSplitter(0);
    TS_ASSERT_EQUALS(splitter0.start().totalNanoseconds(), 0+runstarttime_ns);
    TS_ASSERT_EQUALS(splitter0.stop().totalNanoseconds(), timeinterval_ns+runstarttime_ns);
    TS_ASSERT_EQUALS(splitter0.index(), 0);

    // c) Last interval
    Kernel::SplittingInterval splitterf = splittersws->getSplitter(numintervals-1);
    TS_ASSERT_EQUALS(splitterf.stop(), runstoptime);
    TS_ASSERT_EQUALS(splitterf.index(), numintervals-1);

    // d) Randomly
    Kernel::SplittingInterval splitterR = splittersws->getSplitter(40);
    Kernel::DateAndTime t0 = splitterR.start();
    Kernel::DateAndTime tf = splitterR.stop();
    int64_t dt_ns = tf.totalNanoseconds()-t0.totalNanoseconds();
    TS_ASSERT_EQUALS(dt_ns, timeinterval_ns);
    int64_t dt_runtimestart = t0.totalNanoseconds()-runstarttime_ns;
    TS_ASSERT_EQUALS(dt_runtimestart, 40*timeinterval_ns);

    // 5. Clean
    AnalysisDataService::Instance().remove("Splitters01");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate filter by log value in simple way
   * (1) No time tolerance
   * (2) Just one region
   */
  void test_genSimpleLogValueFilter()
  {
    // Create input
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspace();

    // Init and set property
    GenerateEventsFilter alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", eventWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "Splitters03"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogName", "FastSineLog"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MinimumLogValue", "-0.25"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaximumLogValue",  "0.25"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FilterLogValueByChangingDirection", "Increase"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeTolerance", 1.0E-8));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogBoundary",  "Centre"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InformationWorkspace", "Information"));

    // 3. Running and get result
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 4. Check output
     DataObjects::SplittersWorkspace_sptr splittersws =
         boost::dynamic_pointer_cast<DataObjects::SplittersWorkspace>(AnalysisDataService::Instance().retrieve("Splitters03"));
     TS_ASSERT(splittersws);

     DataObjects::TableWorkspace_const_sptr infows =
         boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve("Information"));
     TS_ASSERT(infows);

     // 5. Check
     size_t numsplitters = 2;
     TS_ASSERT_EQUALS(splittersws->getNumberSplitters(), numsplitters);

     Kernel::SplittingInterval s0 = splittersws->getSplitter(0);
     TS_ASSERT_EQUALS(s0.start().totalNanoseconds(), 3000000000-static_cast<int64_t>(1.0E-8*1.0E9));
     TS_ASSERT_EQUALS(s0.stop().totalNanoseconds(), 3000050000-static_cast<int64_t>(1.0E-8*1.0E9));

     Kernel::SplittingInterval s1 = splittersws->getSplitter(1);
     TS_ASSERT_EQUALS(s1.start().totalNanoseconds(), 3000775000-static_cast<int64_t>(1.0E-8*1.0E9));
     TS_ASSERT_EQUALS(s1.stop().totalNanoseconds(), 3000850000-static_cast<int64_t>(1.0E-8*1.0E9));

     TS_ASSERT_EQUALS(infows->rowCount(), 1);

     // 6. Clean
     AnalysisDataService::Instance().remove("Splitters03");
     AnalysisDataService::Instance().remove("Information");

     return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate filter by log values in increasing
   * (1) No time tolerance
   * (2) Just one
   */
  void test_genMultipleLogValuesFilter()
  {
    // Create input
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspace();

    // Init and set property
    GenerateEventsFilter alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", eventWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "Splitters04"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InformationWorkspace", "Information"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogName", "FastSineLog"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MinimumLogValue", "-1.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaximumLogValue",  "1.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogValueInterval", 0.2));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogValueTolerance", 0.05));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FilterLogValueByChangingDirection", "Increase"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeTolerance", 1.0E-8));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogBoundary",  "Centre"));

    // Running and get result
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Check output
     DataObjects::SplittersWorkspace_sptr splittersws =
         boost::dynamic_pointer_cast<DataObjects::SplittersWorkspace>(AnalysisDataService::Instance().retrieve("Splitters04"));
     TS_ASSERT(splittersws);

     DataObjects::TableWorkspace_const_sptr infows =
         boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve("Information"));
     TS_ASSERT(infows);

     // Check
     size_t numsplitters = 16;
     TS_ASSERT_EQUALS(splittersws->getNumberSplitters(), numsplitters);
     size_t numoutputs = 11;
     TS_ASSERT_EQUALS(infows->rowCount(), numoutputs);

     Kernel::SplittingInterval s0 = splittersws->getSplitter(0);
     TS_ASSERT_EQUALS(s0.start(), 3000000000-static_cast<int>(1.0E-8*1.0E9));
     TS_ASSERT_EQUALS(s0.index(), 5);

     Kernel::SplittingInterval s15 = splittersws->getSplitter(15);
     TS_ASSERT_EQUALS(s15.start(), 3000924990);
     TS_ASSERT_EQUALS(s15.stop(),  3000974990);
     TS_ASSERT_EQUALS(s15.index(), 9);

  }

  //----------------------------------------------------------------------------------------------
  /** Test to generate a set of filters against an integer log
    */
  void test_genFilterByIntegerLog()
  {
    // 1. Create input
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspaceIntLog();
    AnalysisDataService::Instance().addOrReplace("TestEventData2", eventWS);

    // 2. Init and set property
    GenerateEventsFilter alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "TestEventData2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "IntLogSplitter"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InformationWorkspace", "IntLogInformation"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogName", "DummyIntLog"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MinimumLogValue", static_cast<double>(1)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaximumLogValue", static_cast<double>(10)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogValueInterval", static_cast<double>(1)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UnitOfTime", "Seconds"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FilterLogValueByChangingDirection", "Both"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeTolerance", 0.05));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogBoundary",  "Centre"));

    // 3. Running and get result
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 4. Retrieve output workspaces
    SplittersWorkspace_sptr splittersws = boost::dynamic_pointer_cast<SplittersWorkspace>(
          AnalysisDataService::Instance().retrieve("IntLogSplitter"));
    TS_ASSERT(splittersws);

    TableWorkspace_const_sptr infows = boost::dynamic_pointer_cast<TableWorkspace>(
          AnalysisDataService::Instance().retrieve("IntLogInformation"));
    TS_ASSERT(infows);

     // 5. Check output workspace
     size_t numsplitters = 10;
     TS_ASSERT_EQUALS(splittersws->getNumberSplitters(), numsplitters);
     size_t numoutputs = 10;
     TS_ASSERT_EQUALS(infows->rowCount(), numoutputs);

     int64_t factor = static_cast<int64_t>(1.0E9+0.5);

     Kernel::SplittingInterval s0 = splittersws->getSplitter(0);
     TS_ASSERT_EQUALS(s0.start().totalNanoseconds(), 11*factor-5*factor/100);
     TS_ASSERT_EQUALS(s0.index(), 0);

     Kernel::SplittingInterval s9 = splittersws->getSplitter(9);
     // TS_ASSERT_EQUALS(s14.start(), 3000924990);
     // TS_ASSERT_EQUALS(s14.stop(),  3000974990);
     TS_ASSERT_EQUALS(s9.index(), 9);

     // 6. Clean
     AnalysisDataService::Instance().remove("TestEventData2");
     AnalysisDataService::Instance().remove("IntLogSplitter");
     AnalysisDataService::Instance().remove("IntLogInformation");
  }

  //----------------------------------------------------------------------------------------------
  /** Test to generate a set of filters against an integer log by using the single value mode
    */
  void test_genFilterByIntegerLog2()
  {
    // Create input
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspaceIntLog();
    AnalysisDataService::Instance().addOrReplace("TestEventData2", eventWS);

    // Initialize and set property
    GenerateEventsFilter alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "TestEventData2"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "IntLogSplitter"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InformationWorkspace", "IntLogInformation"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogName", "DummyIntLog"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MinimumLogValue", static_cast<double>(1)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaximumLogValue", static_cast<double>(2)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UnitOfTime", "Seconds"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FilterLogValueByChangingDirection", "Both"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeTolerance", 0.05));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogBoundary",  "Centre"));

    // Running and get result
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Retrieve output workspaces
    SplittersWorkspace_sptr splittersws = boost::dynamic_pointer_cast<SplittersWorkspace>(
          AnalysisDataService::Instance().retrieve("IntLogSplitter"));
    TS_ASSERT(splittersws);

    TableWorkspace_const_sptr infows = boost::dynamic_pointer_cast<TableWorkspace>(
          AnalysisDataService::Instance().retrieve("IntLogInformation"));
    TS_ASSERT(infows);

     // Check output workspace
     size_t numsplitters = 1;
     TS_ASSERT_EQUALS(splittersws->getNumberSplitters(), numsplitters);
     size_t numoutputs = 1;
     TS_ASSERT_EQUALS(infows->rowCount(), numoutputs);

     int64_t factor = static_cast<int64_t>(1.0E9+0.5);

     Kernel::SplittingInterval s0 = splittersws->getSplitter(0);
     TS_ASSERT_EQUALS(s0.start().totalNanoseconds(), 11*factor-5*factor/100);
     TS_ASSERT_EQUALS(s0.index(), 0);

     // Clean
     AnalysisDataService::Instance().remove("TestEventData2");
     AnalysisDataService::Instance().remove("IntLogSplitter");
     AnalysisDataService::Instance().remove("IntLogInformation");
  }


  //----------------------------------------------------------------------------------------------
  /** Create an EventWorkspace including
   * (1) proton charge log from
   * (2) test log in sin function with time
   */
  DataObjects::EventWorkspace_sptr createEventWorkspace()
  {
    using namespace WorkspaceCreationHelper;

    // Empty workspace
    DataObjects::EventWorkspace_sptr eventws =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(2, 2, true);

    // Run star time
    int64_t runstarttime_ns = 3000000000;
    int64_t runstoptime_ns  = 3001000000;
    int64_t pulsetime_ns    =     100000;

    Kernel::DateAndTime runstarttime(runstarttime_ns);
    eventws->mutableRun().addProperty("run_start", runstarttime.toISO8601String());

    // Proton charge log
    Kernel::TimeSeriesProperty<double> *protonchargelog =
        new Kernel::TimeSeriesProperty<double>("proton_charge");
    int64_t curtime_ns = runstarttime_ns;
    while (curtime_ns <= runstoptime_ns)
    {
      Kernel::DateAndTime curtime(curtime_ns);
      protonchargelog->addValue(curtime, 1.0);
      curtime_ns += pulsetime_ns;
    }
    eventws->mutableRun().addProperty(protonchargelog, true);
    std::cout << "Proton charge log from " << runstarttime_ns << " to " << runstoptime_ns << "\n";

    // 4. Sine value log (value record 1/4 of pulse time.  it is FAST)
    Kernel::TimeSeriesProperty<double> *sinlog = new Kernel::TimeSeriesProperty<double>("FastSineLog");
    double period = static_cast<double>(pulsetime_ns);
    curtime_ns = runstarttime_ns;
    while (curtime_ns < runstoptime_ns)
    {
      Kernel::DateAndTime curtime(curtime_ns);
      double value = sin(M_PI*static_cast<double>(curtime_ns)/period*0.25);
      sinlog->addValue(curtime, value);
      curtime_ns += pulsetime_ns/4;
    }
    eventws->mutableRun().addProperty(sinlog, true);

    // 5. Cosine value log (value record 4 pulse time.  it is SLOW)
    Kernel::TimeSeriesProperty<double> *coslog = new Kernel::TimeSeriesProperty<double>("SlowCosineLog");
    period = static_cast<double>(pulsetime_ns*10);
    curtime_ns = runstarttime_ns;
    while (curtime_ns < runstoptime_ns)
    {
      Kernel::DateAndTime curtime(curtime_ns);
      double value = sin(2*M_PI*static_cast<double>(curtime_ns)/period);
      coslog->addValue(curtime, value);
      curtime_ns += pulsetime_ns*2;
    }
    eventws->mutableRun().addProperty(coslog, true);

    std::cout << "<----------- Number of events = " << eventws->getNumberEvents() << "\n";

    return eventws;
  }

  //----------------------------------------------------------------------------------------------
  /** Create an EventWorkspace containing an integer log
    * 1. Run start  = 10  (s)
    * 2. Run end    = 22  (s)
    * 3. Pulse      = 0.5 (s)
    * 4. Log change = 1   (s)
    */
  EventWorkspace_sptr createEventWorkspaceIntLog()
  {
    using namespace WorkspaceCreationHelper;

    // 1. Empty workspace
    EventWorkspace_sptr eventws =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(2, 2, true);

    // 2. Run star time
    int64_t factor = static_cast<int64_t>(1.0E9+0.5);
    int64_t runstarttime_ns = 10*factor;
    int64_t runstoptime_ns  = 22*factor;
    int64_t pulsetime_ns    = 5*factor/10;
    int64_t logduration_ns  = 1*factor;

    Kernel::DateAndTime runstarttime(runstarttime_ns);
    eventws->mutableRun().addProperty("run_start", runstarttime.toISO8601String());
    Kernel::DateAndTime runendtime(runstoptime_ns);
    eventws->mutableRun().addProperty("run_end", runendtime.toISO8601String());

    // 3. Proton charge log
    Kernel::TimeSeriesProperty<double> *protonchargelog =
        new Kernel::TimeSeriesProperty<double>("proton_charge");
    int64_t curtime_ns = runstarttime_ns;
    while (curtime_ns <= runstoptime_ns)
    {
      Kernel::DateAndTime curtime(curtime_ns);
      protonchargelog->addValue(curtime, 1.0);
      curtime_ns += pulsetime_ns;
    }
    eventws->mutableRun().addProperty(protonchargelog, true);

    // 4. Integer log
    TimeSeriesProperty<int> *dummyintlog = new TimeSeriesProperty<int>("DummyIntLog");

    int logstep = 1;
    int logvalue = 0;
    // double period = static_cast<double>(pulsetime_ns);
    curtime_ns = runstarttime_ns;
    while (curtime_ns < runstoptime_ns)
    {
      Kernel::DateAndTime curtime(curtime_ns);
      dummyintlog->addValue(curtime, logvalue);

      curtime_ns += logduration_ns;
      logvalue += logstep;
    }
    eventws->mutableRun().addProperty(dummyintlog, true);

    return eventws;
  }

  //----------------------------------------------------------------------------------------------
  /** Test generation of splitters by time for matrix splitter
   */
  void test_genTime1IntervalMatrixSplitter()
  {
    // Create input Workspace
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspace();
    AnalysisDataService::Instance().add("TestWorkspace", eventWS);

    // Init and set property
    GenerateEventsFilter alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", eventWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "Splitters05"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InformationWorkspace", "SplittersInformation"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FastLog", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartTime", "100"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StopTime", "1000000"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UnitOfTime", "Nanoseconds"));

    // Running and get result
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Check output
    API::MatrixWorkspace_sptr splittersws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve("Splitters05"));
    DataObjects::TableWorkspace_sptr splittersinfo =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve("SplittersInformation"));

    TS_ASSERT(splittersws);

    TS_ASSERT_EQUALS(splittersws->readX(0).size(), 2);
    TS_ASSERT_EQUALS(splittersws->readY(0).size(), 1);
    Kernel::DateAndTime runstart(3000000000);
    TS_ASSERT_EQUALS(static_cast<int64_t>(splittersws->readX(0)[0]), runstart.totalNanoseconds()+100);
    TS_ASSERT_EQUALS(static_cast<int64_t>(splittersws->readX(0)[1]), runstart.totalNanoseconds()+1000000);
    TS_ASSERT_EQUALS(static_cast<int>(splittersws->readY(0)[0]), 0);

    TS_ASSERT_EQUALS(splittersinfo->rowCount(), 1);

    AnalysisDataService::Instance().remove("Splitters05");
    AnalysisDataService::Instance().remove("SplittersInformation");
    AnalysisDataService::Instance().remove("TestWorkspace");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test generation of splitters by time
   * (1) Multiple time interval
   * (2) Default start time and stop time
   */
  void test_genTimeMultipleIntervalMatrixSplitter()
  {
    // Create input Workspace & initial setup
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspace();
    AnalysisDataService::Instance().addOrReplace("TestEventWorkspace08", eventWS);
    int64_t timeinterval_ns = 15000;

    // Init and set property
    GenerateEventsFilter alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspace", "TestEventWorkspace08"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "Splitters08"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InformationWorkspace", "InfoWS08"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FastLog", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeInterval", 15000.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UnitOfTime", "Nanoseconds"));

    // Running and get result
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Check output workspace
    API::MatrixWorkspace_sptr splittersws =
        boost::dynamic_pointer_cast<API::MatrixWorkspace>(AnalysisDataService::Instance().retrieve("Splitters08"));

    TS_ASSERT(splittersws);

    // Check values of output workspace
    size_t numintervals = 74;
    TS_ASSERT_EQUALS(splittersws->readY(0).size(), numintervals);

    std::string runstarttimestr = eventWS->run().getProperty("run_start")->value();
    Kernel::DateAndTime runstarttime(runstarttimestr);
    int64_t runstarttime_ns = runstarttime.totalNanoseconds();

    Kernel::TimeSeriesProperty<double> *protonchargelog =
        dynamic_cast<Kernel::TimeSeriesProperty<double>* >(eventWS->run().getProperty("proton_charge"));
    Kernel::DateAndTime runstoptime = Kernel::DateAndTime(protonchargelog->lastTime().totalNanoseconds() + 100000);

    // First interval
    TS_ASSERT_EQUALS(static_cast<int64_t>(splittersws->readX(0)[0]), runstarttime_ns);
    TS_ASSERT_EQUALS(static_cast<int64_t>(splittersws->readX(0)[1]), runstarttime_ns + timeinterval_ns);
    TS_ASSERT_EQUALS(static_cast<int>(splittersws->readY(0)[0]), 0);

    // c) Last interval
    TS_ASSERT_EQUALS(static_cast<int64_t>(splittersws->readX(0).back()), runstoptime.totalNanoseconds());
    TS_ASSERT_EQUALS(static_cast<int>(splittersws->readY(0).back()), numintervals-1);

    /* d) Randomly

    Kernel::SplittingInterval splitterR = splittersws->getSplitter(40);
    Kernel::DateAndTime t0 = splitterR.start();
    Kernel::DateAndTime tf = splitterR.stop();
    int64_t dt_ns = tf.totalNanoseconds()-t0.totalNanoseconds();
    TS_ASSERT_EQUALS(dt_ns, timeinterval_ns);
    int64_t dt_runtimestart = t0.totalNanoseconds()-runstarttime_ns;
    TS_ASSERT_EQUALS(dt_runtimestart, 40*timeinterval_ns);
    */

    // 5. Clean
    AnalysisDataService::Instance().remove("Splitters08");
    AnalysisDataService::Instance().remove("InfoWS08");
    AnalysisDataService::Instance().remove("TestEventWorkspace08");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Generate filter by log values in increasing
   * (1) No time tolerance
   * (2) Just one
   */
  void test_genMultipleLogValuesFilterMatrixSplitter()
  {
    std::cout << "\n==== Test Multiple Log Value Filter (Matrix Splitter) ====\n" << std::endl;

    // Create input
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspace();
    AnalysisDataService::Instance().addOrReplace("TestEventWS04B", eventWS);

    // Init and set property
    GenerateEventsFilter alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "TestEventWS04B"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "Splitters04B"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InformationWorkspace", "InfoWS04B"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FastLog", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogName", "FastSineLog"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MinimumLogValue", "-1.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaximumLogValue",  "1.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogValueInterval", 0.2));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogValueTolerance", 0.05));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FilterLogValueByChangingDirection", "Increase"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeTolerance", 1.0E-8));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogBoundary",  "Centre"));

    // Running and get result
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Check output workspace
    MatrixWorkspace_sptr splittersws =
        boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("Splitters04B"));
    TS_ASSERT(splittersws);

    DataObjects::TableWorkspace_const_sptr infows =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve("InfoWS04B"));
    TS_ASSERT(infows);

    // Run again for non-fast log output
    GenerateEventsFilter alg2;
    alg2.initialize();

    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("InputWorkspace", "TestEventWS04B"));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("OutputWorkspace", "Splitters04C"));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("InformationWorkspace", "InfoWS04C"));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("FastLog", false));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("LogName", "FastSineLog"));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("MinimumLogValue", "-1.0"));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("MaximumLogValue",  "1.0"));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("LogValueInterval", 0.2));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("LogValueTolerance", 0.05));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("FilterLogValueByChangingDirection", "Increase"));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("TimeTolerance", 1.0E-8));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("LogBoundary",  "Centre"));
    
    TS_ASSERT_THROWS_NOTHING(alg2.execute());
    TS_ASSERT(alg2.isExecuted());

    SplittersWorkspace_sptr splittersws2 = boost::dynamic_pointer_cast<SplittersWorkspace>(
          AnalysisDataService::Instance().retrieve("Splitters04C"));
    TS_ASSERT(splittersws2);

    // Compare the result between 2 workspaces containing splitters
    std::vector<Kernel::SplittingInterval> splitters;
    size_t numsplitters = convertMatrixSplitterToSplitters(splittersws, splitters);
    TS_ASSERT_EQUALS(numsplitters, splittersws2->getNumberSplitters());

    if (numsplitters == splittersws2->getNumberSplitters())
    {
      for (size_t i = 0; i < numsplitters; ++i)
      {
        Kernel::SplittingInterval s2 = splittersws2->getSplitter(i);
        Kernel::SplittingInterval s1 = splitters[i];
        TS_ASSERT_EQUALS(s1.start(), s2.start());
        TS_ASSERT_EQUALS(s1.stop(), s2.stop());
        TS_ASSERT_EQUALS(s1.index(), s2.index());
      }
    }

    // Clean
    AnalysisDataService::Instance().remove("TestEventWS04B");
    AnalysisDataService::Instance().remove("Splitters04B");
    AnalysisDataService::Instance().remove("InfoWS04B");
    AnalysisDataService::Instance().remove("Splitters04C");
    AnalysisDataService::Instance().remove("InfoWS04C");
  }



  //----------------------------------------------------------------------------------------------
  /** Generate filter by log values in increasing
   * (1) No time tolerance
   * (2) Just one
   */
  void test_genMultipleLogValuesFilterMatrixSplitterParallel()
  {
    // Create input
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspace();
    AnalysisDataService::Instance().addOrReplace("TestEventWS04B", eventWS);

    // Init and set property
    GenerateEventsFilter alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "TestEventWS04B"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "Splitters04B"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InformationWorkspace", "InfoWS04B"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FastLog", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogName", "FastSineLog"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MinimumLogValue", "-1.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaximumLogValue",  "1.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogValueInterval", 0.2));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogValueTolerance", 0.05));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FilterLogValueByChangingDirection", "Increase"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeTolerance", 1.0E-8));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogBoundary",  "Centre"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UseParallelProcessing",  "Parallel"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("NumberOfThreads", 4));

    // Running and get result
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Check output workspace
    MatrixWorkspace_sptr splittersws =
        boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("Splitters04B"));
    TS_ASSERT(splittersws);

    DataObjects::TableWorkspace_const_sptr infows =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve("InfoWS04B"));
    TS_ASSERT(infows);

    // Run again for non-fast log output
    GenerateEventsFilter alg2;
    alg2.initialize();

    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("InputWorkspace", "TestEventWS04B"));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("OutputWorkspace", "Splitters04C"));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("InformationWorkspace", "InfoWS04C"));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("FastLog", false));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("LogName", "FastSineLog"));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("MinimumLogValue", "-1.0"));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("MaximumLogValue",  "1.0"));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("LogValueInterval", 0.2));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("LogValueTolerance", 0.05));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("FilterLogValueByChangingDirection", "Increase"));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("TimeTolerance", 1.0E-8));
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("LogBoundary",  "Centre"));

    TS_ASSERT_THROWS_NOTHING(alg2.execute());
    TS_ASSERT(alg2.isExecuted());

    SplittersWorkspace_sptr splittersws2 = boost::dynamic_pointer_cast<SplittersWorkspace>(
          AnalysisDataService::Instance().retrieve("Splitters04C"));
    TS_ASSERT(splittersws2);

    // Compare the result between 2 workspaces containing splitters
    std::vector<Kernel::SplittingInterval> splitters;
    size_t numsplitters = convertMatrixSplitterToSplitters(splittersws, splitters);
    TS_ASSERT_EQUALS(numsplitters, splittersws2->getNumberSplitters());

    if (numsplitters == splittersws2->getNumberSplitters())
    {
      for (size_t i = 0; i < numsplitters; ++i)
      {
        Kernel::SplittingInterval s2 = splittersws2->getSplitter(i);
        Kernel::SplittingInterval s1 = splitters[i];
        TS_ASSERT_EQUALS(s1.start(), s2.start());
        TS_ASSERT_EQUALS(s1.stop(), s2.stop());
        TS_ASSERT_EQUALS(s1.index(), s2.index());
      }
    }

    // Clean
    AnalysisDataService::Instance().remove("TestEventWS04B");
    AnalysisDataService::Instance().remove("Splitters04B");
    AnalysisDataService::Instance().remove("InfoWS04B");
    AnalysisDataService::Instance().remove("Splitters04C");
    AnalysisDataService::Instance().remove("InfoWS04C");

  }

  //----------------------------------------------------------------------------------------------
  /** Generate filter by log values in 'FastLog' mode only 1 interval
   */
  void test_genSingleleLogValuesFilterMatrixSplitter()
  {
    std::cout << "\n==== Test Single Log Value Filter (Matrix Splitter) ====\n" << "\n";

    // Create input
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspace();
    AnalysisDataService::Instance().addOrReplace("TestEventWS09", eventWS);

    // Init and set property
    GenerateEventsFilter alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "TestEventWS09"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "Splitters09"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InformationWorkspace", "InfoWS09"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FastLog", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogName", "FastSineLog"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MinimumLogValue", "-1.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaximumLogValue",  "1.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogValueTolerance", 0.05));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FilterLogValueByChangingDirection", "Both"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeTolerance", 1.0E-8));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogBoundary",  "Centre"));

    // Running and get result
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Check output workspace
    MatrixWorkspace_sptr splittersws =
        boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("Splitters09"));
    TS_ASSERT(splittersws);
    if (splittersws)
      TS_ASSERT(splittersws->readX(0).size() >= 2);

    DataObjects::TableWorkspace_const_sptr infows =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve("InfoWS09"));
    TS_ASSERT(infows);


    // Clean
    AnalysisDataService::Instance().remove("TestEventWS09");
    AnalysisDataService::Instance().remove("Splitters09");
    AnalysisDataService::Instance().remove("InfoWS09");
  }


  //----------------------------------------------------------------------------------------------
  /** Generate filter by integer log values in increasing in matrix workspace
   * (1) No time tolerance
   * (2) Just one
   */
  void test_genMultipleIntLogValuesFilterMatrixSplitter()
  {
    // Create input
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspaceIntLog();
    AnalysisDataService::Instance().addOrReplace("TestEventWS09", eventWS);

    // Init and set property
    GenerateEventsFilter alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", "TestEventWS09"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "Splitters09"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InformationWorkspace", "InfoWS09"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FastLog", true));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogName", "DummyIntLog"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MinimumLogValue", static_cast<double>(1)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaximumLogValue", static_cast<double>(10)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogValueInterval", static_cast<double>(1)));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("UnitOfTime", "Seconds"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FilterLogValueByChangingDirection", "Both"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeTolerance", 0.05));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogBoundary",  "Centre"));

    // Running and get result
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Check output workspace
    MatrixWorkspace_sptr splittersws =
        boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("Splitters09"));
    TS_ASSERT(splittersws);

    DataObjects::TableWorkspace_const_sptr infows =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve("InfoWS09"));
    TS_ASSERT(infows);

    if (!splittersws || !infows)
      return;

    TS_ASSERT_EQUALS(splittersws->readY(0).size(), 10);

    int64_t factor = static_cast<int64_t>(1.0E9+0.5);
    TS_ASSERT_DELTA(splittersws->readX(0)[0], static_cast<double>(11*factor-5*factor/100), 0.000001);

    TS_ASSERT_DELTA(splittersws->readY(0)[0], 0.0, 0.00001);
    TS_ASSERT_DELTA(splittersws->readY(0)[1], 1.0, 0.00001);
  }


  //----------------------------------------------------------------------------------------------
  /** Convert the splitters stored in a matrix workspace to a vector of SplittingInterval objects
    */
  size_t convertMatrixSplitterToSplitters(API::MatrixWorkspace_const_sptr matrixws,
                                        std::vector<Kernel::SplittingInterval>& splitters)
  {
    splitters.clear();
    size_t numsplitters = 0;

    const MantidVec& vecX = matrixws->readX(0);
    const MantidVec& vecY = matrixws->readY(0);
    for (size_t i = 0; i < vecY.size(); ++i)
    {
      if (vecY[i] >= -0.0)
      {
        // A valid time interval for Splitters
        Kernel::DateAndTime tstart(static_cast<int64_t>(vecX[i]));
        Kernel::DateAndTime tstop(static_cast<int64_t>(vecX[i+1]));
        int wsindex = static_cast<int>(vecY[i]);

        Kernel::SplittingInterval ti(tstart, tstop, wsindex);
        splitters.push_back(ti);
        ++ numsplitters;
      }
    }

    return numsplitters;
  }

};


#endif /* MANTID_ALGORITHMS_GENERATEEVENTSFILTERTEST_H_ */


























