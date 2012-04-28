#ifndef MANTID_ALGORITHMS_GENERATEEVENTSFILTERTEST_H_
#define MANTID_ALGORITHMS_GENERATEEVENTSFILTERTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/GenerateEventsFilter.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidKernel/TimeSplitter.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/Column.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/Events.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <Poco/File.h>
#include "MantidAlgorithms/FilterEventsHighFrequency.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

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

  /*
   * Test generation of splitters by time
   */
  void test_genTime1Interval()
  {
    // 1. Create input Workspace
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspace();
    AnalysisDataService::Instance().add("TestWorkspace", eventWS);

    // 2. Init and set property
    GenerateEventsFilter alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", eventWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "Splitters01"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("SplittersInformationWorkspace", "SplittersInformation"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StartTime", "100"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("StopTime", "1000000"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeType", "Nanoseconds"));

    // 3. Running and get result
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    // TS_ASSERT(alg.isExecuted());

    // 4. Check output
    DataObjects::SplittersWorkspace_sptr splittersws =
        boost::dynamic_pointer_cast<DataObjects::SplittersWorkspace>(AnalysisDataService::Instance().retrieve("Splitters01"));
    DataObjects::TableWorkspace_const_sptr splittersinfo =
        boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve("SplittersInformation"));

    TS_ASSERT(splittersws);

    TS_ASSERT_EQUALS(splittersws->getNumberSplitters(), 1);
    Kernel::SplittingInterval splitter0 = splittersws->getSplitter(0);
    Kernel::DateAndTime runstart(3000000000);
    TS_ASSERT_EQUALS(splitter0.start().totalNanoseconds(), runstart.totalNanoseconds()+100);
    TS_ASSERT_EQUALS(splitter0.stop().totalNanoseconds(), runstart.totalNanoseconds()+1000000);
    TS_ASSERT_EQUALS(splitter0.index(), 0);

    AnalysisDataService::Instance().remove("Splitters01");

    return;
  }

  /*
   * Test generation of splitters by time
   * (1) Multiple time interval
   * (2) Default start time and stop time
   */
  void test_genTimeMultipleInterval()
  {
    // 1. Create input Workspace
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspace();
    int64_t timeinterval_ns = 15000;

    // 2. Init and set property
    GenerateEventsFilter alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", eventWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "Splitters01"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeInterval", 15000.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeType", "Nanoseconds"));

    // 3. Running and get result
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 4. Check output
    DataObjects::SplittersWorkspace_sptr splittersws =
        boost::dynamic_pointer_cast<DataObjects::SplittersWorkspace>(AnalysisDataService::Instance().retrieve("Splitters01"));

    TS_ASSERT(splittersws);

    size_t numintervals = 67;
    TS_ASSERT_EQUALS(splittersws->getNumberSplitters(), numintervals);

    std::string runstarttimestr = eventWS->run().getProperty("run_start")->value();
    Kernel::DateAndTime runstarttime(runstarttimestr);
    int64_t runstarttime_ns = runstarttime.totalNanoseconds();

    Kernel::TimeSeriesProperty<double> *protonchargelog =
        dynamic_cast<Kernel::TimeSeriesProperty<double>* >(eventWS->run().getProperty("proton_charge"));
    Kernel::DateAndTime runstoptime = protonchargelog->lastTime();

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

  /*
   * Generate filter by log value in simple way
   * (1) No time tolerance
   * (2) Just one
   */
  void test_genSimpleLogValueFilter()
  {
    // 1. Create input
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspace();

    // 2. Init and set property
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
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SplittersInformationWorkspace", "Information"));

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
     TS_ASSERT_EQUALS(s0.start().totalNanoseconds(), 3000025000-static_cast<int64_t>(1.0E-8*1.0E9));
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


  /*
   * Generate filter by log values in increasing
   * (1) No time tolerance
   * (2) Just one
   */
  void test_genMultipleLogValuesFilter()
  {
    std::cout << "\nTest Simple Log Value Filter\n" << std::endl;

    // 1. Create input
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspace();

    // 2. Init and set property
    GenerateEventsFilter alg;
    alg.initialize();

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", eventWS));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "Splitters04"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SplittersInformationWorkspace", "Information"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogName", "FastSineLog"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MinimumLogValue", "-1.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaximumLogValue",  "1.0"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogValueInterval", 0.2));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogValueTolerance", 0.05));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("FilterLogValueByChangingDirection", "Increase"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeTolerance", 1.0E-8));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogBoundary",  "Centre"));

    // 3. Running and get result
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // 4. Check output
     DataObjects::SplittersWorkspace_sptr splittersws =
         boost::dynamic_pointer_cast<DataObjects::SplittersWorkspace>(AnalysisDataService::Instance().retrieve("Splitters04"));
     TS_ASSERT(splittersws);

     DataObjects::TableWorkspace_const_sptr infows =
         boost::dynamic_pointer_cast<DataObjects::TableWorkspace>(AnalysisDataService::Instance().retrieve("Information"));
     TS_ASSERT(infows);

     // 5. Check
     size_t numsplitters = 15;
     TS_ASSERT_EQUALS(splittersws->getNumberSplitters(), numsplitters);
     size_t numoutputs = 11;
     TS_ASSERT_EQUALS(infows->rowCount(), numoutputs);

     Kernel::SplittingInterval s0 = splittersws->getSplitter(0);
     TS_ASSERT_EQUALS(s0.start(), 3000024990);
     TS_ASSERT_EQUALS(s0.index(), 6);

     Kernel::SplittingInterval s14 = splittersws->getSplitter(14);
     TS_ASSERT_EQUALS(s14.start(), 3000924990);
     TS_ASSERT_EQUALS(s14.stop(),  3000949990);
     TS_ASSERT_EQUALS(s14.index(), 9);

  }


  /*
   * Create an EventWorkspace including
   * (1) proton charge log from
   * (2) test log in sin function with time
   */
  DataObjects::EventWorkspace_sptr createEventWorkspace()
  {
    using namespace WorkspaceCreationHelper;
    double PI = 3.14159265;

    // 1. Empty workspace
    DataObjects::EventWorkspace_sptr eventws =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(2, 2, true);
    eventws->setName("TestWorkspace");

    // 2. Run star time
    int64_t runstarttime_ns = 3000000000;
    int64_t runstoptime_ns  = 3001000000;
    int64_t pulsetime_ns    =     100000;

    Kernel::DateAndTime runstarttime(runstarttime_ns);
    eventws->mutableRun().addProperty("run_start", runstarttime.toISO8601String());

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

    // 4. Sine value log (value record 1/4 of pulse time.  it is FAST)
    Kernel::TimeSeriesProperty<double> *sinlog = new Kernel::TimeSeriesProperty<double>("FastSineLog");
    double period = static_cast<double>(pulsetime_ns);
    curtime_ns = runstarttime_ns;
    while (curtime_ns < runstoptime_ns)
    {
      Kernel::DateAndTime curtime(curtime_ns);
      double value = sin(PI*static_cast<double>(curtime_ns)/period*0.25);
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
      double value = sin(2*PI*static_cast<double>(curtime_ns)/period);
      coslog->addValue(curtime, value);
      curtime_ns += pulsetime_ns*2;
    }
    eventws->mutableRun().addProperty(coslog, true);

    return eventws;
  }


};


#endif /* MANTID_ALGORITHMS_GENERATEEVENTSFILTERTEST_H_ */
