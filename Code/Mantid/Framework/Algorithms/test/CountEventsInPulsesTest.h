#ifndef MANTID_ALGORITHMS_COUNTEVENTSINPULSESTEST_H_
#define MANTID_ALGORITHMS_COUNTEVENTSINPULSESTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidAlgorithms/CountEventsInPulses.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidDataObjects/Events.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class CountEventsInPulsesTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CountEventsInPulsesTest *createSuite() { return new CountEventsInPulsesTest(); }
  static void destroySuite( CountEventsInPulsesTest *suite ) { delete suite; }


  /*
   * Test initialization of algorithm
   */
  void test_Init()
  {
    CountEventsInPulses count;
    TS_ASSERT_THROWS_NOTHING(count.initialize());
    TS_ASSERT(count.isInitialized());

    return;
  }

  void test_Workspace2DPer1Pulse()
  {
    // 1. Create Workspace
    std::string wsname("Input01");
    Kernel::DateAndTime run_start(10000000);
    size_t numpulses = 1000;
    DataObjects::EventWorkspace_sptr eventWS = creatEventWorkspace(wsname, run_start, numpulses);

    // 2. Set properties and execute
    CountEventsInPulses count;
    TS_ASSERT_THROWS_NOTHING(count.initialize());

    count.setProperty("InputWorkspace", eventWS);
    count.setProperty("OutputWorkspace", "TestCount1");
    count.setProperty("PulsesPerBin", 1);
    count.setProperty("SumSpectra", false);
    count.setProperty("Unit", "microsecond");
    count.setProperty("Parallel", false);
    count.setProperty("PreserveEvents", false);

    TS_ASSERT_THROWS_NOTHING(count.execute());

    TS_ASSERT(count.isExecuted());

    // 3. Check result
    DataObjects::Workspace2D_sptr outWS =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(AnalysisDataService::Instance().retrieve("TestCount1"));

    TS_ASSERT(outWS);

    TS_ASSERT_EQUALS(outWS->dataX(0).size(), 1000);

    for (size_t iw = 0; iw < outWS->dataY(0).size(); iw ++)
      TS_ASSERT_DELTA(outWS->dataY(0)[iw], 0, 1.0E-8);

    for (size_t iw = 3; iw < 5; iw ++)
    {
      for (size_t ip = 0; ip < outWS->dataY(iw).size(); ip ++)
      {
        size_t numevents = iw + ip + 1;
        TS_ASSERT_DELTA(outWS->dataY(iw)[ip], static_cast<double>(numevents), 1.0E-8);
      }
    }

    return;
  }

  void test_Workspace2DPer5Pulse()
  {
    // 1. Create Workspace
    std::string wsname("Input02");
    Kernel::DateAndTime run_start(10000000);
    size_t numpulses = 1000;
    DataObjects::EventWorkspace_sptr eventWS = creatEventWorkspace(wsname, run_start, numpulses);

    // 2. Set properties and execute
    CountEventsInPulses count;
    TS_ASSERT_THROWS_NOTHING(count.initialize());

    count.setProperty("InputWorkspace", eventWS);
    count.setProperty("OutputWorkspace", "TestCount2");
    count.setProperty("PulsesPerBin", 5);
    count.setProperty("SumSpectra", false);
    count.setProperty("Unit", "microsecond");
    count.setProperty("Parallel", false);
    count.setProperty("PreserveEvents", false);

    TS_ASSERT_THROWS_NOTHING(count.execute());

    TS_ASSERT(count.isExecuted());

    // 3. Check result
    DataObjects::Workspace2D_sptr outWS =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(AnalysisDataService::Instance().retrieve("TestCount2"));

    TS_ASSERT(outWS);

    TS_ASSERT_EQUALS(outWS->dataX(0).size(), 200);

    for (size_t iw = 0; iw < outWS->dataY(0).size(); iw ++)
      TS_ASSERT_DELTA(outWS->dataY(0)[iw], 0, 1.0E-8);

    for (size_t iw = 3; iw < 5; iw ++)
    {
      for (size_t ip = 0; ip < outWS->dataY(iw).size(); ip ++)
      {
        size_t numevents = 0;
        size_t numeventstosum = 5;
        if (ip == outWS->dataY(iw).size()-1)
          numeventstosum += 5-1;
        for (size_t i = 0; i < numeventstosum; i ++)
        {
          numevents += iw + ip*5+i + 1;
        }
        TS_ASSERT_DELTA(outWS->dataY(iw)[ip], static_cast<double>(numevents), 1.0E-8);
      }
    }

    return;
  }

  void test_Workspace2DPer1PulseSumSpectra()
  {
    // 1. Create Workspace
    std::string wsname("Input03");
    Kernel::DateAndTime run_start(10000000);
    size_t numpulses = 1000;
    DataObjects::EventWorkspace_sptr eventWS = creatEventWorkspace(wsname, run_start, numpulses);

    // 2. Set properties and execute
    CountEventsInPulses count;
    TS_ASSERT_THROWS_NOTHING(count.initialize());

    count.setProperty("InputWorkspace", eventWS);
    count.setProperty("OutputWorkspace", "TestCount3");
    count.setProperty("PulsesPerBin", 1);
    count.setProperty("SumSpectra", true);
    count.setProperty("Unit", "microsecond");
    count.setProperty("Parallel", false);
    count.setProperty("PreserveEvents", false);

    TS_ASSERT_THROWS_NOTHING(count.execute());

    TS_ASSERT(count.isExecuted());

    // 3. Check result
    DataObjects::Workspace2D_sptr outWS =
        boost::dynamic_pointer_cast<DataObjects::Workspace2D>(AnalysisDataService::Instance().retrieve("TestCount3"));

    TS_ASSERT(outWS);

    TS_ASSERT_EQUALS(outWS->dataX(0).size(), 1000);

    for (size_t ip = 0; ip < outWS->dataY(0).size(); ip ++)
    {
      size_t numevents = 0;
      for (size_t iw = 3; iw < 5; iw ++)
      {
        numevents += iw + ip + 1;
      }
      TS_ASSERT_DELTA(outWS->dataY(0)[ip], static_cast<double>(numevents), 1.0E-8);
    }

    return;
  }

  void test_EventWorkspacePer1Pulse()
  {
    // 1. Create Workspace
    std::string wsname("Input04");
    Kernel::DateAndTime run_start(10000000000);
    size_t numpulses = 1000;
    DataObjects::EventWorkspace_sptr eventWS = creatEventWorkspace(wsname, run_start, numpulses);

    // 2. Set properties and execute
    CountEventsInPulses count;
    TS_ASSERT_THROWS_NOTHING(count.initialize());

    count.setProperty("InputWorkspace", eventWS);
    count.setProperty("OutputWorkspace", "TestCount4");
    count.setProperty("PulsesPerBin", 1);
    count.setProperty("SumSpectra", false);
    count.setProperty("Unit", "microsecond");
    count.setProperty("Parallel", false);
    count.setProperty("PreserveEvents", true);

    TS_ASSERT_THROWS_NOTHING(count.execute());

    TS_ASSERT(count.isExecuted());

    // 3. Check result
    DataObjects::EventWorkspace_sptr outWS =
        boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve("TestCount4"));

    TS_ASSERT(outWS);

    TS_ASSERT_EQUALS(outWS->readX(0).size(), 1000);

    for (size_t iw = 0; iw < outWS->readY(0).size(); iw ++)
      TS_ASSERT_DELTA(outWS->readY(0)[iw], 0, 1.0E-8);

    for (size_t iw = 3; iw < 5; iw ++)
    {
      for (size_t ip = 0; ip < outWS->readY(iw).size(); ip ++)
      {
        size_t numevents = iw + ip + 1;
        TS_ASSERT_DELTA(outWS->readY(iw)[ip], static_cast<double>(numevents), 1.0);
      }
    }

    return;
  }

  /*
   * Build an eventworkspace including some events and a fake proton charge log
   */
  DataObjects::EventWorkspace_sptr creatEventWorkspace(std::string wsname, Kernel::DateAndTime run_start, size_t numpulses)
  {

    // 1. Init to 1 spectrum, 2 vector x, 1 vector y
    DataObjects::EventWorkspace_sptr eventWS = DataObjects::EventWorkspace_sptr(new DataObjects::EventWorkspace);

    eventWS->init(1, 2, 1);
    eventWS->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
    eventWS->setYUnit("Counts");
    eventWS->setTitle("TestWorkspace");
    eventWS->setName(wsname);

    eventWS->mutableRun().addProperty("run_start", run_start.toISO8601String(), true);

    // 2. Add A Dummy Instrument, i.e., VULCAN
    DataHandling::LoadInstrument load;
    load.initialize();

    load.setProperty("Workspace", eventWS);
    load.setProperty("InstrumentName", "VULCAN");

    load.execute();
    TS_ASSERT(load.isExecuted());

    // 3. Add spectrum list to each workspace index/detector
    detid2det_map detector_map;
    eventWS->getInstrument()->getDetectors(detector_map);
    std::vector<detid_t> detids;

    size_t wsindex = 0;
    for (detid2det_map::iterator it=detector_map.begin(); it!=detector_map.end(); ++it){
      if (it->second->isMonitor()){
        std::cout << "Detector " << it->first << " is monitor" << std::endl;
      }

      if (it->first > 0){
        // if not monitor
        DataObjects::EventList& events = eventWS->getOrAddEventList(wsindex);
        events.setSpectrumNo(static_cast<specid_t>(wsindex+1));
        events.clearDetectorIDs();
        events.addDetectorID(it->first);

        // check!
        detid_t detectorid = -99;
        std::set<detid_t> detectorids = eventWS->getEventList(wsindex).getDetectorIDs();
        std::set<detid_t>::iterator detiter;
        for (detiter=detectorids.begin(); detiter!=detectorids.end(); ++detiter){
          detectorid = *detiter;
        }
        detids.push_back(detectorid);
        wsindex ++;
      }
    }

    // 4. Add proton charge log (for full list of pulse)
    double pulselength = 1.0E9/50.0;

    double pcharge = 1.3;

    Kernel::TimeSeriesProperty<double>* protonchargelog = new Kernel::TimeSeriesProperty<double>("proton_charge");
    for (size_t i = 0; i < numpulses; i ++)
    {
      Kernel::DateAndTime pulsetime(run_start.totalNanoseconds()+static_cast<int64_t>(static_cast<double>(i)*pulselength));
      protonchargelog->addValue(pulsetime, pcharge);
    }
    eventWS->mutableRun().addProperty(protonchargelog);

    // 5. Add Events
    for (size_t iws = 3; iws < 5; ++iws)
    {
      DataObjects::EventList* eventlist = eventWS->getEventListPtr(iws);

      for (size_t ip = 0; ip < numpulses-1; ip ++)
      {
        Kernel::DateAndTime pulsetime = protonchargelog->nthTime(static_cast<int>(ip));
        size_t numevents = ip+iws+1;
        double dtof_ms =(pulselength*0.5)*1.0E-3/static_cast<double>(numevents);
        for (size_t ie = 0; ie < numevents; ie ++)
        {
          double tof = static_cast<double>(ie+1)*dtof_ms;
          DataObjects::TofEvent newevent(tof, pulsetime);
          eventlist->addEventQuickly(newevent);
        } // FOR: add single event
      } // FOR: add events belonging to single pulse
    } // FOR: add events for single detector

    return eventWS;
  }

};


#endif /* MANTID_ALGORITHMS_COUNTEVENTSINPULSESTEST_H_ */
