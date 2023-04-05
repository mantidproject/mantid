// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/TableRow.h"
#include "MantidAlgorithms/FilterEvents.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <random>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Types::Event::TofEvent;

using namespace std;

/* TODO LIST
 *  1. Remove all Ptest
 *  2. Add a new unit test for grouping workspaces in the end
 *  3. Add a new unit test for throwing grouping workspaces if name is not vaid
 *  4. Add a new unit test for excluding sample logs
 *  5. Parallelizing spliting logs?
 *  6. Speed test
 *    6.1 with or without splitting logs;
 *    6.2 different types of splitters workspaces;
 *    6.3 old vs new
 */

class FilterEventsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FilterEventsTest *createSuite() { return new FilterEventsTest(); }
  static void destroySuite(FilterEventsTest *suite) { delete suite; }

  //----------------------------------------------------------------------------------------------
  /** Test initialization
   */
  void test_Initialization() {
    std::cout << std::endl << "test_Initialization..." << std::endl;
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
   * (4) Within one pulse, two consecutive events/neutrons is apart for
   * 10*1000*1000 seconds
   * (5) "Experiment": 5 pulse times.  10 events in each pulse
   */
  void test_CreatedEventWorskpaceAndSplitter() {
    std::cout << std::endl << "test_CreatedEventWorskpaceAndSplitter..." << std::endl;
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100 * 1000 * 1000;
    int64_t tofdt = 10 * 1000 * 1000;
    size_t numpulses = 5;

    EventWorkspace_sptr eventws = createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);

    TS_ASSERT_EQUALS(eventws->getNumberEvents(), 500);

    EventList elist = eventws->getSpectrum(0);
    TS_ASSERT_EQUALS(elist.getNumberEvents(), 50);
    TS_ASSERT(elist.hasDetectorID(1));

    SplittersWorkspace_sptr splittersws = createSplittersWorkspace(runstart_i64, pulsedt, tofdt);
    TS_ASSERT_EQUALS(splittersws->getNumberSplitters(), 5);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /**  Filter events without any correction
   *  Event workspace:
   * (1) 10 detectors
   * (2) Run starts @ 20000000000 ns
   * (3) Pulse length = 100*1000*1000 ns
   * (4) Within one pulse, two consecutive events/neutrons is apart for
   *10*1000*1000 ns
   * (5) "Experiment": 5 pulse times.  10 events in each pulse
   *
   * In this test
   *  (1) Leave correction table workspace empty
   *  (2) Count events in each output including "-1", the excluded/unselected
   *      events
   *
   *  Splitter-log test: each output workspace should have a sample log named
   *"splitter", which
   *  is created by FilterEvents to record the splitters for the corresponding
   *workspace
   *  1: 20000000000, 20035000000, 0
   *  2: 20035000000, 20195000000, 1
   *  3: 20200000000, 20265000000, 2
   *  4: 20300000000, 20365000000, 2
   *  5: 20400000000, 20465000000, 2
   */
  void test_FilterNoCorrection() {
    std::cout << std::endl << "test_FilterNoCorrection..." << std::endl;
    int64_t runstart_i64 = 20000000000;  // 20 seconds, beginning of the fake run
    int64_t pulsedt = 100 * 1000 * 1000; // 100 miliseconds, time between consecutive pulses
    int64_t tofdt = 10 * 1000 * 1000;    // 10 miliseconds, spacing between neutrons events within a pulse
    size_t numpulses = 5;

    // Create EventWorkspace with 10 detector-banks, each bank containing one pixel.
    EventWorkspace_sptr inpWS = createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("Test02", inpWS);

    // Create SplittersWorkspace
    // index         DateandTime        time-int64_t
    //   0    1990-Jan-01 00:00:20      20000000000
    //   1    1990-Jan-01 00:00:20.035  20035000000
    //  -1    1990-Jan-01 00:00:20.195  20195000000
    //   2    1990-Jan-01 00:00:20.200  20200000000
    //  -1    1990-Jan-01 00:00:20.265  20265000000
    //   2    1990-Jan-01 00:00:20.300  20300000000
    //  -1    1990-Jan-01 00:00:20.365  20365000000
    //   2    1990-Jan-01 00:00:20.400  20400000000
    //  -1    1990-Jan-01 00:00:20.465  20465000000
    SplittersWorkspace_sptr splws = createSplittersWorkspace(runstart_i64, pulsedt, tofdt);
    AnalysisDataService::Instance().addOrReplace("Splitter02", splws);

    FilterEvents filter;
    filter.initialize();

    // Set properties
    filter.setProperty("InputWorkspace", "Test02");
    filter.setProperty("OutputWorkspaceBaseName", "FilteredWS01");
    filter.setProperty("SplitterWorkspace", "Splitter02");
    filter.setProperty("OutputTOFCorrectionWorkspace", "CorrectionWS");
    filter.setProperty("OutputUnfilteredEvents", true);

    // Execute
    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    // Get output
    int numsplittedws = filter.getProperty("NumberOutputWS");
    TS_ASSERT_EQUALS(numsplittedws, 4);

    // Check Workspace group 0
    EventWorkspace_sptr filteredws0 =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS01_0"));
    TS_ASSERT(filteredws0);
    TS_ASSERT_EQUALS(filteredws0->getNumberHistograms(), 10);
    TS_ASSERT_EQUALS(filteredws0->getSpectrum(0).getNumberEvents(), 4);
    TS_ASSERT_EQUALS(filteredws0->run().getProtonCharge(), 1);
    TS_ASSERT_EQUALS(filteredws0->run().getTimeROI().debugStrPrint(),
                     "0: 1990-Jan-01 00:00:20 to 1990-Jan-01 00:00:20.035000000\n");
    TS_ASSERT_DELTA(std::stod(filteredws0->run().getProperty("duration")->value()), 0.035, 1.E-9);

    // Check Workspace group 1
    EventWorkspace_sptr filteredws1 =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS01_1"));
    TS_ASSERT(filteredws1);
    TS_ASSERT_EQUALS(filteredws1->getSpectrum(1).getNumberEvents(), 16);
    TS_ASSERT_EQUALS(filteredws1->run().getProtonCharge(), 1);
    TS_ASSERT_EQUALS(filteredws1->run().getTimeROI().debugStrPrint(),
                     "0: 1990-Jan-01 00:00:20.035000000 to 1990-Jan-01 00:00:20.195000000\n");
    TS_ASSERT_DELTA(std::stod(filteredws1->run().getProperty("duration")->value()), 0.160, 1.E-9);

    // Check Workspace group 2
    EventWorkspace_sptr filteredws2 =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS01_2"));
    TS_ASSERT(filteredws2);
    TS_ASSERT_EQUALS(filteredws2->getSpectrum(1).getNumberEvents(), 21);
    TS_ASSERT_EQUALS(filteredws2->run().getProtonCharge(), 3);
    TS_ASSERT_EQUALS(filteredws2->run().getTimeROI().debugStrPrint(),
                     "0: 1990-Jan-01 00:00:20.200000000 to 1990-Jan-01 00:00:20.265000000\n"
                     "1: 1990-Jan-01 00:00:20.300000000 to 1990-Jan-01 00:00:20.365000000\n"
                     "2: 1990-Jan-01 00:00:20.400000000 to 1990-Jan-01 00:00:20.465000000\n");
    TS_ASSERT_DELTA(std::stod(filteredws2->run().getProperty("duration")->value()), 0.195, 1.E-9);
    EventList elist3 = filteredws2->getSpectrum(3);
    elist3.sortPulseTimeTOF();
    TofEvent eventmin = elist3.getEvent(0);
    TS_ASSERT_EQUALS(eventmin.pulseTime().totalNanoseconds(), runstart_i64 + pulsedt * 2);
    TS_ASSERT_DELTA(eventmin.tof(), 0, 1.0E-4);
    TofEvent eventmax = elist3.getEvent(20);
    TS_ASSERT_EQUALS(eventmax.pulseTime().totalNanoseconds(), runstart_i64 + pulsedt * 4);
    TS_ASSERT_DELTA(eventmax.tof(), static_cast<double>(tofdt * 6 / 1000), 1.0E-4);

    // check unfiltered workspace
    EventWorkspace_sptr unfilteredws =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS01_unfiltered"));
    TS_ASSERT(unfilteredws);
    TS_ASSERT_EQUALS(unfilteredws->getSpectrum(1).getNumberEvents(), 9);
    TS_ASSERT_EQUALS(unfilteredws->run().getProtonCharge(), 0);
    TS_ASSERT_EQUALS(unfilteredws->run().getTimeROI().debugStrPrint(),
                     "0: 1990-Jan-01 00:00:20.195000000 to 1990-Jan-01 00:00:20.200000000\n"
                     "1: 1990-Jan-01 00:00:20.265000000 to 1990-Jan-01 00:00:20.300000000\n"
                     "2: 1990-Jan-01 00:00:20.365000000 to 1990-Jan-01 00:00:20.400000000\n"
                     "3: 1990-Jan-01 00:00:20.465000000 to 1990-Jan-01 00:00:20.500000000\n");
    TS_ASSERT_DELTA(std::stod(unfilteredws->run().getProperty("duration")->value()), 0.110, 1.E-9);

    // Clean up
    AnalysisDataService::Instance().remove("Test02");
    AnalysisDataService::Instance().remove("Splitter02");
    std::vector<std::string> outputwsnames = filter.getProperty("OutputWorkspaceNames");
    for (const auto &outputwsname : outputwsnames) {
      AnalysisDataService::Instance().remove(outputwsname);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /**  Filter events without any correction and test for user-specified workspace starting value
   *  Event workspace:
   * (1) 10 detectors
   * (2) Run starts @ 20000000000 seconds
   * (3) Pulse length = 100*1000*1000 seconds
   * (4) Within one pulse, two consecutive events/neutrons is apart for
   *10*1000*1000 seconds
   * (5) "Experiment": 5 pulse times.  10 events in each pulse
   *
   * In this test
   *  (1) Leave correction table workspace empty
   *  (2) Count events in each output including "-1", the excluded/unselected
   *events
   */
  void test_FilterWOCorrection2() {
    std::cout << std::endl << "test_FilterWOCorrection2..." << std::endl;
    // Create EventWorkspace and SplittersWorkspace
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100 * 1000 * 1000;
    int64_t tofdt = 10 * 1000 * 1000;
    size_t numpulses = 5;

    EventWorkspace_sptr inpWS = createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("Test02", inpWS);

    // Create SplittersWorkspace
    // index         DateandTime        time-int64_t
    //   0    1990-Jan-01 00:00:20      20000000000
    //   1    1990-Jan-01 00:00:20.035  20035000000
    //  -1    1990-Jan-01 00:00:20.195  20195000000
    //   2    1990-Jan-01 00:00:20.200  20200000000
    //  -1    1990-Jan-01 00:00:20.265  20265000000
    //   2    1990-Jan-01 00:00:20.300  20300000000
    //  -1    1990-Jan-01 00:00:20.365  20365000000
    //   2    1990-Jan-01 00:00:20.400  20400000000
    //  -1    1990-Jan-01 00:00:20.465  20465000000
    SplittersWorkspace_sptr splws = createSplittersWorkspace(runstart_i64, pulsedt, tofdt);
    AnalysisDataService::Instance().addOrReplace("Splitter02", splws);

    FilterEvents filter;
    filter.initialize();

    // Set properties
    filter.setProperty("InputWorkspace", "Test02");
    filter.setProperty("OutputWorkspaceBaseName", "FilteredWS01");
    filter.setProperty("SplitterWorkspace", "Splitter02");
    filter.setProperty("OutputUnfilteredEvents", true);

    // Execute
    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    // Get output
    int numsplittedws = filter.getProperty("NumberOutputWS");
    TS_ASSERT_EQUALS(numsplittedws, 4);

    // Workspace group 0
    EventWorkspace_sptr filteredws0 =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS01_0"));
    TS_ASSERT(filteredws0);
    TS_ASSERT_EQUALS(filteredws0->getNumberHistograms(), 10);
    TS_ASSERT_EQUALS(filteredws0->getSpectrum(0).getNumberEvents(), 4);
    TS_ASSERT_EQUALS(filteredws0->run().getProtonCharge(), 1);
    TS_ASSERT_EQUALS(filteredws0->run().getTimeROI().debugStrPrint(),
                     "0: 1990-Jan-01 00:00:20 to 1990-Jan-01 00:00:20.035000000\n");
    TS_ASSERT_DELTA(std::stod(filteredws0->run().getProperty("duration")->value()), 0.035, 1.E-9);

    // Workspace group 1
    EventWorkspace_sptr filteredws1 =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS01_1"));
    TS_ASSERT(filteredws1);
    TS_ASSERT_EQUALS(filteredws1->getSpectrum(1).getNumberEvents(), 16);
    TS_ASSERT_EQUALS(filteredws1->run().getProtonCharge(), 1);
    TS_ASSERT_EQUALS(filteredws1->run().getTimeROI().debugStrPrint(),
                     "0: 1990-Jan-01 00:00:20.035000000 to 1990-Jan-01 00:00:20.195000000\n");
    TS_ASSERT_DELTA(std::stod(filteredws1->run().getProperty("duration")->value()), 0.160, 1.E-9);

    // Workspace group 2
    EventWorkspace_sptr filteredws2 =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS01_2"));
    TS_ASSERT(filteredws2);
    TS_ASSERT_EQUALS(filteredws2->getSpectrum(1).getNumberEvents(), 21);
    TS_ASSERT_EQUALS(filteredws2->run().getProtonCharge(), 3);
    TS_ASSERT_EQUALS(filteredws2->run().getTimeROI().debugStrPrint(),
                     "0: 1990-Jan-01 00:00:20.200000000 to 1990-Jan-01 00:00:20.265000000\n"
                     "1: 1990-Jan-01 00:00:20.300000000 to 1990-Jan-01 00:00:20.365000000\n"
                     "2: 1990-Jan-01 00:00:20.400000000 to 1990-Jan-01 00:00:20.465000000\n");
    TS_ASSERT_DELTA(std::stod(filteredws2->run().getProperty("duration")->value()), 0.195, 1.E-9);
    EventList elist3 = filteredws2->getSpectrum(3);
    elist3.sortPulseTimeTOF();
    TofEvent eventmin = elist3.getEvent(0);
    TS_ASSERT_EQUALS(eventmin.pulseTime().totalNanoseconds(), runstart_i64 + pulsedt * 2);
    TS_ASSERT_DELTA(eventmin.tof(), 0, 1.0E-4);
    TofEvent eventmax = elist3.getEvent(20);
    TS_ASSERT_EQUALS(eventmax.pulseTime().totalNanoseconds(), runstart_i64 + pulsedt * 4);
    TS_ASSERT_DELTA(eventmax.tof(), static_cast<double>(tofdt * 6 / 1000), 1.0E-4);

    // Workspace Unfiltered
    // DEBUG: add tests for the unfiltered workspace
    EventWorkspace_sptr unfilteredws =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS01_unfiltered"));
    TS_ASSERT(unfilteredws);
    TS_ASSERT_EQUALS(unfilteredws->getSpectrum(1).getNumberEvents(), 9);
    TS_ASSERT_EQUALS(unfilteredws->run().getProtonCharge(), 0);
    TS_ASSERT_EQUALS(unfilteredws->run().getTimeROI().debugStrPrint(),
                     "0: 1990-Jan-01 00:00:20.195000000 to 1990-Jan-01 00:00:20.200000000\n"
                     "1: 1990-Jan-01 00:00:20.265000000 to 1990-Jan-01 00:00:20.300000000\n"
                     "2: 1990-Jan-01 00:00:20.365000000 to 1990-Jan-01 00:00:20.400000000\n"
                     "3: 1990-Jan-01 00:00:20.465000000 to 1990-Jan-01 00:00:20.500000000\n");
    TS_ASSERT_DELTA(std::stod(unfilteredws->run().getProperty("duration")->value()), 0.110, 1.E-9);

    // Clean up
    AnalysisDataService::Instance().remove("Test02");
    AnalysisDataService::Instance().remove("Splitter02");
    std::vector<std::string> outputwsnames = filter.getProperty("OutputWorkspaceNames");
    for (const auto &outputwsname : outputwsnames) {
      AnalysisDataService::Instance().remove(outputwsname);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /**  Filter test with TOF correction
   */
  void test_FilterWithCustomizedCorrection() {
    std::cout << std::endl << "test_FilterWithCustomizedCorrection..." << std::endl;
    // 1. Create EventWorkspace and SplittersWorkspace
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100 * 1000 * 1000;
    int64_t tofdt = 10 * 1000 * 1000;
    size_t numpulses = 5;

    EventWorkspace_sptr inpWS = createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("EventData", inpWS);

    // Create SplittersWorkspace with two destination workspaces plus the unfiltered workspace
    // index         DateandTime        time-int64_t
    //   0    1990-Jan-01 00:00:20.020  20000000000
    //  -1    1990-Jan-01 00:00:20.040  20040000000
    //   1    1990-Jan-01 00:00:20.060  20060000000
    //  -1    1990-Jan-01 00:00:20.080  20080000000
    //   0    1990-Jan-01 00:00:20.120  20120000000
    //  -1    1990-Jan-01 00:00:20.140  20140000000
    //   1    1990-Jan-01 00:00:20.160  20160000000
    //  -1    1990-Jan-01 00:00:20.180  20180000000
    //   0    1990-Jan-01 00:00:20.220  20220000000
    //  -1    1990-Jan-01 00:00:20.240  20240000000
    //   1    1990-Jan-01 00:00:20.260  20260000000
    //  -1    1990-Jan-01 00:00:20.280  20280000000
    //   0    1990-Jan-01 00:00:20.320  20320000000
    //  -1    1990-Jan-01 00:00:20.340  20340000000
    //   1    1990-Jan-01 00:00:20.360  20360000000
    //  -1    1990-Jan-01 00:00:20.380  20380000000
    //   0    1990-Jan-01 00:00:20.420  20420000000
    //  -1    1990-Jan-01 00:00:20.440  20440000000
    //   1    1990-Jan-01 00:00:20.460  20460000000
    //  -1    1990-Jan-01 00:00:20.480  20480000000
    SplittersWorkspace_sptr splws = createFastFreqLogSplitter(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("SplitterTableX", splws);
    TS_ASSERT_EQUALS(splws->rowCount(), static_cast<size_t>(numpulses) * 2);

    TableWorkspace_sptr timecorrws = createTimeCorrectionTable(inpWS);
    AnalysisDataService::Instance().addOrReplace("TimeCorrectionTableX", timecorrws);
    TS_ASSERT_EQUALS(timecorrws->rowCount(), inpWS->getNumberHistograms());

    FilterEvents filter;
    filter.initialize();

    // Set properties
    TS_ASSERT_THROWS_NOTHING(filter.setProperty("InputWorkspace", "EventData"));
    TS_ASSERT_THROWS_NOTHING(filter.setProperty("OutputWorkspaceBaseName", "SplittedDataX"));
    TS_ASSERT_THROWS_NOTHING(filter.setProperty("CorrectionToSample", "Customized"));
    TS_ASSERT_THROWS_NOTHING(filter.setProperty("DetectorTOFCorrectionWorkspace", "TimeCorrectionTableX"));
    TS_ASSERT_THROWS_NOTHING(filter.setProperty("SplitterWorkspace", splws));
    TS_ASSERT_THROWS_NOTHING(filter.setProperty("OutputUnfilteredEvents", true));

    // Execute
    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    // Get output
    // Workspace group 0
    EventWorkspace_sptr filteredws0 =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("SplittedDataX_0"));
    TS_ASSERT(filteredws0);
    TS_ASSERT_EQUALS(filteredws0->getNumberHistograms(), 10);
    TS_ASSERT_EQUALS(filteredws0->getSpectrum(0).getNumberEvents(), 15);
    TS_ASSERT_EQUALS(filteredws0->getSpectrum(9).getNumberEvents(), 15);
    TS_ASSERT_EQUALS(filteredws0->run().getProtonCharge(), 0);

    // Workspace group 1
    EventWorkspace_sptr filteredws1 =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("SplittedDataX_1"));
    TS_ASSERT(filteredws1);
    TS_ASSERT_EQUALS(filteredws1->getSpectrum(1).getNumberEvents(), 10);
    TS_ASSERT_EQUALS(filteredws0->run().getProtonCharge(), 0);
    // Some individual events
    EventList elist3 = filteredws1->getSpectrum(3);
    elist3.sortPulseTimeTOF();
    if (elist3.getNumberEvents() > 0) {
      TofEvent eventmin = elist3.getEvent(0);
      TS_ASSERT_EQUALS(eventmin.pulseTime().totalNanoseconds(), runstart_i64);
      TS_ASSERT_DELTA(eventmin.tof(), 80 * 1000, 1.0E-4);
    }

    // Unfiltered workspace
    EventWorkspace_sptr unfilteredws =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("SplittedDataX_unfiltered"));
    TS_ASSERT(unfilteredws);
    TS_ASSERT_EQUALS(unfilteredws->getSpectrum(1).getNumberEvents(), 25);
    TS_ASSERT_EQUALS(unfilteredws->run().getProtonCharge(), 5);
    // Clean
    AnalysisDataService::Instance().remove("EventData");
    AnalysisDataService::Instance().remove("TimeCorrectionTableX");
    AnalysisDataService::Instance().remove("SplitterTableX");

    std::vector<std::string> outputwsnames = filter.getProperty("OutputWorkspaceNames");
    for (const auto &outputwsname : outputwsnames) {
      AnalysisDataService::Instance().remove(outputwsname);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test filtering with correction of direct geometry
   */
  void test_FilterElasticCorrection() {
    std::cout << std::endl << "test_FilterElasticCorrection..." << std::endl;
    int64_t runstart{20000000000}; // start run date from Mantid's Epoch, in nanoseconds
    int64_t pulsedt{1000000};      // time between consecutive pulses, in nanoseconds

    EventWorkspace_sptr ws = createEventWorkspaceElastic(runstart, pulsedt);
    AnalysisDataService::Instance().addOrReplace("MockElasticEventWS", ws);
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 10000);

    // Create SplittersWorkspace with 8 destination-workspaces plus the unfiltered workspace
    // index            DateandTime
    //  2     1990-Jan-01 00:00:20.001000000
    //  5     1990-Jan-01 00:00:20.001300000
    //  4     1990-Jan-01 00:00:20.002000000
    // -1     1990-Jan-01 00:00:20.002190000
    //  6     1990-Jan-01 00:00:20.004000000
    //  7     1990-Jan-01 00:00:20.005000000
    //  8     1990-Jan-01 00:00:20.005500000
    // -1     1990-Jan-01 00:00:20.007000000
    //  1     1990-Jan-01 00:00:20.008000000
    //  3     1990-Jan-01 00:00:20.009000000
    // -1     1990-Jan-01 00:00:20.010000000
    MatrixWorkspace_sptr splws = createMatrixSplittersElastic(runstart);
    AnalysisDataService::Instance().addOrReplace("SplitterTableX", splws);

    // Run the filtering
    FilterEvents filter;
    filter.initialize();

    filter.setPropertyValue("InputWorkspace", "MockElasticEventWS");
    filter.setProperty("OutputWorkspaceBaseName", "SplittedDataElastic");
    filter.setProperty("CorrectionToSample", "Elastic");
    filter.setProperty("SplitterWorkspace", "SplitterTableX");
    filter.setProperty("OutputUnfilteredEvents", true);

    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    // Check number of output workspaces
    std::vector<std::string> vecwsname = filter.getProperty("OutputWorkspaceNames");
    TS_ASSERT_EQUALS(vecwsname.size(), 9);

    EventWorkspace_sptr ws5 =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("SplittedDataElastic_5"));
    TS_ASSERT(ws5);
    if (ws5) {
      TS_ASSERT_EQUALS(ws5->getNumberEvents(), 0);
    }

    EventWorkspace_sptr ws7 =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("SplittedDataElastic_7"));
    TS_ASSERT(ws7);
    if (ws7) {
      TS_ASSERT_EQUALS(ws7->getNumberEvents(), 10);
    }

    // Check individual events
    EventList &ev0 = ws7->getSpectrum(0);
    TS_ASSERT_EQUALS(ev0.getNumberEvents(), 1);
    std::vector<double> vectofs = ev0.getTofs();
    TS_ASSERT_DELTA(vectofs[0], 272.0, 0.001);

    // Delete all the workspaces generated here
    AnalysisDataService::Instance().remove("MockDirectEventWS");
    AnalysisDataService::Instance().remove("SplitterTableX");
    for (const auto &workspaceName : vecwsname) {
      AnalysisDataService::Instance().remove(workspaceName);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test filtering with correction of direct geometry
   */
  void test_FilterDGCorrection() {
    std::cout << std::endl << "test_FilterDGCorrection..." << std::endl;
    int64_t runstart{20000000000}; // start run date from Mantid's Epoch, in nanoseconds
    int64_t pulsedt{1000000};      // time between consecutive pulses, in nanoseconds

    EventWorkspace_sptr ws = createEventWorkspaceDirect(runstart, pulsedt);
    AnalysisDataService::Instance().addOrReplace("MockDirectEventWS", ws);

    MatrixWorkspace_sptr splws = createMatrixSplittersDG(runstart);
    AnalysisDataService::Instance().addOrReplace("SplitterTableX", splws);

    // Run the filtering
    FilterEvents filter;
    filter.initialize();

    filter.setProperty("InputWorkspace", ws->getName());
    filter.setProperty("OutputWorkspaceBaseName", "SplittedDataDG");
    filter.setProperty("CorrectionToSample", "Direct");
    filter.setProperty("SplitterWorkspace", "SplitterTableX");

    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    // Check
    EventWorkspace_sptr ws5 =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("SplittedDataDG_5"));
    TS_ASSERT(ws5);
    if (ws5) {
      TS_ASSERT_EQUALS(ws5->getNumberEvents(), 0);
    }

    EventWorkspace_sptr ws7 =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("SplittedDataDG_7"));
    TS_ASSERT(ws7);
    if (ws7) {
      TS_ASSERT_EQUALS(ws7->getNumberEvents(), ws7->getNumberHistograms());
    }

    // FIXME - Should find a way to delete all workspaces holding splitted
    // events

    AnalysisDataService::Instance().remove("MockDirectEventWS");
    AnalysisDataService::Instance().remove("SplitterTableX");
    std::vector<std::string> outputwsnames = filter.getProperty("OutputWorkspaceNames");
    for (const auto &outputwsname : outputwsnames) {
      AnalysisDataService::Instance().remove(outputwsname);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test filtering with correction to indirect geometry inelastic instrument
   */
  void test_FilterIndirectGeometryCorrection() {
    std::cout << std::endl << "test_FilterIndirectGeometryCorrection..." << std::endl;
    int64_t runstart{20000000000}; // start run date from Mantid's Epoch, in nanoseconds
    int64_t pulsedt{1000000};      // time between consecutive pulses, in nanoseconds

    EventWorkspace_sptr ws = createEventWorkspaceInDirect(runstart, pulsedt);
    AnalysisDataService::Instance().addOrReplace("MockIndirectEventWS", ws);

    MatrixWorkspace_sptr splws = createMatrixSplittersDG(runstart);
    AnalysisDataService::Instance().addOrReplace("SplitterTableX", splws);

    // Run the filtering
    FilterEvents filter;
    filter.initialize();

    filter.setProperty("InputWorkspace", "MockIndirectEventWS");
    filter.setProperty("OutputWorkspaceBaseName", "SplittedDataDG");
    filter.setProperty("CorrectionToSample", "Indirect");
    filter.setProperty("SplitterWorkspace", "SplitterTableX");
    filter.setProperty("OutputTOFCorrectionWorkspace", "MockIndGeoCorrWS");

    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    // Check
    MatrixWorkspace_sptr outcorrws =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("MockIndGeoCorrWS"));
    TS_ASSERT(outcorrws);
    if (outcorrws) {
      TS_ASSERT_EQUALS(outcorrws->getNumberHistograms(), ws->getNumberHistograms());
      TS_ASSERT_EQUALS(outcorrws->x(0).size(), 2);

      const auto &spectrumInfo = ws->spectrumInfo();

      for (size_t iws = 0; iws < outcorrws->getNumberHistograms(); ++iws) {
        const ParameterMap &pmap = ws->constInstrumentParameters();

        const auto &det = spectrumInfo.detector(iws);
        Parameter_sptr par = pmap.getRecursive(&det, "Efixed");
        double efix = par->value<double>();

        double l2 = spectrumInfo.l2(iws);

        double shift = -l2 / sqrt(efix * 2. * PhysicalConstants::meV / PhysicalConstants::NeutronMass);

        TS_ASSERT_DELTA(outcorrws->y(iws)[0], 1., 1.0E-9);
        TS_ASSERT_DELTA(outcorrws->y(iws)[1], shift, 1.0E-9);
      }
    }

    // Clean
    AnalysisDataService::Instance().remove("MockIndirectEventWS");
    std::vector<std::string> outputwsnames = filter.getProperty("OutputWorkspaceNames");
    for (const auto &outputwsname : outputwsnames) {
      AnalysisDataService::Instance().remove(outputwsname);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /**  Filter events without any correction and test for splitters in
   *MatrixWorkspace format
   *   and the time given for splitters is relative
   *  Event workspace:
   * (1) 10 detectors
    * (2) Run starts @ 20000000000 seconds
    * (3) Pulse length = 100*1000*1000 seconds
    * (4) Within one pulse, two consecutive events/neutrons is apart for
   *10*1000*1000 seconds
    * (5) "Experiment": 5 pulse times.  10 events in each pulse
    *
    * In this test
   *  (1) Leave correction table workspace empty
   *  (2) Count events in each output including "-1", the excluded/unselected
   *      events
   *
   * Splitter-log test:
        979: 0: 0         -  3.5e+07:  0
        979: 1: 3.5e+07   -  1.95e+08: 1
        979: 2: 1.95e+08  -  2.65e+08: 2
        979: 3: 2.65e+08  -  3.65e+08: 2
        979: 4: 3.65e+08  -  4.65e+08: 2
   */
  void test_FilterRelativeTime() {
    std::cout << std::endl << "test_FilterRelativeTime..." << std::endl;
    // Create EventWorkspace and SplittersWorkspace
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100 * 1000 * 1000;
    int64_t tofdt = 10 * 1000 * 1000;
    size_t numpulses = 5;

    EventWorkspace_sptr inpWS = createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("Test10", inpWS);

    // Create SplittersWorkspace with two destination workspaces plus the unfiltered workspace
    // index    Relative DateandTime     time-int64_t
    //   0    1990-Jan-01 00:00:00.000
    //   1    1990-Jan-01 00:00:00.035
    //   2    1990-Jan-01 00:00:00.195
    //  -1    1990-Jan-01 00:00:00.465
    API::MatrixWorkspace_sptr splws = createMatrixSplitter(0, pulsedt, tofdt);
    AnalysisDataService::Instance().addOrReplace("Splitter10", splws);
    TimeSplitter timeSplitter = TimeSplitter(splws);
    std::string printout{timeSplitter.debugPrint()};

    FilterEvents filter;
    filter.initialize();

    // Set properties
    filter.setProperty("InputWorkspace", "Test10");
    filter.setProperty("OutputWorkspaceBaseName", "FilteredWS10");
    filter.setProperty("SplitterWorkspace", "Splitter10");
    filter.setProperty("RelativeTime", true);
    filter.setProperty("OutputUnfilteredEvents", true);
    filter.setProperty("OutputWorkspaceIndexedFrom1", false);

    // Execute
    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    // Get 3 output workspaces
    int numsplittedws = filter.getProperty("NumberOutputWS");
    TS_ASSERT_EQUALS(numsplittedws, 4);

    std::vector<std::string> output_ws_vector = filter.getProperty("OutputWorkspaceNames");
    for (size_t i = 0; i < output_ws_vector.size(); ++i)
      std::cout << "Output workspace " << i << ": " << output_ws_vector[i] << "\n";

    // Workspace 0
    EventWorkspace_sptr filteredws0 =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS10_0"));
    TS_ASSERT(filteredws0);
    TS_ASSERT_EQUALS(filteredws0->getNumberHistograms(), 10);
    TS_ASSERT_EQUALS(filteredws0->getSpectrum(0).getNumberEvents(), 4);
    TS_ASSERT_EQUALS(filteredws0->run().getProtonCharge(), 1);
    TS_ASSERT_EQUALS(filteredws0->run().getTimeROI().debugStrPrint(),
                     "0: 1990-Jan-01 00:00:20 to 1990-Jan-01 00:00:20.035000000\n");
    TS_ASSERT_DELTA(std::stod(filteredws0->run().getProperty("duration")->value()), 0.035, 1.E-9);

    // Workspace 1
    EventWorkspace_sptr filteredws1 =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS10_1"));
    TS_ASSERT(filteredws1);
    TS_ASSERT_EQUALS(filteredws1->getSpectrum(1).getNumberEvents(), 16);
    TS_ASSERT_EQUALS(filteredws1->run().getProtonCharge(), 1);
    TS_ASSERT_EQUALS(filteredws1->run().getTimeROI().debugStrPrint(),
                     "0: 1990-Jan-01 00:00:20.035000000 to 1990-Jan-01 00:00:20.195000000\n");
    TS_ASSERT_DELTA(std::stod(filteredws1->run().getProperty("duration")->value()), 0.160, 1.E-9);

    // Workspace 2
    EventWorkspace_sptr filteredws2 =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS10_2"));
    TS_ASSERT(filteredws2);
    TS_ASSERT_EQUALS(filteredws2->getSpectrum(1).getNumberEvents(), 27);

    TS_ASSERT_EQUALS(filteredws2->run().getProtonCharge(), 3);
    TS_ASSERT_EQUALS(filteredws2->run().getTimeROI().debugStrPrint(),
                     "0: 1990-Jan-01 00:00:20.195000000 to 1990-Jan-01 00:00:20.465000000\n");
    TS_ASSERT_DELTA(std::stod(filteredws2->run().getProperty("duration")->value()), 0.270, 1.E-9);

    // Unfiltered workspace
    EventWorkspace_sptr unfilteredws =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS10_unfiltered"));
    TS_ASSERT(unfilteredws);
    TS_ASSERT_EQUALS(unfilteredws->getSpectrum(1).getNumberEvents(), 3);
    TS_ASSERT_EQUALS(unfilteredws->run().getProtonCharge(), 0);
    TS_ASSERT_EQUALS(unfilteredws->run().getTimeROI().debugStrPrint(),
                     "0: 1990-Jan-01 00:00:20.465000000 to 1990-Jan-01 00:00:20.500000000\n");
    TS_ASSERT_DELTA(std::stod(unfilteredws->run().getProperty("duration")->value()), 0.035, 1.E-9);

    // Check spectrum 3 of workspace 2
    EventList elist3 = filteredws2->getSpectrum(3);
    elist3.sortPulseTimeTOF();

    TofEvent eventmin = elist3.getEvent(0);
    TS_ASSERT_EQUALS(eventmin.pulseTime().totalNanoseconds(), runstart_i64 + pulsedt * 2);
    TS_ASSERT_DELTA(eventmin.tof(), 0, 1.0E-4);

    TofEvent eventmax = elist3.getEvent(26);
    TS_ASSERT_EQUALS(eventmax.pulseTime().totalNanoseconds(), runstart_i64 + pulsedt * 4);
    TS_ASSERT_DELTA(eventmax.tof(), static_cast<double>(tofdt * 6 / 1000), 1.0E-4);

    std::vector<std::string> outputwsnames = filter.getProperty("OutputWorkspaceNames");

    //  Test the sample logs
    for (const auto &outputwsname : outputwsnames) {
      EventWorkspace_sptr filtered_ws = std::dynamic_pointer_cast<DataObjects::EventWorkspace>(
          AnalysisDataService::Instance().retrieve(outputwsname));

      TS_ASSERT(filtered_ws->run().hasProperty("LogA"));
      TS_ASSERT(filtered_ws->run().hasProperty("LogB"));
      TS_ASSERT(filtered_ws->run().hasProperty("LogC"));
      Kernel::Property *logA = filtered_ws->run().getProperty("LogA");
      std::string valueA = logA->value();
      TS_ASSERT_EQUALS(valueA.compare("A"), 0);

      TS_ASSERT(filtered_ws->run().hasProperty("slow_int_log"));
      Kernel::TimeSeriesProperty<int> *intlog =
          dynamic_cast<Kernel::TimeSeriesProperty<int> *>(filtered_ws->run().getProperty("slow_int_log"));
      TS_ASSERT(intlog);
      TS_ASSERT_EQUALS(intlog->units(), "meter");
    }

    // clean up all the workspaces generated
    AnalysisDataService::Instance().remove("Test10");
    AnalysisDataService::Instance().remove("Splitter10");
    for (const auto &outputwsname : outputwsnames) {
      AnalysisDataService::Instance().remove(outputwsname);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /**  Filter events without any correction and test for splitters in
   *    TableWorkspace filter format
   *    and the time given for splitters is relative
   *
   *  It is exactly the same as unit test: test_FilterRelativeTime()
   *
   *  Event workspace:
   * (1) 10 detectors
   * (2) Run starts @ 20000000000 seconds
   * (3) Pulse length = 100*1000*1000 seconds
   * (4) Within one pulse, two consecutive events/neutrons is apart for
   * 10*1000*1000 seconds
   * (5) "Experiment": 5 pulse times.  10 events in each pulse
   *
   * In this test
   *  (1) Leave correction table workspace empty
   *  (2) Count events in each output including "-1", the excluded/unselected
   * events
   */
  void test_tableSplitter() {
    std::cout << std::endl << "test_tableSplitter..." << std::endl;
    // Create EventWorkspace and SplittersWorkspace
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100 * 1000 * 1000;
    int64_t tofdt = 10 * 1000 * 1000;
    size_t numpulses = 5;

    EventWorkspace_sptr inpWS = createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("Test11", inpWS);

    DataObjects::TableWorkspace_sptr splws = createTableSplitters(0, pulsedt, tofdt);
    AnalysisDataService::Instance().addOrReplace("TableSplitter1", splws);

    FilterEvents filter;
    filter.initialize();

    // Set properties
    filter.setProperty("InputWorkspace", "Test11");
    filter.setProperty("OutputWorkspaceBaseName", "FilteredWS_FromTable");
    filter.setProperty("SplitterWorkspace", "TableSplitter1");
    filter.setProperty("RelativeTime", true);
    filter.setProperty("OutputUnfilteredEvents", true);
    filter.setProperty("OutputWorkspaceIndexedFrom1", true);

    // Execute
    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    // Get output workspaces
    int numsplittedws = filter.getProperty("NumberOutputWS");
    TS_ASSERT_EQUALS(numsplittedws, 4);

    std::vector<std::string> output_ws_vector = filter.getProperty("OutputWorkspaceNames");
    for (size_t i = 0; i < output_ws_vector.size(); ++i)
      std::cout << "Output workspace " << i << ": " << output_ws_vector[i] << "\n";

    // Workspace 0
    EventWorkspace_sptr filteredws0 =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS_FromTable_A"));
    TS_ASSERT(filteredws0);
    TS_ASSERT_EQUALS(filteredws0->getNumberHistograms(), 10);
    TS_ASSERT_EQUALS(filteredws0->getSpectrum(0).getNumberEvents(), 4);
    TS_ASSERT_EQUALS(filteredws0->run().getProtonCharge(), 1);
    TS_ASSERT_EQUALS(filteredws0->run().getTimeROI().debugStrPrint(),
                     "0: 1990-Jan-01 00:00:20 to 1990-Jan-01 00:00:20.035000000\n");
    TS_ASSERT_DELTA(std::stod(filteredws0->run().getProperty("duration")->value()), 0.035, 1.E-9);

    // Workspace 1
    EventWorkspace_sptr filteredws1 =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS_FromTable_B"));
    TS_ASSERT(filteredws1);
    TS_ASSERT_EQUALS(filteredws1->getSpectrum(1).getNumberEvents(), 16);
    TS_ASSERT_EQUALS(filteredws1->run().getProtonCharge(), 1);
    TS_ASSERT_EQUALS(filteredws1->run().getTimeROI().debugStrPrint(),
                     "0: 1990-Jan-01 00:00:20.035000000 to 1990-Jan-01 00:00:20.195000000\n");
    TS_ASSERT_DELTA(std::stod(filteredws1->run().getProperty("duration")->value()), 0.160, 1.E-9);

    // Workspace 2
    EventWorkspace_sptr filteredws2 =
        std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("FilteredWS_FromTable_C"));
    TS_ASSERT(filteredws2);
    TS_ASSERT_EQUALS(filteredws2->getSpectrum(1).getNumberEvents(), 27);
    TS_ASSERT_EQUALS(filteredws2->run().getProtonCharge(), 3);
    TS_ASSERT_EQUALS(filteredws2->run().getTimeROI().debugStrPrint(),
                     "0: 1990-Jan-01 00:00:20.195000000 to 1990-Jan-01 00:00:20.465000000\n");
    TS_ASSERT_DELTA(std::stod(filteredws2->run().getProperty("duration")->value()), 0.270, 1.E-9);

    // unfiltered workspace
    EventWorkspace_sptr unfilteredws = std::dynamic_pointer_cast<EventWorkspace>(
        AnalysisDataService::Instance().retrieve("FilteredWS_FromTable_unfiltered"));
    TS_ASSERT(unfilteredws);
    TS_ASSERT_EQUALS(unfilteredws->getSpectrum(1).getNumberEvents(), 3);
    TS_ASSERT_EQUALS(unfilteredws->run().getProtonCharge(), 0);
    TS_ASSERT_EQUALS(unfilteredws->run().getTimeROI().debugStrPrint(),
                     "0: 1990-Jan-01 00:00:20.465000000 to 1990-Jan-01 00:00:20.500000000\n");
    TS_ASSERT_DELTA(std::stod(unfilteredws->run().getProperty("duration")->value()), 0.035, 1.E-9);

    // Check spectrum 3 of workspace 2
    EventList elist3 = filteredws2->getSpectrum(3);
    elist3.sortPulseTimeTOF();

    TofEvent eventmin = elist3.getEvent(0);
    TS_ASSERT_EQUALS(eventmin.pulseTime().totalNanoseconds(), runstart_i64 + pulsedt * 2);
    TS_ASSERT_DELTA(eventmin.tof(), 0, 1.0E-4);

    TofEvent eventmax = elist3.getEvent(26);
    TS_ASSERT_EQUALS(eventmax.pulseTime().totalNanoseconds(), runstart_i64 + pulsedt * 4);
    TS_ASSERT_DELTA(eventmax.tof(), static_cast<double>(tofdt * 6 / 1000), 1.0E-4);

    // Clean up the generated workspaces
    AnalysisDataService::Instance().remove("Test11");
    AnalysisDataService::Instance().remove("TableSplitter1");
    std::vector<std::string> outputwsnames = filter.getProperty("OutputWorkspaceNames");
    for (const auto &outputwsname : outputwsnames) {
      AnalysisDataService::Instance().remove(outputwsname);
    }

    return;
  }

  /** Test the feature to exclude some sample logs to be split and add to child
   * workspaces
   * @brief test_excludeSampleLogs
   */
  // BUGFIX: excluding logs leads to Segmentation Fault! (rename "excludeSampleLogs" to "test_excludeSampleLogs")

  /**
  void excludeSampleLogs() {
    std::cout << std::endl << "test_excludeSampleLogs..." << std::endl;
    // Create EventWorkspace and SplittersWorkspace
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100 * 1000 * 1000;
    int64_t tofdt = 10 * 1000 * 1000;
    size_t numpulses = 5;

    EventWorkspace_sptr inpWS = createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("Test12", inpWS);

    DataObjects::TableWorkspace_sptr splws = createTableSplitters(0, pulsedt, tofdt);
    AnalysisDataService::Instance().addOrReplace("TableSplitter2", splws);

    FilterEvents filter;
    filter.initialize();

    // Set properties
    filter.setProperty("InputWorkspace", "Test12");
    filter.setProperty("OutputWorkspaceBaseName", "FilteredFromTable");
    filter.setProperty("SplitterWorkspace", "TableSplitter2");
    filter.setProperty("RelativeTime", true);
    filter.setProperty("OutputWorkspaceIndexedFrom1", true);

    std::vector<std::string> prop_vec;
    prop_vec.emplace_back("LogB");
    prop_vec.emplace_back("slow_int_log");
    filter.setProperty("TimeSeriesPropertyLogs", prop_vec);
    filter.setProperty("ExcludeSpecifiedLogs", true);

    // Execute
    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    // Get 3 output workspaces
    int numsplittedws = filter.getProperty("NumberOutputWS");
    TS_ASSERT_EQUALS(numsplittedws, 3);

    // check number of sample logs
    size_t num_original_logs = inpWS->run().getProperties().size();

    std::vector<std::string> outputwsnames = filter.getProperty("OutputWorkspaceNames");
    for (const auto &outputwsname : outputwsnames) {
      EventWorkspace_sptr childworkspace =
          std::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(outputwsname));
      TS_ASSERT(childworkspace);
      // there is 1 sample logs that is excluded from propagating to the child
      // workspaces. LogB is not TSP, so it won't be excluded even if it is
      // listed
      // a new TSP splitter is added by FilterEvents. So there will be exactly
      // same number, but some different, sample logs in the input and output
      // workspaces

      TS_ASSERT_EQUALS(num_original_logs, childworkspace->run().getProperties().size());
    }

    // clean workspaces
    AnalysisDataService::Instance().remove("Test12");
    AnalysisDataService::Instance().remove("TableSplitter2");
    for (const auto &outputwsname : outputwsnames) {
      AnalysisDataService::Instance().remove(outputwsname);
    }
    return;
  }
  */

  /** test for the case that the input workspace name is same as output base
   * workspace name
   * @brief test_ThrowSameName
   */
  void test_ThrowSameName() {
    std::cout << std::endl << "test_ThrowSameName..." << std::endl;
    // Create EventWorkspace and SplittersWorkspace
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100 * 1000 * 1000;
    int64_t tofdt = 10 * 1000 * 1000;
    size_t numpulses = 5;

    EventWorkspace_sptr inpWS = createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("Test13", inpWS);

    DataObjects::TableWorkspace_sptr splws = createTableSplitters(0, pulsedt, tofdt);
    AnalysisDataService::Instance().addOrReplace("TableSplitter2", splws);

    FilterEvents filter;
    filter.initialize();

    // Set properties
    filter.setProperty("InputWorkspace", "Test13");
    filter.setProperty("OutputWorkspaceBaseName", "Test13");
    filter.setProperty("SplitterWorkspace", "TableSplitter2");
    filter.setProperty("RelativeTime", true);
    filter.setProperty("GroupWorkspaces", true);

    // Execute
    TS_ASSERT(!filter.execute());

    // clean workspaces
    AnalysisDataService::Instance().remove("Test13");
    AnalysisDataService::Instance().remove("TableSplitter2");

    return;
  }

  /** test for the case that the input workspace name is same as output base
   * workspace name
   * @brief test_ThrowSameName
   */
  void test_groupWorkspaces() {
    std::cout << std::endl << "test_groupWorkspaces..." << std::endl;
    // Create EventWorkspace and SplittersWorkspace
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100 * 1000 * 1000;
    int64_t tofdt = 10 * 1000 * 1000;
    size_t numpulses = 5;

    EventWorkspace_sptr inpWS = createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("Test13", inpWS);

    DataObjects::TableWorkspace_sptr splws = createTableSplitters(0, pulsedt, tofdt);
    AnalysisDataService::Instance().addOrReplace("TableSplitter2", splws);

    FilterEvents filter;
    filter.initialize();

    // Set properties
    filter.setProperty("InputWorkspace", "Test13");
    filter.setProperty("OutputWorkspaceBaseName", "13");
    filter.setProperty("SplitterWorkspace", "TableSplitter2");
    filter.setProperty("RelativeTime", true);
    filter.setProperty("GroupWorkspaces", true);

    // Execute
    TS_ASSERT(filter.execute());

    // clean workspaces
    AnalysisDataService::Instance().remove("Test13");
    AnalysisDataService::Instance().remove("TableSplitter2");

    return;
  }

  /** test for the case that the input workspace names are of the form
   * basename_startTime_stopTime
   */
  void test_descriptiveWorkspaceNamesTime() {
    std::cout << std::endl << "test_descriptiveWorkspaceNamesTime..." << std::endl;
    // Create EventWorkspace and SplittersWorkspace
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100 * 1000 * 1000;
    int64_t tofdt = 10 * 1000 * 1000;
    size_t numpulses = 5;

    EventWorkspace_sptr inpWS = createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("InputWS", inpWS);

    SplittersWorkspace_sptr splws = std::make_shared<SplittersWorkspace>();
    for (int i = 0; i < 5; i++) {
      auto t0 = runstart_i64 + i * pulsedt;
      auto t1 = runstart_i64 + (i + 1) * pulsedt;
      Kernel::SplittingInterval interval2(t0, t1, i);
      splws->addSplitter(interval2);
    }
    AnalysisDataService::Instance().addOrReplace("TableSplitter", splws);

    TableWorkspace_sptr infows = createInformationWorkspace(3, true);
    AnalysisDataService::Instance().addOrReplace("InfoWorkspace", infows);

    FilterEvents filter;
    filter.initialize();

    // Set properties
    filter.setProperty("InputWorkspace", "InputWS");
    filter.setProperty("OutputWorkspaceBaseName", "BaseName");
    filter.setProperty("SplitterWorkspace", "TableSplitter");
    filter.setProperty("InformationWorkspace", "InfoWorkspace");
    filter.setProperty("RelativeTime", true);
    filter.setProperty("DescriptiveOutputNames", true);
    filter.setProperty("OutputUnfilteredEvents", true);

    // Execute
    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    std::vector<std::string> output_ws_vector = filter.getProperty("OutputWorkspaceNames");

    std::string expectedName = "BaseName_unfiltered";
    TS_ASSERT_EQUALS(output_ws_vector[0], expectedName);

    for (int i = 0; i < 5; i++) {
      auto t0 = static_cast<double>(i * pulsedt) / 1.E9;
      auto t1 = static_cast<double>((i + 1) * pulsedt) / 1.E9;
      std::stringstream expectedName;
      expectedName << "BaseName_" << t0 << "_" << t1;
      TS_ASSERT_EQUALS(output_ws_vector[i + 1], expectedName.str());
    }

    // clean workspaces
    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("TableSplitter");
    AnalysisDataService::Instance().remove("InfoWorkspace");

    return;
  }

  /** test for the case that the input workspace names are of the form
   * basename_startTime_stopTime
   */
  void test_descriptiveWorkspaceNamesLog() {
    std::cout << std::endl << "test_descriptiveWorkspaceNamesLog..." << std::endl;
    // Create EventWorkspace and SplittersWorkspace
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100 * 1000 * 1000;
    int64_t tofdt = 10 * 1000 * 1000;
    size_t numpulses = 5;

    EventWorkspace_sptr inpWS = createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("InputWS", inpWS);

    DataObjects::TableWorkspace_sptr splws = createSplittersWorkspace(0, pulsedt, tofdt);
    AnalysisDataService::Instance().addOrReplace("TableSplitter", splws);

    DataObjects::TableWorkspace_sptr infows = createInformationWorkspace(3, false);
    AnalysisDataService::Instance().addOrReplace("InfoWorkspace", infows);

    FilterEvents filter;
    filter.initialize();

    // Set properties
    filter.setProperty("InputWorkspace", "InputWS");
    filter.setProperty("OutputWorkspaceBaseName", "BaseName");
    filter.setProperty("SplitterWorkspace", "TableSplitter");
    filter.setProperty("InformationWorkspace", "InfoWorkspace");
    filter.setProperty("RelativeTime", true);
    filter.setProperty("DescriptiveOutputNames", true);
    filter.setProperty("OutputUnfilteredEvents", true);

    // Execute
    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    std::vector<std::string> output_ws_vector = filter.getProperty("OutputWorkspaceNames");

    std::string expectedName = "BaseName_unfiltered";
    TS_ASSERT_EQUALS(output_ws_vector[0], expectedName);

    for (int i = 0; i < 3; i++) {
      std::stringstream expectedName;
      expectedName << "BaseName_"
                   << "Log.proton_charge.From.start" << i << ".To.end" << i << ".Value-change-direction:both";
      TS_ASSERT_EQUALS(output_ws_vector[i + 1], expectedName.str());
    }

    // clean workspaces
    AnalysisDataService::Instance().remove("InputWS");
    AnalysisDataService::Instance().remove("TableSplitter");
    AnalysisDataService::Instance().remove("InfoWorkspace");

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Create an EventWorkspace.  This workspace has
   * @param runstart_i64 : absolute run start time in int64_t format with unit of nanoseconds
   * @param pulsedt : time between consecutive pulses in int64_t format with unit nanosecond
   * @param todft : time interval between 2 adjacent events in same pulse in int64_t format of unit nanosecond
   * @param numpulses : number of pulses in the event workspace
   */
  EventWorkspace_sptr createEventWorkspace(int64_t runstart_i64, int64_t pulsedt, int64_t tofdt, size_t numpulses) {

    constexpr int64_t N_EVENTS_IN_PULSE{10};
    constexpr double nanosecToMilisec{0.001};
    constexpr double nanosecToSec{1e-9};
    // Validation: the TOF of the last event cannot exceed pulsedt, lest we incur in frame overlap.
    if (tofdt * N_EVENTS_IN_PULSE > pulsedt)
      throw std::invalid_argument("tofdt cannot exceed pulsedt / N_EVENTS_IN_PULSE");

    // Create an EventWorkspace with 10 detectors, one pixel per detector, and no events
    int detectorBankCount{10};
    int pixelPerBankcount{1};
    bool clearEvents{true};
    EventWorkspace_sptr eventWS = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(
        detectorBankCount, pixelPerBankcount, clearEvents);

    // create a vector with all the epoch pulse times
    DateAndTime runstart(runstart_i64);
    std::vector<DateAndTime> pulseTimes;
    for (int64_t pid = 0; pid < static_cast<int64_t>(numpulses); pid++) {
      int64_t pulsetime_i64 = pid * pulsedt + runstart.totalNanoseconds();
      DateAndTime pulsetime(pulsetime_i64);
      pulseTimes.emplace_back(pulsetime);
    }

    // create a vector with all events. These will be the same for all detector pixels
    std::vector<TofEvent> events;
    for (const auto &pulseTime : pulseTimes)
      for (int64_t e = 0; e < N_EVENTS_IN_PULSE; e++) {
        double tof = nanosecToMilisec * static_cast<double>(e * tofdt); // TOF in miliseconds
        TofEvent event(tof, pulseTime);
        events.emplace_back(event);
      }

    // Iterate over all the spectra (detectorBankCount * pixelPerBankcount)
    // Populate its event list using the previous vector of events
    for (size_t histIndex = 0; histIndex < eventWS->getNumberHistograms(); histIndex++) {
      auto &elist = eventWS->getSpectrum(histIndex); // event list
      for (const auto &event : events)
        elist.addEventQuickly(event);
    }

    // Insert a sample log for the start and end of the run, with one charge count per pulse
    eventWS->mutableRun().addProperty("run_start", runstart.toISO8601String(), true);
    double duration = nanosecToSec * static_cast<double>(pulsedt * numpulses);
    DateAndTime runEnd = runstart + duration;
    eventWS->mutableRun().addProperty("run_end", runEnd.toISO8601String(), true);

    // Insert a sample log for the proton charge
    auto pchargeLog = std::make_unique<Kernel::TimeSeriesProperty<double>>("proton_charge");
    for (const auto &pulseTime : pulseTimes)
      pchargeLog->addValue(pulseTime, 1.);
    eventWS->mutableRun().addLogData(pchargeLog.release());
    eventWS->mutableRun().integrateProtonCharge();

    // add single value logs, which won't be need to split
    eventWS->mutableRun().addProperty(new Kernel::PropertyWithValue<std::string>("LogA", "A"));
    eventWS->mutableRun().addProperty(new Kernel::PropertyWithValue<std::string>("LogB", "B"));
    eventWS->mutableRun().addProperty(new Kernel::PropertyWithValue<std::string>("LogC", "C"), true);
    eventWS->mutableRun().addProperty(new Kernel::PropertyWithValue<std::string>("Title", "Testing EventWorkspace"));
    eventWS->mutableRun().addProperty(new Kernel::PropertyWithValue<double>("duration", duration));

    // add time series log. The TimeRoi associated to the Run object of each target workspace
    // will be applied when finding out its conserved values.
    auto int_tsp = std::make_unique<Kernel::TimeSeriesProperty<int>>("slow_int_log");
    int_tsp->setUnits("meter");
    int entriesCount{10};                           // this log will have 10 entries
    double probingPeriod = duration / entriesCount; // time in between consecutive log entries
    DateAndTime logTime = runstart;
    for (int i = 0; i < entriesCount; ++i) {
      int logValue = (i + 1) * 20;
      int_tsp->addValue(logTime, logValue);
      logTime += probingPeriod;
    }
    eventWS->mutableRun().addLogData(int_tsp.release());

    return eventWS;
  }

  //----------------------------------------------------------------------------------------------
  /** Create an EventWorkspace to mimic direct inelastic scattering insturment.
   * This workspace will have the same neutron events as the test case in EventList.
   * @param runstart_i64 : start run date from Mantid's Epoch time, in nanoseconds
   * @param pulsedt : time between consecutive pulses, in nanoseconds
   * @param pulseCount : number of pulses
   */
  EventWorkspace_sptr createEventWorkspaceDirect(int64_t runstart_i64, int64_t pulsedt, int64_t pulseCount = 1000) {
    // Create an EventWorkspace with 10 banks with 1 detector each.  No events are generated
    EventWorkspace_sptr eventWS = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(10, 1, true);

    // L1 = 10
    const auto &spectrumInfo = eventWS->spectrumInfo();
    double l1 = spectrumInfo.l1();

    Types::Core::DateAndTime runstart(runstart_i64);

    // Create `pulseCount` pulses with one event per pulse
    EventList fakeevlist = fake_uniform_time_sns_data(runstart_i64, pulsedt, pulseCount);
    Types::Core::DateAndTime runend(runstart + pulseCount * pulsedt);

    // Set properties: (1) run_start time; (2) Ei
    eventWS->mutableRun().addProperty("run_start", runstart.toISO8601String(), true);
    eventWS->mutableRun().addProperty("run_end", runend.toISO8601String(), true);

    double shift = 2.E-4;
    double ei = (l1 * l1 * PhysicalConstants::NeutronMass) / (shift * shift * 2. * PhysicalConstants::meV);

    eventWS->mutableRun().addProperty<double>("Ei", ei, true);

    // Add neutrons
    for (size_t i = 0; i < eventWS->getNumberHistograms(); i++) {
      auto &elist = eventWS->getSpectrum(i);

      for (size_t ievent = 0; ievent < fakeevlist.getNumberEvents(); ++ievent) {
        TofEvent tofevent = fakeevlist.getEvent(ievent);
        elist.addEventQuickly(tofevent);
      } // FOR each pulse
    }   // For each bank

    eventWS->mutableRun().integrateProtonCharge();

    // double constshift = l1 / sqrt(ei * 2. * PhysicalConstants::meV /
    //                           PhysicalConstants::NeutronMass);

    return eventWS;
  }

  /** Create an EventWorkspace to mimic direct inelastic scattering instrument
   * @param runstart_i64 : start run date from Mantid's Epoch time, in nanoseconds
   * @param pulsedt : time between consecutive pulses, in nanoseconds
   * @param pulseCount : number of pulses
   */
  EventWorkspace_sptr createEventWorkspaceInDirect(int64_t runstart_i64, int64_t pulsedt, int64_t pulseCount = 1000) {
    // Create an EventWorkspace with 10 banks with 1 detector each.  No events are generated
    EventWorkspace_sptr eventWS = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(10, 1, true);

    // Properties for the start and end of the run
    Types::Core::DateAndTime runstart(runstart_i64);
    Types::Core::DateAndTime runend(runstart + pulseCount * pulsedt);
    eventWS->mutableRun().addProperty("run_start", runstart.toISO8601String(), true);
    eventWS->mutableRun().addProperty("run_end", runend.toISO8601String(), true);

    // Add EFixed to each detector
    const ParameterMap &pmap = eventWS->constInstrumentParameters();

    for (size_t i = 0; i < 10; ++i) {

      const auto &spectrumInfo = eventWS->spectrumInfo();

      const auto &det = spectrumInfo.detector(i);
      Parameter_sptr par = pmap.getRecursive(&det, "Efixed");
      if (par) {
        // No need to set up E-Fix
        // double efix = par->value<double>();
        ;
      } else {

        eventWS->setEFixed(det.getID(), 2.08);
      }
    }

    // Create `pulseCount` pulses with one event per pulse
    EventList fakeevlist = fake_uniform_time_sns_data(runstart_i64, pulsedt, pulseCount);
    for (size_t i = 0; i < eventWS->getNumberHistograms(); i++) {
      auto &elist = eventWS->getSpectrum(i);

      for (size_t ievent = 0; ievent < fakeevlist.getNumberEvents(); ++ievent) {
        TofEvent tofevent = fakeevlist.getEvent(ievent);
        elist.addEventQuickly(tofevent);
      } // FOR each pulse
    }   // For each bank

    return eventWS;
  }

  //----------------------------------------------------------------------------------------------
  /** Create an EventWorkspace as diffractometer
   * @param runstart_i64 : start run date from Mantid's Epoch time, in nanoseconds
   * @param pulsedt : time between consecutive pulses, in nanoseconds
   * @param pulseCount : number of pulses
   * @return
   */
  EventWorkspace_sptr createEventWorkspaceElastic(int64_t runstart_i64, int64_t pulsedt, int64_t pulseCount = 1000) {
    // Create an EventWorkspace with 10 banks with 1 detector each.  No events
    // is generated
    EventWorkspace_sptr eventWS = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(10, 1, true);

    Types::Core::DateAndTime runstart(runstart_i64);

    // Create `pulseCount` pulses with one event per pulse
    EventList fakeevlist = fake_uniform_time_sns_data(runstart_i64, pulsedt, pulseCount);
    Types::Core::DateAndTime runend(runstart + pulseCount * pulsedt);

    // Set properties
    eventWS->mutableRun().addProperty("run_start", runstart.toISO8601String(), true);
    eventWS->mutableRun().addProperty("run_end", runend.toISO8601String(), true);

    // Add neutrons
    for (size_t i = 0; i < eventWS->getNumberHistograms(); i++) {
      auto &elist = eventWS->getSpectrum(i);

      for (size_t ievent = 0; ievent < fakeevlist.getNumberEvents(); ++ievent) {
        TofEvent tofevent = fakeevlist.getEvent(ievent);
        elist.addEventQuickly(tofevent);
      } // FOR each pulse
    }   // For each bank

    return eventWS;
  }

  //----------------------------------------------------------------------------------------------
  /** Create a  SplitterWorkspace containing five splitters and three target-workspace indexes
   *
   * List of splitters (all times relative to runstart_i64):
   *            start-time                 end-time                   target-index
   * -------------------------------------------------------------------------------
   *               0                  3*tofdt + tofdt/2                      0
   *       3*tofdt + tofdt/2       pulsedt + 9*tofdt + tofdt/2               1
   *          2*pulsedt          2*pulsedt + 6*tofdt + tofdt/2               2
   *          3*pulsedt          3*pulsedt + 6*tofdt + tofdt/2               2
   *          4*pulsedt          4*pulsedt + 6*tofdt + tofdt/2               2
   *
   * @param runstart_i64 : absolute run start time in int64_t format with unit nanosecond
   * @param pulsedt : time between consecutive pulses in int64_t format with unit nanosecond
   * @param todft : time interval between 2 adjacent event in same pulse in int64_t format of unit nanosecond
   * @param numpulses : number of pulses in the event workspace
   */
  SplittersWorkspace_sptr createSplittersWorkspace(int64_t runstart_i64, int64_t pulsedt, int64_t tofdt) {
    SplittersWorkspace_sptr splitterws = std::make_shared<SplittersWorkspace>();

    // 1. Splitter 0: 0 ~ 3+ (first pulse)
    int64_t t0 = runstart_i64;
    int64_t t1 = t0 + tofdt * 3 + tofdt / 2;
    Kernel::SplittingInterval interval0(t0, t1, 0);
    splitterws->addSplitter(interval0);

    // 2. Splitter 1: 3+ ~ 9+ (second pulse)
    t0 = t1;
    t1 = runstart_i64 + pulsedt + tofdt * 9 + tofdt / 2;
    Kernel::SplittingInterval interval1(t0, t1, 1);
    splitterws->addSplitter(interval1);

    // 3. Splitter 2: from 3rd pulse, 0 ~ 6+
    for (size_t i = 2; i < 5; i++) {
      t0 = runstart_i64 + i * pulsedt;
      TS_ASSERT(t0 >= t1); // validate the previous splitter doesn't overlap with the next one
      t1 = runstart_i64 + i * pulsedt + 6 * tofdt + tofdt / 2;
      Kernel::SplittingInterval interval2(t0, t1, 2);
      splitterws->addSplitter(interval2);
    }

    return splitterws;
  }

  //----------------------------------------------------------------------------------------------
  /** Create a  Splitter for output
   *  Region:
   * 0: pulse 0: 0 ~ 3+
   * 1: pulse 0: 3+ ~ pulse 1: 9+
   * 2: from pulse 2: 0 ~ 6+
   * -1: from pulse 2: 6+ ~ 9+
   * @brief createMatrixSplitter
   * @param runstart_i64 : absolute run start time in int64_t format with unit
   * nanosecond
   * @param pulsedt: pulse length in int64_t format with unit nanosecond
   * @param tofdt: time interval between 2 adjacent event in same pulse in
   * int64_t format of unit nanosecond
   * @return
   */
  API::MatrixWorkspace_sptr createMatrixSplitter(int64_t runstart_i64, int64_t pulsedt, int64_t tofdt) {
    // Create vectors for the splitters
    std::vector<int64_t> time_vec;
    std::vector<int> index_vec;

    time_vec.emplace_back(runstart_i64);

    // Splitter 0: 0 ~ 3+ (first pulse)
    int64_t t1 = runstart_i64 + tofdt * 3 + tofdt / 2;
    time_vec.emplace_back(t1);
    index_vec.emplace_back(0);

    // Splitter 1: 3+ ~ 9+ (second pulse)
    int64_t t2 = runstart_i64 + pulsedt + tofdt * 9 + tofdt / 2;
    time_vec.emplace_back(t2);
    index_vec.emplace_back(1);

    // Splitter 2 and so on: from 3rd pulse, 0 ~ 6+
    for (size_t i = 2; i < 5; i++) {
      int64_t newT = runstart_i64 + i * pulsedt + 6 * tofdt + tofdt / 2;
      time_vec.emplace_back(newT);
      index_vec.emplace_back(2);
    }

    // Create the workspace and set it
    size_t size_x = time_vec.size();
    size_t size_y = index_vec.size();
    TS_ASSERT(size_x - size_y == 1);

    MatrixWorkspace_sptr splitterws = std::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", 1, size_x, size_y));

    for (size_t ix = 0; ix < size_x; ++ix)
      splitterws->mutableX(0)[ix] = static_cast<double>(time_vec[ix]) * 1.E-9;
    for (size_t iy = 0; iy < size_y; ++iy)
      splitterws->mutableY(0)[iy] = static_cast<double>(index_vec[iy]);

    // print out splitters
    for (size_t ix = 0; ix < size_y; ++ix) {
      std::cout << ix << ": " << splitterws->mutableX(0)[ix] << " sec  -  " << splitterws->mutableX(0)[ix + 1]
                << " sec "
                << ": " << splitterws->mutableY(0)[ix] << "\n";
    }

    return splitterws;
  }

  /** Create splitters in TableWorkspace for output which is exactly as the
   * Matrix splitters
   *  Region:
   * 0: pulse 0: 0 ~ 3+
   * 1: pulse 0: 3+ ~ pulse 1: 9+
   * 2: from pulse 2: 0 ~ 6+
   * -1: from pulse 2: 6+ ~ 9+
   * @brief createMatrixSplitter
   * @param runstart_i64 : absolute run start time in int64_t format with unit
   * nanosecond
   * @param pulsedt: pulse length in int64_t format with unit nanosecond
   * @param tofdt: time interval between 2 adjacent event in same pulse in
   * int64_t format of unit nanosecond
   * @return
   */
  DataObjects::TableWorkspace_sptr createTableSplitters(int64_t runstart_i64, int64_t pulsedt, int64_t tofdt) {
    // create table workspace
    DataObjects::TableWorkspace_sptr tablesplitter = std::make_shared<DataObjects::TableWorkspace>();
    tablesplitter->addColumn("double", "start");
    tablesplitter->addColumn("double", "stop");
    tablesplitter->addColumn("str", "target");

    // generate row by row
    // Splitter 0: 0 ~ 3+ (first pulse)
    size_t row_index = 0;
    int64_t t1 = runstart_i64 + tofdt * 3 + tofdt / 2;
    std::string itarget = "A";
    tablesplitter->appendRow();
    tablesplitter->cell<double>(row_index, 0) = static_cast<double>(runstart_i64) * 1.0E-9;
    tablesplitter->cell<double>(row_index, 1) = static_cast<double>(t1) * 1.E-9;
    tablesplitter->cell<std::string>(row_index, 2) = itarget;

    // Splitter 1: 3+ ~ 9+ (second pulse)
    ++row_index;
    int64_t t2 = runstart_i64 + pulsedt + tofdt * 9 + tofdt / 2;
    itarget = "B";
    tablesplitter->appendRow();
    tablesplitter->cell<double>(row_index, 0) = static_cast<double>(t1) * 1.0E-9;
    tablesplitter->cell<double>(row_index, 1) = static_cast<double>(t2) * 1.E-9;
    tablesplitter->cell<std::string>(row_index, 2) = itarget;

    // Splitter 2 and so on: from 3rd pulse, 0 ~ 6+
    int64_t lastT = t2;
    for (size_t i = 2; i < 5; i++) {
      ++row_index;
      itarget = "C";
      int64_t newT = runstart_i64 + i * pulsedt + 6 * tofdt + tofdt / 2;
      tablesplitter->appendRow();
      tablesplitter->cell<double>(row_index, 0) = static_cast<double>(lastT) * 1.0E-9;
      tablesplitter->cell<double>(row_index, 1) = static_cast<double>(newT) * 1.E-9;
      tablesplitter->cell<std::string>(row_index, 2) = itarget;
      lastT = newT;
    }

    return tablesplitter;
  }

  /** Create an InformationWorkspace
   * @param noOfRows: number of rows in the information workspace
   * @param splitByTime: whether the workspace has been split by time or log
   * @return InformationWorkspace
   */
  DataObjects::TableWorkspace_sptr createInformationWorkspace(int noOfRows, bool splitByTime) {
    // create table workspace
    DataObjects::TableWorkspace_sptr infoWorkspace = std::make_shared<DataObjects::TableWorkspace>();
    infoWorkspace->addColumn("int", "workspacegroup");
    infoWorkspace->addColumn("str", "title");

    for (int row_index = 0; row_index < noOfRows; row_index++) {
      infoWorkspace->appendRow();
      std::stringstream rowTitle;
      infoWorkspace->cell<int>(row_index, 0) = row_index;
      // The start and end values in the information workspace title are not
      // needed for test so title contains row_index instead
      if (splitByTime) {
        rowTitle << "Time.Interval.From.start" << row_index << ".To.end" << row_index;
      } else {
        rowTitle << "Log.proton_charge.From.start " << row_index << ".To.end" << row_index
                 << ".Value-change-direction:both";
      }
      infoWorkspace->cell<std::string>(row_index, 1) = rowTitle.str();
    }

    return infoWorkspace;
  }

  //----------------------------------------------------------------------------------------------
  /** Create a Splitter for fast fequency log for output
   * The splitter is within every pulse.  2 groups of splitters are created.
   *In each pulse
   * 1. group 0: 0.2 dT ~ 0.4 dT    dT = pulsedt
   * 2. group 1: 0.6 dT ~ 0.8 dT
   *
   * @param runstart_i64 : absolute run start time in int64_t format with unit
   *nanosecond
   * @param pulsedt : pulse length in int64_t format with unit nanosecond
   * @param todft : time interval between 2 adjacent event in same pulse in
   *int64_t format of unit nanosecond
   * @param numpulses : number of pulses in the event workspace
   */
  SplittersWorkspace_sptr createFastFreqLogSplitter(int64_t runstart_i64, int64_t pulsedt, int64_t tofdt,
                                                    size_t numpulses) {

    UNUSED_ARG(tofdt);

    // 1. Create an empty splitter workspace
    SplittersWorkspace_sptr splitterws = std::make_shared<SplittersWorkspace>();

    // 2. Create splitters
    for (size_t i = 0; i < numpulses; ++i) {
      int64_t t0a =
          runstart_i64 + static_cast<int64_t>(i) * pulsedt + static_cast<int64_t>(static_cast<double>(pulsedt) * 0.2);
      int64_t tfa =
          runstart_i64 + static_cast<int64_t>(i) * pulsedt + static_cast<int64_t>(static_cast<double>(pulsedt) * 0.4);
      Kernel::SplittingInterval interval_a(t0a, tfa, 0);

      int64_t t0b =
          runstart_i64 + static_cast<int64_t>(i) * pulsedt + static_cast<int64_t>(static_cast<double>(pulsedt) * 0.6);
      int64_t tfb =
          runstart_i64 + static_cast<int64_t>(i) * pulsedt + static_cast<int64_t>(static_cast<double>(pulsedt) * 0.8);
      Kernel::SplittingInterval interval_b(t0b, tfb, 1);

      splitterws->addSplitter(interval_a);
      splitterws->addSplitter(interval_b);
    }

    return splitterws;
  }

  //----------------------------------------------------------------------------------------------
  /** Create the time correction table
   */
  TableWorkspace_sptr createTimeCorrectionTable(const MatrixWorkspace_sptr &inpws) {
    // 1. Generate an empty table
    auto corrtable = std::make_shared<TableWorkspace>();
    corrtable->addColumn("int", "DetectorID");
    corrtable->addColumn("double", "Correction");

    // 2. Add rows
    const auto &detectorInfo = inpws->detectorInfo();
    const auto detids = detectorInfo.detectorIDs();
    for (int detid : detids) {
      double factor = 0.75;
      TableRow newrow = corrtable->appendRow();
      newrow << detid << factor;
    }

    return corrtable;
  }

  //----------------------------------------------------------------------------------------------
  /** Fake uniform time data more close to SNS case
   * @param runstart : run start time in nanoseconds
   * @param pulsedt : time between consecutive pulses, in nanoseconds
   * @param pulseCount : number of pulses
   */
  EventList fake_uniform_time_sns_data(int64_t runstart, int64_t pulselength, int64_t pulseCount = 1000) {
    // Clear the list
    EventList el = EventList();

    // Create some mostly-reasonable fake data.
    unsigned seed1 = 1;
    std::minstd_rand0 g1(seed1);

    for (int time = 0; time < pulseCount; time++) {
      // All pulse times from 0 to 999 in seconds
      Types::Core::DateAndTime pulsetime(static_cast<int64_t>(time * pulselength + runstart));
      double tof = static_cast<double>(g1() % 1000);
      el += TofEvent(tof, pulsetime);
    }

    return el;
  }

  /** Create a matrix splitters workspace for elastic correction
   * @param runstart_i64 : start run date from Mantid's Epoch time, in nanoseconds
   */
  API::MatrixWorkspace_sptr createMatrixSplittersElastic(int64_t runstart) {
    MatrixWorkspace_sptr spws =
        std::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceFactory::Instance().create("Workspace2D", 1, 11, 10));

    auto &vec_splitTimes = spws->mutableX(0);
    auto &vec_splitGroup = spws->mutableY(0);

    // set up the splitters in nanosecond
    vec_splitTimes[0] = 1000000;
    vec_splitTimes[1] = 1300000;
    vec_splitTimes[2] = 2000000;
    vec_splitTimes[3] = 2190000;
    vec_splitTimes[4] = 4000000;
    vec_splitTimes[5] = 5000000;
    vec_splitTimes[6] = 5500000;
    vec_splitTimes[7] = 7000000;
    vec_splitTimes[8] = 8000000;
    vec_splitTimes[9] = 9000000;
    vec_splitTimes[10] = 10000000;

    // add runstart and convert the splitters' time to second
    constexpr double nanosecondToSecond{1.E-9};
    for (size_t i = 0; i < vec_splitTimes.size(); ++i) {
      vec_splitTimes[i] += static_cast<double>(runstart);
      vec_splitTimes[i] *= nanosecondToSecond;
    }

    vec_splitGroup[0] = 2;
    vec_splitGroup[1] = 5;
    vec_splitGroup[2] = 4;
    vec_splitGroup[3] = -1;
    vec_splitGroup[4] = 6;
    vec_splitGroup[5] = 7;
    vec_splitGroup[6] = 8;
    vec_splitGroup[7] = -1;
    vec_splitGroup[8] = 1;
    vec_splitGroup[9] = 3;

    return spws;
  }

  /** Create a matrix splitters workspace for direct geometry correction
   * @param runstart_i64 : start run date from Mantid's Epoch time, in nanoseconds
   */
  API::MatrixWorkspace_sptr createMatrixSplittersDG(int64_t runstart = 0) {
    MatrixWorkspace_sptr spws =
        std::dynamic_pointer_cast<MatrixWorkspace>(WorkspaceFactory::Instance().create("Workspace2D", 1, 11, 10));

    auto &vec_splitTimes = spws->mutableX(0);
    auto &vec_splitGroup = spws->mutableY(0);

    // create the splitters in nanosecond
    vec_splitTimes[0] = 1000000;
    vec_splitTimes[1] = 1300000; // Rule in  1,339,000
    vec_splitTimes[2] = 2000000;
    vec_splitTimes[3] = 2190000; // Rule out 2,155,000
    vec_splitTimes[4] = 4000000;
    vec_splitTimes[5] = 5000000;
    vec_splitTimes[6] = 5500000; // Rule in  5,741,000
    vec_splitTimes[7] = 7000000;
    vec_splitTimes[8] = 8000000;
    vec_splitTimes[9] = 9000000;
    vec_splitTimes[10] = 10000000;

    // add runstart and convert the splitters' time to second
    constexpr double nanosecondToSecond{1.E-9};
    for (size_t i = 0; i < vec_splitTimes.size(); ++i) {
      vec_splitTimes[i] += static_cast<double>(runstart);
      vec_splitTimes[i] *= nanosecondToSecond;
    }

    vec_splitGroup[0] = 2;
    vec_splitGroup[1] = 5;
    vec_splitGroup[2] = 4;
    vec_splitGroup[3] = -1;
    vec_splitGroup[4] = 6;
    vec_splitGroup[5] = 7;
    vec_splitGroup[6] = 8;
    vec_splitGroup[7] = -1;
    vec_splitGroup[8] = 1;
    vec_splitGroup[9] = 3;

    return spws;
  }
};
