#ifndef MANTID_ALGORITHMS_VULCANFILTEREVENTSTEST_H_
#define MANTID_ALGORITHMS_VULCANFILTEREVENTSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
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

class FilterEventsHighFrequencyTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FilterEventsHighFrequencyTest *createSuite() { return new FilterEventsHighFrequencyTest(); }
  static void destroySuite( FilterEventsHighFrequencyTest *suite ) { delete suite; }

  void test_Initialization()
  {
    FilterEventsHighFrequency alg;
    alg.initialize();

    TS_ASSERT(alg.isInitialized());
  }

  void test_FilterEventsZeroOffset()
  {

    std::cout << "\nTest Filter Events with zero offsets" << std::endl;

    // 1. Set up
    DataObjects::EventWorkspace_sptr eventWS(new DataObjects::EventWorkspace());
    AnalysisDataService::Instance().addOrReplace("RawData1", eventWS);
    std::string logname = "DummySensor";
    std::string calibfilename = "detoffset.dat";
    double ssoffset = 0.0;
    double value_low = 0.9;
    double value_upp = 1.1;
    std::string outputWSname = "TestOutput1";
    double t0 = 0.0;
    double tf = 9.9;
    Kernel::DateAndTime run_start(20000000000);
    size_t numevents = 10;
    size_t lenlog = 20;

    std::cout << "Run Start Time = " << run_start << " / " << run_start.total_nanoseconds() << " ns" << std::endl;

    double detoffset = 1.0;
    double deltatof = 200;
    buildEventWorkspace(eventWS, run_start, numevents, detoffset);
    addTimeSeriesLog(eventWS, run_start, logname, lenlog, deltatof);

    // 2. Initialize
    FilterEventsHighFrequency alg;
    alg.initialize();

    // 3. Set up
    alg.setPropertyValue("InputEventWorkspace", "RawData1");
    alg.setProperty("LogName", logname);
    alg.setProperty("InputCalFile", calibfilename);
    alg.setProperty("SensorToSampleOffset", ssoffset);
    alg.setProperty("ValueLowerBoundary", value_low);
    alg.setProperty("ValueUpperBoundary", value_upp);
    alg.setPropertyValue("OutputWorkspace", outputWSname);
    alg.setProperty("T0", t0);
    alg.setProperty("Tf", tf);

    // 4. Execute
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // 5. Verify algorithm result
    DataObjects::EventWorkspace_sptr filterWS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
        AnalysisDataService::Instance().retrieve(outputWSname));

    std::cout << "Output Workspace Name = " << filterWS->getName() << std::endl;

    // i. Compare Spectrum Number
    /*
    for (size_t i = 0; i < 10; i ++){
      detid_t detin = getDetector(eventWS, i);
      detid_t detout = getDetector(filterWS, i);
      std::cout << "Spectrum " << i << "  Workspace (in) = " << detin << "  Workspace (out) = " << detout << std::endl;
    }
    */

    size_t numspec = eventWS->getNumberHistograms();
    TS_ASSERT_EQUALS(filterWS->getNumberHistograms(), numspec);

    // ii. Check Log
    // Kernel::TimeSeriesProperty<double>* timeproperty = boost::dynamic_pointer_cast<Kernel::TimeSeriesProperty<double>* >(eventWS->run().getLogData(logname));
    const API::Run& runlogs = eventWS->run();
    Kernel::TimeSeriesProperty<double> * timeproperty
        = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( runlogs.getLogData(logname) );
    std::vector<Kernel::DateAndTime> times = timeproperty->timesAsVector();
    std::cout << "Number of entries in time series = " << times.size() << std::endl;
    for (size_t i = 0; i < times.size(); i ++){
      std::cout << "Log Time = " << times[i] << " / " << times[i].total_nanoseconds() << ":  " << timeproperty->getSingleValue(times[i]) << std::endl;
    }

    // iii. Check spectrum 1
    DataObjects::EventList eventsIn0 = eventWS->getEventList(0);
    std::cout << "(In)  Spectrum 0: Number of Events = " << eventsIn0.getNumberEvents() << std::endl;

    DataObjects::EventList events0 = filterWS->getEventList(0);
    std::cout << "(Out) Spectrum 0:  Number of Events = " << events0.getNumberEvents() << std::endl;
    for (size_t i = 0; i < events0.getNumberEvents(); i ++){
      DataObjects::WeightedEvent e0 = events0.getEvent(i);
      int64_t abstime2 = e0.pulseTime().total_nanoseconds()+static_cast<int64_t>(e0.tof()*1000.0);
      std::cout << "Selected Event " << i << " = " << e0.pulseTime() << ", " << e0.m_tof << " / " << abstime2<< std::endl;
    }

    DataObjects::WeightedEvent e0;
    int64_t abstime2;

    TS_ASSERT_EQUALS(events0.getNumberEvents(), 5);

    if (events0.getNumberEvents() > 2){
      e0 = events0.getEvent(0);
      abstime2 = e0.pulseTime().total_nanoseconds()+static_cast<int64_t>(e0.tof()*1000.0);
      TS_ASSERT_EQUALS(abstime2, 20002800000);
      e0 = events0.getEvent(1);
      abstime2 = e0.pulseTime().total_nanoseconds()+static_cast<int64_t>(e0.tof()*1000.0);
      TS_ASSERT_EQUALS(abstime2, 20001200000);
    }

    for (size_t i = 0; i < eventsIn0.getNumberEvents(); i ++)
    {
      DataObjects::WeightedEvent event = eventsIn0.getEvent(i);
      std::cout << "Event " <<  std::setw(5) << i <<
          event.pulseTime().total_nanoseconds()+static_cast<int64_t>(event.tof()*1000) << std::endl;
    }

    // -1. Use Poco/file to delete generated temp file
    // -1.1 testcal.dat
    Poco::File("detoffset.dat").remove();

  }

  void test_FilterEventsOffset()
  {

    std::cout << "\nTest Filter Events With Offsets" << std::endl;

    // 1. Set up
    DataObjects::EventWorkspace_sptr eventWS(new DataObjects::EventWorkspace());
    AnalysisDataService::Instance().addOrReplace("RawData2", eventWS);
    std::string logname = "DummySensor";
    std::string calibfilename = "detoffset.dat";
    double ssoffset = 0.0;
    double value_low = 0.9;
    double value_upp = 1.1;
    std::string outputWSname = "TestOutput2";
    double t0 = 0.0;
    double tf = 9.9;
    Kernel::DateAndTime run_start(20000000000);
    std::cout << "Run Start Time = " << run_start << " / " << run_start.total_nanoseconds() << " ns" << std::endl;
    size_t numevents = 10;
    size_t lenlog = 20;

    double detoffset = 0.6;
    double deltatof = 200;
    buildEventWorkspace(eventWS, run_start, numevents, detoffset);
    addTimeSeriesLog(eventWS, run_start, logname, lenlog, deltatof);

    // 2. Initialize
    FilterEventsHighFrequency alg;
    alg.initialize();

    // 3. Set up
    alg.setPropertyValue("InputEventWorkspace", "RawData2");
    alg.setProperty("LogName", logname);
    alg.setProperty("InputCalFile", calibfilename);
    // alg.setPropertyValue("SampleEnvironmentWorkspace", "NOUSED");
    alg.setProperty("SensorToSampleOffset", ssoffset);
    alg.setProperty("ValueLowerBoundary", value_low);
    alg.setProperty("ValueUpperBoundary", value_upp);
    alg.setPropertyValue("OutputWorkspace", outputWSname);
    alg.setProperty("T0", t0);
    alg.setProperty("Tf", tf);

    // 4. Execute
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // 5. Verify algorithm result
     DataObjects::EventWorkspace_sptr filterWS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
         AnalysisDataService::Instance().retrieve(outputWSname));

     std::cout << "Output Workspace Name = " << filterWS->getName() << std::endl;

     // i. Compare Spectrum Number
     /*
     for (size_t i = 0; i < 10; i ++){
       detid_t detin = getDetector(eventWS, i);
       detid_t detout = getDetector(filterWS, i);
       std::cout << "Spectrum " << i << "  Workspace (in) = " << detin << "  Workspace (out) = " << detout << std::endl;
     }
     */

     size_t numspec = eventWS->getNumberHistograms();
     TS_ASSERT_EQUALS(filterWS->getNumberHistograms(), numspec);

     // ii. Check Log
     // Kernel::TimeSeriesProperty<double>* timeproperty = boost::dynamic_pointer_cast<Kernel::TimeSeriesProperty<double>* >(eventWS->run().getLogData(logname));
     const API::Run& runlogs = eventWS->run();
     Kernel::TimeSeriesProperty<double> * timeproperty
         = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( runlogs.getLogData(logname) );
     std::vector<Kernel::DateAndTime> times = timeproperty->timesAsVector();
     std::cout << "Number of entries in time series = " << times.size() << std::endl;
     for (size_t i = 0; i < times.size(); i ++){
       std::cout << "Log Time = " << times[i] << " / " << times[i].total_nanoseconds() << ":  " << timeproperty->getSingleValue(times[i]) << std::endl;
     }

     // iii. Check spectrum 1
     DataObjects::EventList eventsIn0 = eventWS->getEventList(0);
     std::cout << "(In) Spectrum 0: Number of Events = " << eventsIn0.getNumberEvents() << std::endl;

     DataObjects::EventList events0 = filterWS->getEventList(0);
     std::cout << "Spectrum 0:  Number of Events = " << events0.getNumberEvents() << std::endl;
     for (size_t i = 0; i < events0.getNumberEvents(); i ++){
       DataObjects::WeightedEvent e0 = events0.getEvent(i);
       int64_t abstime2 = e0.pulseTime().total_nanoseconds()+static_cast<int64_t>(e0.tof()*1000.0);
       std::cout << "Selected Event " << i << " = " << e0.pulseTime() << ", " << e0.m_tof << " / " << abstime2<< std::endl;
     }

     DataObjects::WeightedEvent e0;
     int64_t abstime2;

     TS_ASSERT_EQUALS(events0.getNumberEvents(), 5);

     e0 = events0.getEvent(0);
     abstime2 = e0.pulseTime().total_nanoseconds()+static_cast<int64_t>(e0.tof()*1000.0);
     TS_ASSERT_EQUALS(abstime2, 20002800000);
     e0 = events0.getEvent(3);
     abstime2 = e0.pulseTime().total_nanoseconds()+static_cast<int64_t>(e0.tof()*1000.0);
     TS_ASSERT_EQUALS(abstime2, 20001400000);

     // -1. Use Poco/file to delete generated temp file
     // -1.1 testcal.dat
     Poco::File("detoffset.dat").remove();

  }

  void test_FilterSingleSpectrumEventsOffset()
   {

     std::cout << "\nTest Filter Events With Offsets" << std::endl;

     // 1. Set up
     DataObjects::EventWorkspace_sptr eventWS(new DataObjects::EventWorkspace());
     AnalysisDataService::Instance().addOrReplace("RawData3", eventWS);
     std::string logname = "DummySensor";
     std::string calibfilename = "detoffset.dat";
     double ssoffset = 0.0;
     double value_low = 0.9;
     double value_upp = 1.1;
     std::string outputWSname = "TestOutput3";
     double t0 = 0.0;
     double tf = 9.9;
     Kernel::DateAndTime run_start(20000000000);
     std::cout << "Run Start Time = " << run_start << " / " << run_start.total_nanoseconds() << " ns" << std::endl;
     size_t numevents = 10;
     size_t lenlog = 20;

     double detoffset = 0.6;
     double deltatof = 200;
     buildEventWorkspace(eventWS, run_start, numevents, detoffset);
     addTimeSeriesLog(eventWS, run_start, logname, lenlog, deltatof);

     // 2. Initialize
     FilterEventsHighFrequency alg;
     alg.initialize();

     // 3. Set up
     alg.setPropertyValue("InputEventWorkspace", "RawData3");
     alg.setProperty("LogName", logname);
     alg.setProperty("InputCalFile", calibfilename);
     // alg.setPropertyValue("SampleEnvironmentWorkspace", "NOUSED");
     alg.setProperty("SensorToSampleOffset", ssoffset);
     alg.setProperty("ValueLowerBoundary", value_low);
     alg.setProperty("ValueUpperBoundary", value_upp);
     alg.setPropertyValue("OutputWorkspace", outputWSname);
     alg.setProperty("T0", t0);
     alg.setProperty("Tf", tf);
     alg.setProperty("WorkspaceIndex", 0);
     alg.setProperty("NumberOfIntervals", 5);

     // 4. Execute
     alg.execute();
     TS_ASSERT(alg.isExecuted());

     // 5. Verify algorithm result
      DataObjects::EventWorkspace_sptr filterWS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
          AnalysisDataService::Instance().retrieve(outputWSname));

      std::cout << "Output Workspace Name = " << filterWS->getName() << std::endl;

      // i. Compare Spectrum Number
      /*
      for (size_t i = 0; i < 10; i ++){
        detid_t detin = getDetector(eventWS, i);
        detid_t detout = getDetector(filterWS, i);
        std::cout << "Spectrum " << i << "  Workspace (in) = " << detin << "  Workspace (out) = " << detout << std::endl;
      }
      */

      size_t numspec = eventWS->getNumberHistograms();
      TS_ASSERT_EQUALS(filterWS->getNumberHistograms(), numspec);

      // ii. Check Log
      const API::Run& runlogs = eventWS->run();
      Kernel::TimeSeriesProperty<double> * timeproperty
          = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( runlogs.getLogData(logname) );
      std::vector<Kernel::DateAndTime> times = timeproperty->timesAsVector();
      std::cout << "Number of entries in time series = " << times.size() << std::endl;
      for (size_t i = 0; i < times.size(); i ++){
        std::cout << "Log Time = " << times[i] << " / " << times[i].total_nanoseconds() << ":  " << timeproperty->getSingleValue(times[i]) << std::endl;
      }

      // iii. Check spectrum 1
      DataObjects::EventList eventsIn0 = eventWS->getEventList(0);
      std::cout << "(In) Spectrum 0: Number of Events = " << eventsIn0.getNumberEvents() << std::endl;

      DataObjects::EventList events0 = filterWS->getEventList(0);
      std::cout << "Spectrum 0:  Number of Events = " << events0.getNumberEvents() << std::endl;
      for (size_t i = 0; i < events0.getNumberEvents(); i ++){
        DataObjects::WeightedEvent e0 = events0.getEvent(i);
        int64_t abstime2 = e0.pulseTime().total_nanoseconds()+static_cast<int64_t>(e0.tof()*1000.0);
        std::cout << "Selected Event " << i << " = " << e0.pulseTime() << ", " << e0.m_tof << " / " << abstime2<< std::endl;
      }

      DataObjects::WeightedEvent e0;
      int64_t abstime2;

      TS_ASSERT_EQUALS(events0.getNumberEvents(), 5);

      e0 = events0.getEvent(0);
      abstime2 = e0.pulseTime().total_nanoseconds()+static_cast<int64_t>(e0.tof()*1000.0);
      TS_ASSERT_EQUALS(abstime2, 20001200000);
      e0 = events0.getEvent(3);
      abstime2 = e0.pulseTime().total_nanoseconds()+static_cast<int64_t>(e0.tof()*1000.0);
      TS_ASSERT_EQUALS(abstime2, 20002600000);

      // -1. Use Poco/file to delete generated temp file
      // -1.1 testcal.dat
      Poco::File("detoffset.dat").remove();

   }

  detid_t getDetector(DataObjects::EventWorkspace_sptr inWS, size_t specid){
    DataObjects::EventList events = inWS->getEventList(specid);
    std::set<detid_t> detids = events.getDetectorIDs();
    detid_t detid = -1;
    for (std::set<detid_t>::iterator it=detids.begin(); it!=detids.end(); ++it){
      detid = *it;
    }
    return detid;
  }

  /*
   * Build an event workspace with instrument and events
   * (1) Instrument is VULCAN.  Thus it has thousands of detector
   * (2) Add same number of events to each detector
   */
  void buildEventWorkspace(DataObjects::EventWorkspace_sptr iws, Kernel::DateAndTime run_start, size_t numevents,
      double detectoroffset)
  {

    // 1. Init to 1 spectrum, 2 vector x, 1 vector y
    iws->init(1, 2, 1);
    iws->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
    iws->setYUnit("Counts");
    iws->setTitle("TestWorkspace");

    // 2. Add A Dummy Instrument
    DataHandling::LoadInstrument load;
    load.initialize();

    load.setProperty("Workspace", iws);
    load.setProperty("InstrumentName", "VULCAN");

    load.execute();

    TS_ASSERT(load.isExecuted());

    std::cout << iws->getInstrument()->getName() << std::endl;
    std::cout << "Number of Detectors = " << iws->getInstrument()->getDetectorIDs().size() << std::endl;

    // 3. Pop up all spectra/detector map
    detid2det_map detector_map;
    iws->getInstrument()->getDetectors(detector_map);
    std::vector<detid_t> detids;

    size_t wsindex = 0;
    for (detid2det_map::iterator it=detector_map.begin(); it!=detector_map.end(); ++it){
      if (it->second->isMonitor()){
        std::cout << "Detector " << it->first << " is monitor" << std::endl;
      }

      if (it->first > 0){
        // if not monitor
        DataObjects::EventList& events = iws->getOrAddEventList(wsindex);
        events.setSpectrumNo(static_cast<specid_t>(wsindex+1));
        events.clearDetectorIDs();
        events.addDetectorID(it->first);

        // check!
        detid_t detectorid = -99;
        std::set<detid_t> detectorids = iws->getEventList(wsindex).getDetectorIDs();
        std::set<detid_t>::iterator detiter;
        for (detiter=detectorids.begin(); detiter!=detectorids.end(); ++detiter){
          detectorid = *detiter;
        }
        detids.push_back(detectorid);
        wsindex ++;
      }
    }

    // 4. Add Events
    std::cout << "Before Adding Events (spec 0): " << iws->getEventListPtr(0)->getNumberEvents() << std::endl;
    int64_t pulsetime = run_start.total_nanoseconds();

    for (size_t isp=0; isp<iws->getNumberHistograms(); isp++){
      double dtof = 200;
      double tof = 1000+static_cast<double>(isp);
      DataObjects::EventList* events = iws->getEventListPtr(isp);

      for (size_t i = 0; i < numevents; i ++){
        // a) generate an event
        DataObjects::TofEvent event;
        event.m_pulsetime = pulsetime;
        event.m_tof = tof;
        tof += dtof;

        // b) add event list
        events->addEventQuickly(event);
      } // FOREACH event

    } // FOREACH spectrum

    // 5. Output Detector ID file
    std::ofstream ofs;
    ofs.open("detoffset.dat", std::ios::out);
    for (size_t i = 0; i < detids.size(); i ++){
      ofs << std::setw(10) << detids[i] << std::setprecision(5) << std::setw(15) << detectoroffset << std::endl;
    }
    ofs.close();

    std::cout << "After Adding Events (spec 0): " << iws->getEventListPtr(0)->getNumberEvents() << std::endl;

    return;
  }

  /*
   * Add (1) run_start time (2) time series log and generate a test calibration file
   * @param dtof: delta tof (ms)
   */
  void addTimeSeriesLog(DataObjects::EventWorkspace_sptr iws, Kernel::DateAndTime run_start,
      std::string logname, size_t lenlog, double dtof){

    // 1. Add run_start
    iws->mutableRun().addProperty("run_start", run_start.to_ISO8601_string(), true);

    // 2. Add a log at the same intervals to events added
    Kernel::TimeSeriesProperty<double> *timeprop = new Kernel::TimeSeriesProperty<double>(logname);
    double tof = 100;
    int64_t t0 = run_start.total_nanoseconds();
    double value = 0;
    for (size_t i = 0; i < lenlog; i ++){
      int64_t st = t0+static_cast<int64_t>((tof+dtof*static_cast<double>(i))*1000);
      Kernel::DateAndTime logtime(st);
      timeprop->addValue(logtime, value);
      value = 1.0-value;
    }

    iws->mutableRun().addProperty(timeprop, true);

    /*
    // 3. Generate a calibration file
    // a) get all detectors
    std::fstream fs;
    fs.open(calibfilename.c_str(), std::ios::out);
    for (size_t i = 0; i < iws->getNumberHistograms(); i ++){
      DataObjects::EventList *events = iws->getEventListPtr(i);
      std::set<detid_t> detids = events->getDetectorIDs();
      detid_t detid = 0;
      std::set<detid_t>::iterator detiter;
      for (detiter=detids.begin(); detiter!=detids.end(); ++detiter){
        detid = *detiter;
      }

      fs << detid << "\t\t" << 0.0 << std::endl;
    }
    fs.close();
    std::cout << "Writing a calibration file " << calibfilename << std::endl;
    */

    return;
  }


};


#endif /* MANTID_ALGORITHMS_VULCANFILTEREVENTSTEST_H_ */
