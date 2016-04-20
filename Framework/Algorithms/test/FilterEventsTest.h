#ifndef MANTID_ALGORITHMS_FILTEREVENTSTEST_H_
#define MANTID_ALGORITHMS_FILTEREVENTSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"

#include "MantidAlgorithms/FilterEvents.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Events.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/TimeSplitter.h"

#include <random>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

using namespace std;

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
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100 * 1000 * 1000;
    int64_t tofdt = 10 * 1000 * 1000;
    size_t numpulses = 5;

    DataObjects::EventWorkspace_sptr eventws =
        createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);

    TS_ASSERT_EQUALS(eventws->getNumberEvents(), 500);

    DataObjects::EventList elist = eventws->getEventList(0);
    TS_ASSERT_EQUALS(elist.getNumberEvents(), 50);
    TS_ASSERT(elist.hasDetectorID(1));

    DataObjects::SplittersWorkspace_sptr splittersws =
        createSplitter(runstart_i64, pulsedt, tofdt);
    TS_ASSERT_EQUALS(splittersws->getNumberSplitters(), 5);

    return;
  }

  //----------------------------------------------------------------------------------------------
  /**  Filter events without any correction
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
  void test_FilterNoCorrection() {
    // Create EventWorkspace and SplittersWorkspace
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100 * 1000 * 1000;
    int64_t tofdt = 10 * 1000 * 1000;
    size_t numpulses = 5;

    DataObjects::EventWorkspace_sptr inpWS =
        createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("Test02", inpWS);

    DataObjects::SplittersWorkspace_sptr splws =
        createSplitter(runstart_i64, pulsedt, tofdt);
    AnalysisDataService::Instance().addOrReplace("Splitter02", splws);

    FilterEvents filter;
    filter.initialize();

    // Set properties
    filter.setProperty("InputWorkspace", "Test02");
    filter.setProperty("OutputWorkspaceBaseName", "FilteredWS01");
    filter.setProperty("SplitterWorkspace", "Splitter02");
    filter.setProperty("OutputTOFCorrectionWorkspace", "CorrectionWS");

    // Execute
    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    // Get output
    int numsplittedws = filter.getProperty("NumberOutputWS");
    TS_ASSERT_EQUALS(numsplittedws, 4);

    // Check Workspace group 0
    DataObjects::EventWorkspace_sptr filteredws0 =
        boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
            AnalysisDataService::Instance().retrieve("FilteredWS01_0"));
    TS_ASSERT(filteredws0);
    TS_ASSERT_EQUALS(filteredws0->getNumberHistograms(), 10);
    TS_ASSERT_EQUALS(filteredws0->getEventList(0).getNumberEvents(), 4);
    TS_ASSERT_EQUALS(filteredws0->run().getProtonCharge(), 10);

    // Check Workspace group 1
    DataObjects::EventWorkspace_sptr filteredws1 =
        boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
            AnalysisDataService::Instance().retrieve("FilteredWS01_1"));
    TS_ASSERT(filteredws1);
    TS_ASSERT_EQUALS(filteredws1->getEventList(1).getNumberEvents(), 16);
    TS_ASSERT_EQUALS(filteredws1->run().getProtonCharge(), 11);

    // Check Workspace group 2
    DataObjects::EventWorkspace_sptr filteredws2 =
        boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
            AnalysisDataService::Instance().retrieve("FilteredWS01_2"));
    TS_ASSERT(filteredws2);
    TS_ASSERT_EQUALS(filteredws2->getEventList(1).getNumberEvents(), 21);
    TS_ASSERT_EQUALS(filteredws2->run().getProtonCharge(), 21);

    DataObjects::EventList elist3 = filteredws2->getEventList(3);
    elist3.sortPulseTimeTOF();

    DataObjects::TofEvent eventmin = elist3.getEvent(0);
    TS_ASSERT_EQUALS(eventmin.pulseTime().totalNanoseconds(),
                     runstart_i64 + pulsedt * 2);
    TS_ASSERT_DELTA(eventmin.tof(), 0, 1.0E-4);

    DataObjects::TofEvent eventmax = elist3.getEvent(20);
    TS_ASSERT_EQUALS(eventmax.pulseTime().totalNanoseconds(),
                     runstart_i64 + pulsedt * 4);
    TS_ASSERT_DELTA(eventmax.tof(), static_cast<double>(tofdt * 6 / 1000),
                    1.0E-4);

    // Clean up
    AnalysisDataService::Instance().remove("Test02");
    AnalysisDataService::Instance().remove("Splitter02");
    std::vector<std::string> outputwsnames =
        filter.getProperty("OutputWorkspaceNames");
    for (size_t i = 0; i < outputwsnames.size(); ++i) {
      AnalysisDataService::Instance().remove(outputwsnames[i]);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /**  Filter events without any correction and test for user-specified
   *workspace starting value
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
    // Create EventWorkspace and SplittersWorkspace
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100 * 1000 * 1000;
    int64_t tofdt = 10 * 1000 * 1000;
    size_t numpulses = 5;

    DataObjects::EventWorkspace_sptr inpWS =
        createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("Test02", inpWS);

    DataObjects::SplittersWorkspace_sptr splws =
        createSplitter(runstart_i64, pulsedt, tofdt);
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
    int numsplittedws = filter.getProperty("NumberOutputWS");
    TS_ASSERT_EQUALS(numsplittedws, 3);

    // 4.1 Workspace group 0
    DataObjects::EventWorkspace_sptr filteredws0 =
        boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
            AnalysisDataService::Instance().retrieve("FilteredWS01_1"));
    TS_ASSERT(filteredws0);
    TS_ASSERT_EQUALS(filteredws0->getNumberHistograms(), 10);
    TS_ASSERT_EQUALS(filteredws0->getEventList(0).getNumberEvents(), 4);

    // 4.2 Workspace group 1
    DataObjects::EventWorkspace_sptr filteredws1 =
        boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
            AnalysisDataService::Instance().retrieve("FilteredWS01_2"));
    TS_ASSERT(filteredws1);
    TS_ASSERT_EQUALS(filteredws1->getEventList(1).getNumberEvents(), 16);

    // 4.3 Workspace group 2
    DataObjects::EventWorkspace_sptr filteredws2 =
        boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
            AnalysisDataService::Instance().retrieve("FilteredWS01_3"));
    TS_ASSERT(filteredws2);
    TS_ASSERT_EQUALS(filteredws2->getEventList(1).getNumberEvents(), 21);

    DataObjects::EventList elist3 = filteredws2->getEventList(3);
    elist3.sortPulseTimeTOF();

    DataObjects::TofEvent eventmin = elist3.getEvent(0);
    TS_ASSERT_EQUALS(eventmin.pulseTime().totalNanoseconds(),
                     runstart_i64 + pulsedt * 2);
    TS_ASSERT_DELTA(eventmin.tof(), 0, 1.0E-4);

    DataObjects::TofEvent eventmax = elist3.getEvent(20);
    TS_ASSERT_EQUALS(eventmax.pulseTime().totalNanoseconds(),
                     runstart_i64 + pulsedt * 4);
    TS_ASSERT_DELTA(eventmax.tof(), static_cast<double>(tofdt * 6 / 1000),
                    1.0E-4);

    // 5. Clean up
    AnalysisDataService::Instance().remove("Test02");
    AnalysisDataService::Instance().remove("Splitter02");
    std::vector<std::string> outputwsnames =
        filter.getProperty("OutputWorkspaceNames");
    for (size_t i = 0; i < outputwsnames.size(); ++i) {
      AnalysisDataService::Instance().remove(outputwsnames[i]);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /**  Filter test with TOF correction
    */
  void test_FilterWithCustumizedCorrection() {
    // 1. Create EventWorkspace and SplittersWorkspace
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100 * 1000 * 1000;
    int64_t tofdt = 10 * 1000 * 1000;
    size_t numpulses = 5;

    DataObjects::EventWorkspace_sptr inpWS =
        createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("EventData", inpWS);

    DataObjects::SplittersWorkspace_sptr splws =
        createFastFreqLogSplitter(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("SplitterTableX", splws);
    TS_ASSERT_EQUALS(splws->rowCount(), static_cast<size_t>(numpulses) * 2);

    TableWorkspace_sptr timecorrws = createTimeCorrectionTable(inpWS);
    AnalysisDataService::Instance().addOrReplace("TimeCorrectionTableX",
                                                 timecorrws);
    TS_ASSERT_EQUALS(timecorrws->rowCount(), inpWS->getNumberHistograms());

    FilterEvents filter;
    filter.initialize();

    // 2. Set properties
    TS_ASSERT_THROWS_NOTHING(filter.setProperty("InputWorkspace", "EventData"));
    TS_ASSERT_THROWS_NOTHING(
        filter.setProperty("OutputWorkspaceBaseName", "SplittedDataX"));
    TS_ASSERT_THROWS_NOTHING(
        filter.setProperty("CorrectionToSample", "Customized"));
    TS_ASSERT_THROWS_NOTHING(filter.setProperty(
        "DetectorTOFCorrectionWorkspace", "TimeCorrectionTableX"));
    TS_ASSERT_THROWS_NOTHING(filter.setProperty("SplitterWorkspace", splws));

    // 3. Execute
    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    // 4. Get output
    // 4.1 Workspace group 0
    DataObjects::EventWorkspace_sptr filteredws0 =
        boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
            AnalysisDataService::Instance().retrieve("SplittedDataX_0"));
    TS_ASSERT(filteredws0);
    TS_ASSERT_EQUALS(filteredws0->getNumberHistograms(), 10);
    TS_ASSERT_EQUALS(filteredws0->getEventList(0).getNumberEvents(), 15);
    TS_ASSERT_EQUALS(filteredws0->getEventList(9).getNumberEvents(), 15);
    TS_ASSERT_EQUALS(filteredws0->run().getProtonCharge(), 5);

    // 4.2 Workspace group 1
    DataObjects::EventWorkspace_sptr filteredws1 =
        boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
            AnalysisDataService::Instance().retrieve("SplittedDataX_1"));
    TS_ASSERT(filteredws1);
    TS_ASSERT_EQUALS(filteredws1->getEventList(1).getNumberEvents(), 10);
    TS_ASSERT_EQUALS(filteredws0->run().getProtonCharge(), 5);

    // 4.3 Some individual events
    DataObjects::EventList elist3 = filteredws1->getEventList(3);
    elist3.sortPulseTimeTOF();

    if (elist3.getNumberEvents() > 0) {
      DataObjects::TofEvent eventmin = elist3.getEvent(0);
      TS_ASSERT_EQUALS(eventmin.pulseTime().totalNanoseconds(), runstart_i64);
      TS_ASSERT_DELTA(eventmin.tof(), 80 * 1000, 1.0E-4);
    }

    // 5. Clean
    AnalysisDataService::Instance().remove("EventData");
    AnalysisDataService::Instance().remove("TimeCorrectionTableX");
    AnalysisDataService::Instance().remove("SplitterTableX");

    std::vector<std::string> outputwsnames =
        filter.getProperty("OutputWorkspaceNames");
    for (size_t i = 0; i < outputwsnames.size(); ++i) {
      AnalysisDataService::Instance().remove(outputwsnames[i]);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test filtering with correction of direct geometry
    */
  void test_FilterElasticCorrection() {
    DataObjects::EventWorkspace_sptr ws =
        createEventWorkspaceElastic(0, 1000000);
    AnalysisDataService::Instance().addOrReplace("MockElasticEventWS", ws);
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 10000);

    MatrixWorkspace_sptr splws = createMatrixSplittersElastic();
    AnalysisDataService::Instance().addOrReplace("SplitterTableX", splws);

    // Run the filtering
    FilterEvents filter;
    filter.initialize();

    filter.setPropertyValue("InputWorkspace", "MockElasticEventWS");
    filter.setProperty("OutputWorkspaceBaseName", "SplittedDataElastic");
    filter.setProperty("CorrectionToSample", "Elastic");
    filter.setProperty("SplitterWorkspace", "SplitterTableX");

    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    // Check number of output workspaces
    std::vector<std::string> vecwsname =
        filter.getProperty("OutputWorkspaceNames");
    TS_ASSERT_EQUALS(vecwsname.size(), 9);

    EventWorkspace_sptr ws5 = boost::dynamic_pointer_cast<EventWorkspace>(
        AnalysisDataService::Instance().retrieve("SplittedDataElastic_5"));
    TS_ASSERT(ws5);
    if (ws5) {
      TS_ASSERT_EQUALS(ws5->getNumberEvents(), 0);
    }

    EventWorkspace_sptr ws7 = boost::dynamic_pointer_cast<EventWorkspace>(
        AnalysisDataService::Instance().retrieve("SplittedDataElastic_7"));
    TS_ASSERT(ws7);
    if (ws7) {
      TS_ASSERT_EQUALS(ws7->getNumberEvents(), 10);
    }

    // Check individual events
    EventList &ev0 = ws7->getEventList(0);
    TS_ASSERT_EQUALS(ev0.getNumberEvents(), 1);
    std::vector<double> vectofs = ev0.getTofs();
    TS_ASSERT_DELTA(vectofs[0], 272.0, 0.001);

    // Delete all the workspaces generated here
    AnalysisDataService::Instance().remove("MockDirectEventWS");
    AnalysisDataService::Instance().remove("SplitterTableX");
    for (size_t i = 0; i < vecwsname.size(); ++i) {
      AnalysisDataService::Instance().remove(vecwsname[i]);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test filtering with correction of direct geometry
    */
  void test_FilterDGCorrection() {
    DataObjects::EventWorkspace_sptr ws =
        createEventWorkspaceDirect(0, 1000000);
    AnalysisDataService::Instance().addOrReplace("MockDirectEventWS", ws);

    MatrixWorkspace_sptr splws = createMatrixSplittersDG();
    AnalysisDataService::Instance().addOrReplace("SplitterTableX", splws);

    // Run the filtering
    FilterEvents filter;
    filter.initialize();

    filter.setProperty("InputWorkspace", ws->name());
    filter.setProperty("OutputWorkspaceBaseName", "SplittedDataDG");
    filter.setProperty("CorrectionToSample", "Direct");
    filter.setProperty("SplitterWorkspace", "SplitterTableX");

    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    // Check
    EventWorkspace_sptr ws5 = boost::dynamic_pointer_cast<EventWorkspace>(
        AnalysisDataService::Instance().retrieve("SplittedDataDG_5"));
    TS_ASSERT(ws5);
    if (ws5) {
      TS_ASSERT_EQUALS(ws5->getNumberEvents(), 0);
    }

    EventWorkspace_sptr ws7 = boost::dynamic_pointer_cast<EventWorkspace>(
        AnalysisDataService::Instance().retrieve("SplittedDataDG_7"));
    TS_ASSERT(ws7);
    if (ws7) {
      TS_ASSERT_EQUALS(ws7->getNumberEvents(), ws7->getNumberHistograms());
    }

    // FIXME - Should find a way to delete all workspaces holding splitted
    // events

    AnalysisDataService::Instance().remove("MockDirectEventWS");
    AnalysisDataService::Instance().remove("SplitterTableX");
    std::vector<std::string> outputwsnames =
        filter.getProperty("OutputWorkspaceNames");
    for (size_t i = 0; i < outputwsnames.size(); ++i) {
      AnalysisDataService::Instance().remove(outputwsnames[i]);
    }

    return;
  }

  //----------------------------------------------------------------------------------------------
  /** Test filtering with correction to indirect geometry inelastic instrument
    */
  void test_FilterIndirectGeometryCorrection() {
    // Create workspaces for filtering
    DataObjects::EventWorkspace_sptr ws =
        createEventWorkspaceInDirect(0, 1000000);
    AnalysisDataService::Instance().addOrReplace("MockIndirectEventWS", ws);

    MatrixWorkspace_sptr splws = createMatrixSplittersDG();
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
        boost::dynamic_pointer_cast<MatrixWorkspace>(
            AnalysisDataService::Instance().retrieve("MockIndGeoCorrWS"));
    TS_ASSERT(outcorrws);
    if (outcorrws) {
      TS_ASSERT_EQUALS(outcorrws->getNumberHistograms(),
                       ws->getNumberHistograms());
      TS_ASSERT_EQUALS(outcorrws->readX(0).size(), 2);

      Kernel::V3D samplepos = ws->getInstrument()->getSample()->getPos();

      for (size_t iws = 0; iws < outcorrws->getNumberHistograms(); ++iws) {
        const ParameterMap &pmap = ws->constInstrumentParameters();

        IDetector_const_sptr det = ws->getDetector(iws);
        Kernel::V3D detpos = det->getPos();
        Parameter_sptr par = pmap.getRecursive(det.get(), "Efixed");
        double efix = par->value<double>();

        double l2 = samplepos.distance(detpos);

        double shift = -l2 / sqrt(efix * 2. * PhysicalConstants::meV /
                                  PhysicalConstants::NeutronMass);

        TS_ASSERT_DELTA(outcorrws->readY(iws)[0], 1., 1.0E-9);
        TS_ASSERT_DELTA(outcorrws->readY(iws)[1], shift, 1.0E-9);
      }
    }

    // Clean
    AnalysisDataService::Instance().remove("MockIndirectEventWS");
    std::vector<std::string> outputwsnames =
        filter.getProperty("OutputWorkspaceNames");
    for (size_t i = 0; i < outputwsnames.size(); ++i) {
      AnalysisDataService::Instance().remove(outputwsnames[i]);
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
   *events
   */
  void test_FilterRelativeTime() {
    // Create EventWorkspace and SplittersWorkspace
    int64_t runstart_i64 = 20000000000;
    int64_t pulsedt = 100 * 1000 * 1000;
    int64_t tofdt = 10 * 1000 * 1000;
    size_t numpulses = 5;

    DataObjects::EventWorkspace_sptr inpWS =
        createEventWorkspace(runstart_i64, pulsedt, tofdt, numpulses);
    AnalysisDataService::Instance().addOrReplace("Test10", inpWS);

    API::MatrixWorkspace_sptr splws = createMatrixSplitter(0, pulsedt, tofdt);
    AnalysisDataService::Instance().addOrReplace("Splitter10", splws);

    FilterEvents filter;
    filter.initialize();

    // Set properties
    filter.setProperty("InputWorkspace", "Test10");
    filter.setProperty("OutputWorkspaceBaseName", "FilteredWS10");
    filter.setProperty("SplitterWorkspace", "Splitter10");
    filter.setProperty("RelativeTime", true);
    filter.setProperty("OutputWorkspaceIndexedFrom1", true);

    // Execute
    TS_ASSERT_THROWS_NOTHING(filter.execute());
    TS_ASSERT(filter.isExecuted());

    // Get 3 output workspaces
    int numsplittedws = filter.getProperty("NumberOutputWS");
    TS_ASSERT_EQUALS(numsplittedws, 3);

    // Workspace 0
    DataObjects::EventWorkspace_sptr filteredws0 =
        boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
            AnalysisDataService::Instance().retrieve("FilteredWS10_1"));
    TS_ASSERT(filteredws0);
    TS_ASSERT_EQUALS(filteredws0->getNumberHistograms(), 10);
    TS_ASSERT_EQUALS(filteredws0->getEventList(0).getNumberEvents(), 3);

    // Workspace 1
    DataObjects::EventWorkspace_sptr filteredws1 =
        boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
            AnalysisDataService::Instance().retrieve("FilteredWS10_2"));
    TS_ASSERT(filteredws1);
    TS_ASSERT_EQUALS(filteredws1->getEventList(1).getNumberEvents(), 16);

    // Workspace 2
    DataObjects::EventWorkspace_sptr filteredws2 =
        boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
            AnalysisDataService::Instance().retrieve("FilteredWS10_3"));
    TS_ASSERT(filteredws2);
    TS_ASSERT_EQUALS(filteredws2->getEventList(1).getNumberEvents(), 27);

    // Check spectrum 3 of workspace 2
    DataObjects::EventList elist3 = filteredws2->getEventList(3);
    elist3.sortPulseTimeTOF();

    DataObjects::TofEvent eventmin = elist3.getEvent(0);
    TS_ASSERT_EQUALS(eventmin.pulseTime().totalNanoseconds(),
                     runstart_i64 + pulsedt * 2);
    TS_ASSERT_DELTA(eventmin.tof(), 0, 1.0E-4);

    DataObjects::TofEvent eventmax = elist3.getEvent(26);
    TS_ASSERT_EQUALS(eventmax.pulseTime().totalNanoseconds(),
                     runstart_i64 + pulsedt * 4);
    TS_ASSERT_DELTA(eventmax.tof(), static_cast<double>(tofdt * 6 / 1000),
                    1.0E-4);

    // 5. Clean up
    AnalysisDataService::Instance().remove("Test02");
    AnalysisDataService::Instance().remove("Splitter02");
    std::vector<std::string> outputwsnames =
        filter.getProperty("OutputWorkspaceNames");
    for (size_t i = 0; i < outputwsnames.size(); ++i) {
      AnalysisDataService::Instance().remove(outputwsnames[i]);
    }

    return;
  }
  //----------------------------------------------------------------------------------------------
  /** Create an EventWorkspace.  This workspace has
    * @param runstart_i64 : absolute run start time in int64_t format with unit
   * nanosecond
    * @param pulsedt : pulse length in int64_t format with unit nanosecond
    * @param todft : time interval between 2 adjacent event in same pulse in
   * int64_t format of unit nanosecond
    * @param numpulses : number of pulses in the event workspace
   */
  DataObjects::EventWorkspace_sptr createEventWorkspace(int64_t runstart_i64,
                                                        int64_t pulsedt,
                                                        int64_t tofdt,
                                                        size_t numpulses) {
    // 1. Create an EventWorkspace with 10 detectors
    DataObjects::EventWorkspace_sptr eventWS =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(10, 1,
                                                                        true);

    Kernel::DateAndTime runstart(runstart_i64);

    // 2. Set run_start time
    eventWS->mutableRun().addProperty("run_start", runstart.toISO8601String(),
                                      true);

    // create a pcharge log
    auto pchargeLog = Kernel::make_unique<Kernel::TimeSeriesProperty<double>>(
        "proton_charge");

    for (size_t i = 0; i < eventWS->getNumberHistograms(); i++) {
      DataObjects::EventList *elist = eventWS->getEventListPtr(i);

      for (int64_t pid = 0; pid < static_cast<int64_t>(numpulses); pid++) {
        int64_t pulsetime_i64 = pid * pulsedt + runstart.totalNanoseconds();
        Kernel::DateAndTime pulsetime(pulsetime_i64);
        pchargeLog->addValue(pulsetime, 1.);
        for (size_t e = 0; e < 10; e++) {
          double tof = static_cast<double>(e * tofdt / 1000);
          DataObjects::TofEvent event(tof, pulsetime);
          elist->addEventQuickly(event);
        }
      } // FOR each pulse
    }   // For each bank

    eventWS->mutableRun().addLogData(pchargeLog.release());
    eventWS->mutableRun().integrateProtonCharge();

    return eventWS;
  }

  //----------------------------------------------------------------------------------------------
  /** Create an EventWorkspace to mimic direct inelastic scattering insturment.
    * This workspace will have the same neutron events as the test case in
    *EventList
    *
    * @param runstart_i64 : absolute run start time in int64_t format with unit
    *nanosecond
    * @param pulsedt : pulse length in int64_t format with unit nanosecond
   */
  DataObjects::EventWorkspace_sptr
  createEventWorkspaceDirect(int64_t runstart_i64, int64_t pulsedt) {
    // Create an EventWorkspace with 10 banks with 1 detector each.  No events
    // is generated
    DataObjects::EventWorkspace_sptr eventWS =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(10, 1,
                                                                        true);

    // L1 = 10
    Kernel::V3D samplepos = eventWS->getInstrument()->getSample()->getPos();
    Kernel::V3D sourcepos = eventWS->getInstrument()->getSource()->getPos();
    double l1 = samplepos.distance(sourcepos);

    Kernel::DateAndTime runstart(runstart_i64);

    EventList fakeevlist = fake_uniform_time_sns_data(runstart_i64, pulsedt);

    // Set properties: (1) run_start time; (2) Ei
    eventWS->mutableRun().addProperty("run_start", runstart.toISO8601String(),
                                      true);

    double shift = 2.E-4;
    double ei = (l1 * l1 * PhysicalConstants::NeutronMass) /
                (shift * shift * 2. * PhysicalConstants::meV);

    eventWS->mutableRun().addProperty<double>("Ei", ei, true);

    // Add neutrons
    for (size_t i = 0; i < eventWS->getNumberHistograms(); i++) {
      DataObjects::EventList *elist = eventWS->getEventListPtr(i);

      for (size_t ievent = 0; ievent < fakeevlist.getNumberEvents(); ++ievent) {
        TofEvent tofevent = fakeevlist.getEvent(ievent);
        elist->addEventQuickly(tofevent);
      } // FOR each pulse
    }   // For each bank

    // double constshift = l1 / sqrt(ei * 2. * PhysicalConstants::meV /
    //                           PhysicalConstants::NeutronMass);

    return eventWS;
  }

  //----------------------------------------------------------------------------------------------
  /** Create an EventWorkspace to mimic direct inelastic scattering insturment.
   * This workspace has
    * @param runstart_i64 : absolute run start time in int64_t format with unit
   * nanosecond
    * @param pulsedt : pulse length in int64_t format with unit nanosecond
    */
  DataObjects::EventWorkspace_sptr
  createEventWorkspaceInDirect(int64_t runstart_i64, int64_t pulsedt) {
    // Create an EventWorkspace with 10 banks with 1 detector each.  No events
    // is generated
    DataObjects::EventWorkspace_sptr eventWS =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(10, 1,
                                                                        true);

    // Add EFixed to each detector
    const ParameterMap &pmap = eventWS->constInstrumentParameters();

    for (size_t i = 0; i < 10; ++i) {
      Geometry::IDetector_const_sptr det = eventWS->getDetector(i);
      Parameter_sptr par = pmap.getRecursive(det.get(), "Efixed");
      if (par) {
        // No need to set up E-Fix
        // double efix = par->value<double>();
        ;
      } else {

        eventWS->setEFixed(det->getID(), 2.08);
      }
    }

    // Add neutrons
    EventList fakeevlist = fake_uniform_time_sns_data(runstart_i64, pulsedt);
    for (size_t i = 0; i < eventWS->getNumberHistograms(); i++) {
      DataObjects::EventList *elist = eventWS->getEventListPtr(i);

      for (size_t ievent = 0; ievent < fakeevlist.getNumberEvents(); ++ievent) {
        TofEvent tofevent = fakeevlist.getEvent(ievent);
        elist->addEventQuickly(tofevent);
      } // FOR each pulse
    }   // For each bank

    return eventWS;
  }

  //----------------------------------------------------------------------------------------------
  /** Create an EventWorkspace as diffractometer
   * @brief createEventWorkspaceElastic
   * @param runstart_i64
   * @param pulsedt
   * @return
   */
  DataObjects::EventWorkspace_sptr
  createEventWorkspaceElastic(int64_t runstart_i64, int64_t pulsedt) {
    // Create an EventWorkspace with 10 banks with 1 detector each.  No events
    // is generated
    DataObjects::EventWorkspace_sptr eventWS =
        WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(10, 1,
                                                                        true);

    // L1 = 10
    /*
    Kernel::V3D samplepos = eventWS->getInstrument()->getSample()->getPos();
    Kernel::V3D sourcepos = eventWS->getInstrument()->getSource()->getPos();
    std::cout << "sample position: " << samplepos.toString() << "\n";
    std::cout << "source position: " << sourcepos.toString() << "\n";
    double l1 = samplepos.distance(sourcepos);
    std::cout << "L1 = " << l1 << "\n";
    for (size_t i = 0; i < eventWS->getNumberHistograms(); ++i) {
      Kernel::V3D detpos = eventWS->getDetector(i)->getPos();
      double l2 = samplepos.distance(detpos);
      std::cout << "detector " << i << ": L2 = " << l2 << "\n";
    }
    */

    Kernel::DateAndTime runstart(runstart_i64);

    // Create 1000 events
    EventList fakeevlist = fake_uniform_time_sns_data(runstart_i64, pulsedt);

    // Set properties: (1) run_start time; (2) Ei
    eventWS->mutableRun().addProperty("run_start", runstart.toISO8601String(),
                                      true);

    // Add neutrons
    for (size_t i = 0; i < eventWS->getNumberHistograms(); i++) {
      DataObjects::EventList *elist = eventWS->getEventListPtr(i);

      for (size_t ievent = 0; ievent < fakeevlist.getNumberEvents(); ++ievent) {
        TofEvent tofevent = fakeevlist.getEvent(ievent);
        elist->addEventQuickly(tofevent);
      } // FOR each pulse
    }   // For each bank

    return eventWS;
  }

  //----------------------------------------------------------------------------------------------
  /** Create a  Splitter for output
   *  Region:
   * 0: pulse 0: 0 ~ 3+
   * 1: pulse 0: 3+ ~ pulse 1: 9+
   * 2: from pulse 2: 0 ~ 6+
   * -1: from pulse 2: 6+ ~ 9+
    * @param runstart_i64 : absolute run start time in int64_t format with unit
   * nanosecond
    * @param pulsedt : pulse length in int64_t format with unit nanosecond
    * @param todft : time interval between 2 adjacent event in same pulse in
   * int64_t format of unit nanosecond
    * @param numpulses : number of pulses in the event workspace
   */
  DataObjects::SplittersWorkspace_sptr
  createSplitter(int64_t runstart_i64, int64_t pulsedt, int64_t tofdt) {
    DataObjects::SplittersWorkspace_sptr splitterws =
        boost::shared_ptr<DataObjects::SplittersWorkspace>(
            new DataObjects::SplittersWorkspace);

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
  API::MatrixWorkspace_sptr
  createMatrixSplitter(int64_t runstart_i64, int64_t pulsedt, int64_t tofdt) {
    // Create vectors for the splitters
    std::vector<int64_t> time_vec;
    std::vector<int> index_vec;

    time_vec.push_back(runstart_i64);

    // Splitter 0: 0 ~ 3+ (first pulse)
    int64_t t1 = runstart_i64 + tofdt * 3 + tofdt / 2;
    time_vec.push_back(t1);
    index_vec.push_back(0);

    // Splitter 1: 3+ ~ 9+ (second pulse)
    int64_t t2 = runstart_i64 + pulsedt + tofdt * 9 + tofdt / 2;
    time_vec.push_back(t2);
    index_vec.push_back(1);

    // Splitter 2 and so on: from 3rd pulse, 0 ~ 6+
    for (size_t i = 2; i < 5; i++) {
      int64_t newT = runstart_i64 + i * pulsedt + 6 * tofdt + tofdt / 2;
      time_vec.push_back(newT);
      index_vec.push_back(2);
    }

    // Create the workspace and set it
    size_t size_x = time_vec.size();
    size_t size_y = index_vec.size();
    TS_ASSERT(size_x - size_y == 1);

    MatrixWorkspace_sptr splitterws =
        boost::dynamic_pointer_cast<MatrixWorkspace>(
            WorkspaceFactory::Instance().create("Workspace2D", 1, size_x,
                                                size_y));

    for (size_t ix = 0; ix < size_x; ++ix)
      splitterws->dataX(0)[ix] = static_cast<double>(time_vec[ix]);
    for (size_t iy = 0; iy < size_y; ++iy)
      splitterws->dataY(0)[iy] = static_cast<double>(index_vec[iy]);

    return splitterws;
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
  SplittersWorkspace_sptr createFastFreqLogSplitter(int64_t runstart_i64,
                                                    int64_t pulsedt,
                                                    int64_t tofdt,
                                                    size_t numpulses) {

    UNUSED_ARG(tofdt);

    // 1. Create an empty splitter workspace
    SplittersWorkspace_sptr splitterws =
        boost::shared_ptr<DataObjects::SplittersWorkspace>(
            new DataObjects::SplittersWorkspace);

    // 2. Create splitters
    for (size_t i = 0; i < numpulses; ++i) {
      int64_t t0a = runstart_i64 + static_cast<int64_t>(i) * pulsedt +
                    static_cast<int64_t>(static_cast<double>(pulsedt) * 0.2);
      int64_t tfa = runstart_i64 + static_cast<int64_t>(i) * pulsedt +
                    static_cast<int64_t>(static_cast<double>(pulsedt) * 0.4);
      Kernel::SplittingInterval interval_a(t0a, tfa, 0);

      int64_t t0b = runstart_i64 + static_cast<int64_t>(i) * pulsedt +
                    static_cast<int64_t>(static_cast<double>(pulsedt) * 0.6);
      int64_t tfb = runstart_i64 + static_cast<int64_t>(i) * pulsedt +
                    static_cast<int64_t>(static_cast<double>(pulsedt) * 0.8);
      Kernel::SplittingInterval interval_b(t0b, tfb, 1);

      splitterws->addSplitter(interval_a);
      splitterws->addSplitter(interval_b);
    }

    return splitterws;
  }

  //----------------------------------------------------------------------------------------------
  /** Create the time correction table
    */
  TableWorkspace_sptr createTimeCorrectionTable(MatrixWorkspace_sptr inpws) {
    // 1. Generate an empty table
    auto corrtable = boost::make_shared<TableWorkspace>();
    corrtable->addColumn("int", "DetectorID");
    corrtable->addColumn("double", "Correction");

    // 2. Add rows
    Instrument_const_sptr instrument = inpws->getInstrument();
    vector<int> detids = instrument->getDetectorIDs();
    for (size_t i = 0; i < detids.size(); ++i) {
      int detid = detids[i];
      double factor = 0.75;
      TableRow newrow = corrtable->appendRow();
      newrow << detid << factor;
    }

    return corrtable;
  }

  //----------------------------------------------------------------------------------------------
  /** Fake uniform time data more close to SNS case
    * A list of 1000 events
    * Pulse length: 1000000 * nano-second
    */
  EventList fake_uniform_time_sns_data(int64_t runstart, int64_t pulselength) {
    // Clear the list
    EventList el = EventList();

    // Create some mostly-reasonable fake data.
    unsigned seed1 = 1;
    std::minstd_rand0 g1(seed1);

    for (int time = 0; time < 1000; time++) {
      // All pulse times from 0 to 999 in seconds
      Kernel::DateAndTime pulsetime(
          static_cast<int64_t>(time * pulselength + runstart));
      double tof = static_cast<double>(g1() % 1000);
      el += TofEvent(tof, pulsetime);
      // std::cout << "Added 20th event as " << tof << ", " << pulsetime <<
      // "\n";
    }

    return el;
  }

  /** Create a matrix splitters workspace for elastic correction
   * @brief createMatrixSplittersElastic
   * @return
   */
  API::MatrixWorkspace_sptr createMatrixSplittersElastic() {
    MatrixWorkspace_sptr spws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", 1, 11, 10));

    MantidVec &vec_splitTimes = spws->dataX(0);
    MantidVec &vec_splitGroup = spws->dataY(0);

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

  API::MatrixWorkspace_sptr createMatrixSplittersDG() {
    MatrixWorkspace_sptr spws = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceFactory::Instance().create("Workspace2D", 1, 11, 10));

    MantidVec &vec_splitTimes = spws->dataX(0);
    MantidVec &vec_splitGroup = spws->dataY(0);

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

#endif /* MANTID_ALGORITHMS_FILTEREVENTSTEST_H_ */
