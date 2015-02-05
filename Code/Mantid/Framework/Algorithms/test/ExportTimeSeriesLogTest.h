#ifndef MANTID_ALGORITHMS_EXPORTTIMESERIESLOGTEST_H_
#define MANTID_ALGORITHMS_EXPORTTIMESERIESLOGTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <cmath>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <Poco/File.h>

#include "MantidAlgorithms/ExportTimeSeriesLog.h"
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

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class ExportTimeSeriesLogTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExportTimeSeriesLogTest *createSuite() { return new ExportTimeSeriesLogTest(); }
  static void destroySuite( ExportTimeSeriesLogTest *suite ) { delete suite; }


  /** Test initialization
    */
  void test_Init()
  {
    ExportTimeSeriesLog getalg;
    TS_ASSERT_THROWS_NOTHING(getalg.initialize());
    TS_ASSERT(getalg.isInitialized());
  }

  /** Test outpout event workspace
    */
  void test_OutputEventWorkspace()
  {
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspace();
    AnalysisDataService::Instance().addOrReplace("EventWorkspace", eventWS);

    ExportTimeSeriesLog getalg;
    getalg.initialize();

    getalg.setProperty("InputWorkspace", eventWS);
    getalg.setProperty("OutputWorkspace", "FastSineLogEventWS");
    getalg.setProperty("LogName", "FastSineLog");
    getalg.setProperty("IsEventWorkspace", true);

    getalg.execute();
    TS_ASSERT(getalg.isExecuted());

    EventWorkspace_sptr outws = boost::dynamic_pointer_cast<EventWorkspace>(
          AnalysisDataService::Instance().retrieve("FastSineLogEventWS"));
    TS_ASSERT(outws);

    size_t numevents = outws->getNumberEvents();
    TS_ASSERT_EQUALS(numevents, 40);

    // -1 Clean
    AnalysisDataService::Instance().remove("EventWorkspace");
    AnalysisDataService::Instance().remove("FastSineLogEventWS");

    return;
  }

  /** Test outpout Workspace2D
    */
  void test_Output2DWorkspace()
  {
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspace();
    AnalysisDataService::Instance().addOrReplace("EventWorkspace", eventWS);

    ExportTimeSeriesLog getalg;
    getalg.initialize();

    getalg.setProperty("InputWorkspace", eventWS);
    getalg.setProperty("OutputWorkspace", "FastSineLog2DWS");
    getalg.setProperty("LogName", "FastSineLog");
    getalg.setProperty("IsEventWorkspace", false);

    getalg.execute();
    TS_ASSERT(getalg.isExecuted());

    Workspace2D_sptr outws = boost::dynamic_pointer_cast<Workspace2D>(
          AnalysisDataService::Instance().retrieve("FastSineLog2DWS"));
    TS_ASSERT(outws);

    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outws->dataX(0).size(), 40);

    for (size_t i = 1; i < 40; ++i)
    {
      TS_ASSERT(outws->dataX(0)[i-1] < outws->dataX(0)[i]);
    }


    // -1 Clean
    AnalysisDataService::Instance().remove("EventWorkspace");
    AnalysisDataService::Instance().remove("FastSineLog2DWS");

    return;
  }

  /** Test outpout Workspace2D with limited number
    */
  void test_Output2DWorkspacePartialLog()
  {
    DataObjects::EventWorkspace_sptr eventWS = createEventWorkspace();
    AnalysisDataService::Instance().addOrReplace("EventWorkspace", eventWS);

    ExportTimeSeriesLog getalg;
    getalg.initialize();

    getalg.setProperty("InputWorkspace", eventWS);
    getalg.setProperty("OutputWorkspace", "FastSineLog2DWS");
    getalg.setProperty("LogName", "FastSineLog");
    getalg.setProperty("NumberEntriesExport", 20);
    getalg.setProperty("IsEventWorkspace", false);

    getalg.execute();
    TS_ASSERT(getalg.isExecuted());

    Workspace2D_sptr outws = boost::dynamic_pointer_cast<Workspace2D>(
          AnalysisDataService::Instance().retrieve("FastSineLog2DWS"));
    TS_ASSERT(outws);

    TS_ASSERT_EQUALS(outws->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outws->dataX(0).size(), 20);

    for (size_t i = 1; i < 20; ++i)
    {
      TS_ASSERT(outws->dataX(0)[i-1] < outws->dataX(0)[i]);
    }

    // -1 Clean
    AnalysisDataService::Instance().remove("EventWorkspace");
    AnalysisDataService::Instance().remove("FastSineLog2DWS");

  }


  /** Create an EventWorkspace including
   * (1) proton charge log from
   * (2) test log in sin function with time
   */
  DataObjects::EventWorkspace_sptr createEventWorkspace()
  {
    using namespace WorkspaceCreationHelper;

    // 1. Empty workspace
    DataObjects::EventWorkspace_sptr eventws =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(2, 2, true);
    //eventws->setName("TestWorkspace");

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
    size_t numevents = 0;
    while (curtime_ns < runstoptime_ns)
    {
      Kernel::DateAndTime curtime(curtime_ns);
      double value = sin(M_PI*static_cast<double>(curtime_ns)/period*0.25);
      sinlog->addValue(curtime, value);
      curtime_ns += pulsetime_ns/4;
      ++ numevents;
    }
    eventws->mutableRun().addProperty(sinlog, true);
    // cout << "Log 'FastSine' has " << numevents << ".\n";

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

    return eventws;
  }


};


#endif /* MANTID_ALGORITHMS_EXPORTTIMESERIESLOGTEST_H_ */
