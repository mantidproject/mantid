#ifndef MANTID_DATAHANDLING_PROCESSDASNEXUSLOGTEST_H_
#define MANTID_DATAHANDLING_PROCESSDASNEXUSLOGTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidDataHandling/ProcessDasNexusLog.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class ProcessDasNexusLogTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProcessDasNexusLogTest *createSuite() { return new ProcessDasNexusLogTest(); }
  static void destroySuite( ProcessDasNexusLogTest *suite ) { delete suite; }


  void test_ConvertLog()
  {
      // 1. Create event workspace w/ pulse as 0.1 second
      DataObjects::EventWorkspace_sptr eventws = createEventWorkspace(0.1, 100);
      AnalysisDataService::Instance().addOrReplace("EventWS", eventws);

      // 2. Start
      ProcessDasNexusLog alg;
      TS_ASSERT_THROWS_NOTHING(alg.initialize());
      TS_ASSERT(alg.isInitialized());

      // 3. Do it
      alg.setPropertyValue("InputWorkspace", "EventWS");
      alg.setProperty("LogToProcess", "daslog");
      alg.setProperty("ProcessedLog", "newlog");
      alg.setProperty("NumberOfOutputs", -1);
      alg.setProperty("OutputLogFile", "mylog.dat");
      alg.setProperty("OutputDirectory", "./");

      TS_ASSERT_THROWS_NOTHING(alg.execute());
      TS_ASSERT(alg.isExecuted());

      // 4. Get result
      DataObjects::EventWorkspace_sptr outws =
              boost::dynamic_pointer_cast<DataObjects::EventWorkspace>
              (AnalysisDataService::Instance().retrieve("EventWS"));
      TS_ASSERT(outws);
      if (!outws)
          return;

      // 5. Check data
      Kernel::TimeSeriesProperty<double>* newlog = dynamic_cast<Kernel::TimeSeriesProperty<double>* >(outws->run().getProperty("newlog"));

      TS_ASSERT_EQUALS(newlog->size(), 100);

      int64_t t0 =  newlog->nthTime(0).totalNanoseconds();
      TS_ASSERT_EQUALS(t0, 20000030000);

      // -1. Clean
      AnalysisDataService::Instance().remove("EventWS");

  }


  /*
   * Create an EventWorkspace.  This workspace will have
   * (1) Events with wall time even in time
   * (2) Pulse length: in second
   */
  DataObjects::EventWorkspace_sptr createEventWorkspace(double pulselength, size_t numpulses)
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

    // 3. Add a log (DAS)
    Kernel::TimeSeriesProperty<double>* daslog = new Kernel::TimeSeriesProperty<double>("daslog");

    double tofms = 300.0;
    for (size_t i = 0; i < numpulses; i ++)
    {
      Kernel::DateAndTime pulsetime(runstart.totalNanoseconds()+static_cast<int64_t>(static_cast<double>(i)*pulselength*1.0E-9));
      daslog->addValue(pulsetime, tofms);
    }
    eventWS->mutableRun().addProperty(daslog);

    return eventWS;
  }


};


#endif /* MANTID_DATAHANDLING_PROCESSDASNEXUSLOGTEST_H_ */
