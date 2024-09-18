// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidIndexing/SpectrumIndexSet.h"
#include "MantidIndexing/SpectrumNumber.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidNexusGeometry/Hdf5Version.h"

#include "Poco/Path.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using Mantid::Types::Core::DateAndTime;
using Mantid::Types::Event::TofEvent;

void run_multiprocess_load(const std::string &file, bool precount) {
  Mantid::API::FrameworkManager::Instance();
  LoadEventNexus ld;
  ld.initialize();
  ld.setPropertyValue("Loadtype", "Multiprocess (experimental)");
  std::string outws_name = "multiprocess";
  ld.setPropertyValue("Filename", file);
  ld.setPropertyValue("OutputWorkspace", outws_name);
  ld.setPropertyValue("Precount", std::to_string(precount));
  ld.setProperty<bool>("LoadLogs", false); // Time-saver
  TS_ASSERT_THROWS_NOTHING(ld.execute());
  TS_ASSERT(ld.isExecuted())

  EventWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws_name);
  TS_ASSERT(ws);

  LoadEventNexus ldRef;
  ldRef.initialize();
  ldRef.setPropertyValue("Loadtype", "Default");
  outws_name = "reference";
  ldRef.setPropertyValue("Filename", file);
  ldRef.setPropertyValue("OutputWorkspace", outws_name);
  ldRef.setPropertyValue("Precount", "1");
  ldRef.setProperty<bool>("LoadLogs", false); // Time-saver
  TS_ASSERT_THROWS_NOTHING(ldRef.execute());
  TS_ASSERT(ldRef.isExecuted())

  EventWorkspace_sptr wsRef = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws_name);
  TS_ASSERT(wsRef);

  TSM_ASSERT_EQUALS("Different spectrum number in reference ws.", wsRef->getNumberHistograms(),
                    ws->getNumberHistograms());
  if (wsRef->getNumberHistograms() != ws->getNumberHistograms())
    return;
  for (size_t i = 0; i < wsRef->getNumberHistograms(); ++i) {
    auto &eventList = ws->getSpectrum(i).getEvents();
    auto &eventListRef = wsRef->getSpectrum(i).getEvents();
    TSM_ASSERT_EQUALS("Different events number in reference spectra", eventList.size(), eventListRef.size());
    if (eventList.size() != eventListRef.size())
      return;
    for (size_t j = 0; j < eventListRef.size(); ++j) {
      TSM_ASSERT_EQUALS("Events are not equal", eventList[j].tof(), eventListRef[j].tof());
      TSM_ASSERT_EQUALS("Events are not equal", eventList[j].pulseTime(), eventListRef[j].pulseTime());
    }
  }
}

class LoadEventNexusTest : public CxxTest::TestSuite {
private:
  void do_test_filtering_start_and_end_filtered_loading(const bool metadataonly) {
    const std::string wsName = "test_filtering";
    constexpr double filterStart = 1;
    constexpr double filterEnd = 1000;

    LoadEventNexus ld;
    ld.initialize();
    ld.setPropertyValue("OutputWorkspace", wsName);
    ld.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    ld.setProperty("FilterByTimeStart", filterStart);
    ld.setProperty("FilterByTimeStop", filterEnd);
    ld.setProperty("MetaDataOnly", metadataonly);
    ld.setProperty("NumberOfBins", 1); // only one bin to make validation easier

    TS_ASSERT(ld.execute());

    auto outWs = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName);

    Property *prop = outWs->run().getLogData("SampleTemp");
    TSM_ASSERT_EQUALS("Should have 16 elements after filtering.", 16, prop->size());
    if (prop->size() != 16)
      return;
    // Further tests
    TimeSeriesProperty<double> *sampleTemps = dynamic_cast<TimeSeriesProperty<double> *>(prop);
    auto filteredLogStartTime = sampleTemps->nthTime(0);
    auto filteredLogEndTime = sampleTemps->nthTime(sampleTemps->size() - 1);
    TS_ASSERT_EQUALS("2010-Mar-25 16:09:27.620000000", filteredLogStartTime.toSimpleString());
    TS_ASSERT_EQUALS("2010-Mar-25 16:11:51.558003540", filteredLogEndTime.toSimpleString());

    // check the events themselves
    const auto NUM_HIST = outWs->getNumberHistograms();
    TS_ASSERT_EQUALS(NUM_HIST, 51200); // observed value
    if (metadataonly) {
      // check that no events were created
      TS_ASSERT_EQUALS(outWs->getNumberEvents(), 0);
    } else {
      // total number of events
      TS_ASSERT_EQUALS(outWs->getNumberEvents(), 110969); // observed value

      // check some particular spectra - observed values
      TS_ASSERT_EQUALS(outWs->getSpectrum(0).getNumberEvents(), 0);
      TS_ASSERT_EQUALS(outWs->getSpectrum(2).getNumberEvents(), 1);
      TS_ASSERT_EQUALS(outWs->getSpectrum(3).getNumberEvents(), 0);
      TS_ASSERT_EQUALS(outWs->getSpectrum(5).getNumberEvents(), 4);
      TS_ASSERT_EQUALS(outWs->getSpectrum(7).getNumberEvents(), 2);
      TS_ASSERT_EQUALS(outWs->getSpectrum(11).getNumberEvents(), 0);
      TS_ASSERT_EQUALS(outWs->getSpectrum(13).getNumberEvents(), 0);
      TS_ASSERT_EQUALS(outWs->getSpectrum(17).getNumberEvents(), 2);
      TS_ASSERT_EQUALS(outWs->getSpectrum(29).getNumberEvents(), 1);
      constexpr size_t bank2offset = 128 * 12; // half way into bank2
      TS_ASSERT_EQUALS(outWs->getSpectrum(bank2offset + 0).getNumberEvents(), 0);
      TS_ASSERT_EQUALS(outWs->getSpectrum(bank2offset + 2).getNumberEvents(), 0);
      TS_ASSERT_EQUALS(outWs->getSpectrum(bank2offset + 3).getNumberEvents(), 0);
      TS_ASSERT_EQUALS(outWs->getSpectrum(bank2offset + 5).getNumberEvents(), 0);
      TS_ASSERT_EQUALS(outWs->getSpectrum(bank2offset + 7).getNumberEvents(), 1);
      TS_ASSERT_EQUALS(outWs->getSpectrum(bank2offset + 11).getNumberEvents(), 2);
      TS_ASSERT_EQUALS(outWs->getSpectrum(bank2offset + 13).getNumberEvents(), 1);
      TS_ASSERT_EQUALS(outWs->getSpectrum(bank2offset + 17).getNumberEvents(), 1);
      TS_ASSERT_EQUALS(outWs->getSpectrum(bank2offset + 29).getNumberEvents(), 1);
      constexpr size_t bank4offset = 128 * 8 * 4 + 128 * 4; // half way into bank4
      TS_ASSERT_EQUALS(outWs->getSpectrum(bank4offset + 0).getNumberEvents(), 0);
      TS_ASSERT_EQUALS(outWs->getSpectrum(bank4offset + 2).getNumberEvents(), 0);
      TS_ASSERT_EQUALS(outWs->getSpectrum(bank4offset + 3).getNumberEvents(), 0);
      TS_ASSERT_EQUALS(outWs->getSpectrum(bank4offset + 5).getNumberEvents(), 2);
      TS_ASSERT_EQUALS(outWs->getSpectrum(bank4offset + 7).getNumberEvents(), 1);
      TS_ASSERT_EQUALS(outWs->getSpectrum(bank4offset + 11).getNumberEvents(), 0);
      TS_ASSERT_EQUALS(outWs->getSpectrum(bank4offset + 13).getNumberEvents(), 2);
      TS_ASSERT_EQUALS(outWs->getSpectrum(bank4offset + 17).getNumberEvents(), 1);
      TS_ASSERT_EQUALS(outWs->getSpectrum(bank4offset + 29).getNumberEvents(), 3);
    }
  }

  void validate_pulse_time_sorting(EventWorkspace_sptr eventWS) {
    for (size_t i = 0; i < eventWS->getNumberHistograms(); i++) {
      auto eventList = eventWS->getSpectrum(i);
      if (eventList.getSortType() == DataObjects::PULSETIME_SORT) {
        std::vector<DateAndTime> pulsetimes;
        for (auto &event : eventList.getEvents()) {
          pulsetimes.emplace_back(event.pulseTime());
        }
        TS_ASSERT(std::is_sorted(pulsetimes.cbegin(), pulsetimes.cend()));
      }
    }
  }

  /*
   * Verify that the compressed and uncompressed workspaces have the same number of counts per pixel and reasonable
   * number of events.
   */
  void validateUncompressedCompressed(EventWorkspace_sptr ws_uncompressed, EventWorkspace_sptr ws_compressed,
                                      const std::size_t NUM_HIST, const EventType uncompressed_type = EventType::TOF) {
    TS_ASSERT_EQUALS(ws_uncompressed->getNumberHistograms(), NUM_HIST);
    TS_ASSERT_EQUALS(ws_compressed->getNumberHistograms(), NUM_HIST);

    // Compressed should no more events than the uncompressed
    TS_ASSERT_LESS_THAN_EQUALS(ws_compressed->getNumberEvents(), ws_uncompressed->getNumberEvents());

    for (size_t wi = 0; wi < NUM_HIST; wi++) {
      // total counts in uncompressed and compressed should be equal
      TS_ASSERT_EQUALS(ws_compressed->readY(wi), ws_uncompressed->readY(wi));

      // all uncompressed spectra should be raw events
      TS_ASSERT_EQUALS(ws_uncompressed->getSpectrum(wi).getEventType(), uncompressed_type);

      // pixels with at least one event will have switched to weighted
      if (ws_compressed->getSpectrum(wi).getNumberEvents() > 0)
        TS_ASSERT_EQUALS(ws_compressed->getSpectrum(wi).getEventType(), EventType::WEIGHTED_NOTIME);
    }

    // std::cout << "Uncompressed " << ws_uncompressed->getMemorySizeAsStr() << " vs Compressed "
    //           << ws_compressed->getMemorySizeAsStr() << "\n";
  }

public:
  void test_load_event_nexus_v20_ess() {
    const std::string file = "V20_ESS_example.nxs";
    LoadEventNexus alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("Filename", file);
    alg.setProperty("OutputWorkspace", "dummy_for_child");
    alg.execute();
    Workspace_sptr ws = alg.getProperty("OutputWorkspace");
    auto eventWS = std::dynamic_pointer_cast<EventWorkspace>(ws);
    TS_ASSERT(eventWS);

    TS_ASSERT_EQUALS(eventWS->getNumberEvents(), 1439);
    TS_ASSERT_EQUALS(eventWS->detectorInfo().size(),
                     (150 * 150) + 2) // Two monitors
  }

  void test_load_event_nexus_v20_ess_log_filtered() {
    const std::string file = "V20_ESS_example.nxs";
    std::vector<std::string> allowed = {"proton_charge", "S2HGap", "S2VGap"};

    LoadEventNexus alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("Filename", file);
    alg.setProperty("AllowList", allowed);
    alg.setProperty("OutputWorkspace", "dummy_for_child");
    alg.execute();
    Workspace_sptr ws = alg.getProperty("OutputWorkspace");
    auto eventWS = std::dynamic_pointer_cast<EventWorkspace>(ws);
    TS_ASSERT(eventWS);

    TS_ASSERT_EQUALS(eventWS->getNumberEvents(), 1439);
    TS_ASSERT_EQUALS(eventWS->detectorInfo().size(),
                     (150 * 150) + 2) // Two monitors

    // this file contains events that are sorted in pulse time order
    validate_pulse_time_sorting(eventWS);
  }

  void test_load_event_nexus_v20_ess_integration_2018() {
    // Only perform this test if the version of hdf5 supports vlen strings
    if (NexusGeometry::Hdf5Version::checkVariableLengthStringSupport()) {
      const std::string file = "V20_ESSIntegration_2018-12-13_0942.nxs";
      LoadEventNexus alg;
      alg.setChild(true);
      alg.setRethrows(true);
      alg.initialize();
      alg.setProperty("Filename", file);
      alg.setProperty("OutputWorkspace", "dummy_for_child");
      alg.execute();
      Workspace_sptr ws = alg.getProperty("OutputWorkspace");
      auto eventWS = std::dynamic_pointer_cast<EventWorkspace>(ws);
      TS_ASSERT(eventWS);

      TS_ASSERT_EQUALS(eventWS->getNumberEvents(), 43277);
      TS_ASSERT_EQUALS(eventWS->detectorInfo().size(),
                       (300 * 300) + 2) // Two monitors
      TS_ASSERT_DELTA(eventWS->getTofMin(), 9.815, 1.0e-3);
      TS_ASSERT_DELTA(eventWS->getTofMax(), 130748.563, 1.0e-3);

      // this file contains events that aren't sorted in pulse time order but the event lists per spectra are sorted
      validate_pulse_time_sorting(eventWS);
    }
  }

  void test_load_event_nexus_POLARIS() {
    // POLARIS file slow to create geometry cache so use a pregenerated vtp file. Details of the geometry don't matter
    // for this test
    const std::string vtpDirectoryKey = "instrumentDefinition.vtp.directory";
    std::string foundFile =
        Kernel::ConfigService::Instance().getFullPath("POLARIS9fbf7121b4274c833043ae8933ec643ff7b9313d.vtp", true, 0);
    bool hasVTPDirectory = ConfigService::Instance().hasProperty(vtpDirectoryKey);
    auto origVTPDirectory = ConfigService::Instance().getString(vtpDirectoryKey);
    ConfigService::Instance().setString(vtpDirectoryKey, Poco::Path(foundFile).parent().toString());
    const std::string file = "POLARIS00130512.nxs";
    LoadEventNexus alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("Filename", file);
    alg.setProperty("OutputWorkspace", "dummy_for_child");
    alg.execute();
    Workspace_sptr ws = alg.getProperty("OutputWorkspace");
    auto eventWS = std::dynamic_pointer_cast<EventWorkspace>(ws);
    TS_ASSERT(eventWS);

    TS_ASSERT_EQUALS(eventWS->getNumberEvents(), 19268117);
    TS_ASSERT_DELTA(eventWS->getTofMin(), 0., 1.0e-3);
    TS_ASSERT_DELTA(eventWS->getTofMax(), 19994.945, 1.0e-3);

    // this file contains events that aren't sorted in pulse time order, even per spectra
    validate_pulse_time_sorting(eventWS);
    if (hasVTPDirectory)
      ConfigService::Instance().setString(vtpDirectoryKey, origVTPDirectory);
    else
      ConfigService::Instance().remove(vtpDirectoryKey);
  }

  void test_NumberOfBins() {
    const std::string file = "SANS2D00022048.nxs";
    int nBins = 273;
    LoadEventNexus alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("Filename", file);
    alg.setProperty("OutputWorkspace", "dummy_for_child");
    alg.setProperty("NumberOfBins", nBins);
    alg.execute();
    Workspace_sptr ws = alg.getProperty("OutputWorkspace");
    auto eventWS = std::dynamic_pointer_cast<EventWorkspace>(ws);
    TS_ASSERT(eventWS);

    TS_ASSERT_EQUALS(eventWS->blocksize(), nBins);
  }

  void test_load_event_nexus_sans2d_ess() {
    const std::string file = "SANS2D_ESS_example.nxs";
    LoadEventNexus alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("Filename", file);
    alg.setProperty("OutputWorkspace", "dummy_for_child");
    alg.setProperty("NumberOfBins", 1);
    alg.execute();
    Workspace_sptr ws = alg.getProperty("OutputWorkspace");
    auto eventWS = std::dynamic_pointer_cast<EventWorkspace>(ws);
    TS_ASSERT(eventWS);

    TS_ASSERT_EQUALS(eventWS->getNumberEvents(), 14258850);
    TS_ASSERT_EQUALS(eventWS->counts(0)[0], 0.0);
    TS_ASSERT_EQUALS(eventWS->counts(1)[0], 2.0);
    TS_ASSERT_EQUALS(eventWS->counts(2)[0], 1.0);
    TS_ASSERT_EQUALS(eventWS->counts(122879)[0],
                     4.0); // Regession test for miss
                           // setting max detector and
                           // subsequent incorrect event
                           // count
    TS_ASSERT_EQUALS(eventWS->indexInfo().spectrumNumber(0), 1);
    TS_ASSERT_EQUALS(eventWS->indexInfo().spectrumNumber(1), 2);
    TS_ASSERT_EQUALS(eventWS->indexInfo().spectrumNumber(2), 3);
  }

#ifdef _WIN32
  bool windows = true;
#else
  bool windows = false;
#endif // _WIN32
  void test_multiprocess_loader_precount() {
    if (!windows) {
      run_multiprocess_load("SANS2D00022048.nxs", true);
      run_multiprocess_load("LARMOR00003368.nxs", true);
    }
  }

  void test_multiprocess_loader_producer_consumer() {
    if (!windows) {
      run_multiprocess_load("SANS2D00022048.nxs", false);
      run_multiprocess_load("LARMOR00003368.nxs", false);
    }
  }

  void test_SingleBank_PixelsOnlyInThatBank() { doTestSingleBank(true, false); }

  void test_load_event_nexus_ornl_eqsans() {
    // This file has a 2D entry/sample/name
    const std::string file = "EQSANS_89157.nxs.h5";
    LoadEventNexus alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("Filename", file);
    alg.setProperty("MetaDataOnly", true);
    alg.setProperty("OutputWorkspace", "dummy_for_child");
    alg.execute();
    Workspace_sptr ws = alg.getProperty("OutputWorkspace");
    auto eventWS = std::dynamic_pointer_cast<EventWorkspace>(ws);
    TS_ASSERT(eventWS);
    const double duration = eventWS->mutableRun().getPropertyValueAsType<double>("duration");
    TS_ASSERT_DELTA(duration, 7200.012, 0.01);
  }

  void test_wallclock_filtering() {
    const std::string wsName("test_wallclock_filtering");
    const std::string filename("EQSANS_89157.nxs.h5");
    constexpr double filterStart{200}; // seconds
    constexpr double filterEnd{5000};  // seconds
    constexpr size_t NUM_HIST{49152};  // observed value

    // number of events - all are observed
    constexpr size_t NUM_EVENTS_FULL{14553};
    constexpr size_t NUM_EVENTS_BEGIN{366};
    constexpr size_t NUM_EVENTS_END{4353};

    { // first time is unfiltered
      LoadEventNexus alg;
      alg.initialize();
      alg.setRethrows(true);
      alg.setPropertyValue("Filename", filename);
      alg.setPropertyValue("OutputWorkspace", wsName);
      alg.setProperty("NumberOfBins", 1); // only one bin to make validation easier
      TS_ASSERT(alg.execute());

      // get the workspace
      auto outWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName);
      TS_ASSERT(outWS);

      TS_ASSERT_EQUALS(outWS->getNumberHistograms(), NUM_HIST);
      TS_ASSERT_EQUALS(outWS->getNumberEvents(), NUM_EVENTS_FULL);
    }

    { // filter only the beginning
      LoadEventNexus alg;
      alg.initialize();
      alg.setRethrows(true);
      alg.setPropertyValue("Filename", filename);
      alg.setPropertyValue("OutputWorkspace", wsName);
      alg.setProperty("FilterByTimeStart", filterStart);
      TS_ASSERT(alg.execute());

      // get the workspace
      auto outWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName);
      TS_ASSERT(outWS);

      TS_ASSERT_EQUALS(outWS->getNumberHistograms(), NUM_HIST);
      TS_ASSERT_EQUALS(outWS->getNumberEvents(), NUM_EVENTS_FULL - NUM_EVENTS_BEGIN);
    }

    { // filter only the end
      LoadEventNexus alg;
      alg.initialize();
      alg.setRethrows(true);
      alg.setPropertyValue("Filename", filename);
      alg.setPropertyValue("OutputWorkspace", wsName);
      alg.setProperty("FilterByTimeStop", filterEnd);
      alg.setProperty("NumberOfBins", 1); // only one bin to make validation easier
      TS_ASSERT(alg.execute());

      // get the workspace
      auto outWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName);
      TS_ASSERT(outWS);

      TS_ASSERT_EQUALS(outWS->getNumberHistograms(), NUM_HIST);
      TS_ASSERT_EQUALS(outWS->getNumberEvents(), NUM_EVENTS_FULL - NUM_EVENTS_END);
    }

    { // filter both
      LoadEventNexus alg;
      alg.initialize();
      alg.setRethrows(true);
      alg.setPropertyValue("Filename", filename);
      alg.setPropertyValue("OutputWorkspace", wsName);
      alg.setProperty("FilterByTimeStart", filterStart);
      alg.setProperty("FilterByTimeStop", filterEnd);
      alg.setProperty("NumberOfBins", 1); // only one bin to make validation easier
      TS_ASSERT(alg.execute());

      // get the workspace
      auto outWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName);
      TS_ASSERT(outWS);

      TS_ASSERT_EQUALS(outWS->getNumberHistograms(), NUM_HIST);
      TS_ASSERT_EQUALS(outWS->getNumberEvents(), NUM_EVENTS_FULL - NUM_EVENTS_BEGIN - NUM_EVENTS_END);
    }

    // cleanup assumes the the last one of these worked
    AnalysisDataService::Instance().remove(wsName);
  }

  // FilteredLoadvsLoadThenFilter system test and algorithm usage example
  void test_CNCS_7860_filtering() {
    const std::string filename("CNCS_7860_event.nxs");
    const std::string wsName("CNCS_7860");
    constexpr double filterStart{60};
    constexpr double filterEnd{120};
    constexpr size_t NUM_HIST{8 * 128 * 50};
    constexpr size_t NUM_EVENTS{29753};

    LoadEventNexus alg;
    alg.initialize();
    alg.setPropertyValue("Filename", filename);
    alg.setPropertyValue("OutputWorkspace", wsName);
    alg.setProperty("FilterByTimeStart", filterStart);
    alg.setProperty("FilterByTimeStop", filterEnd);
    alg.setProperty("NumberOfBins", 1); // only one bin to make validation easier
    TS_ASSERT(alg.execute());

    // get the workspace
    auto outWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName);
    TS_ASSERT(outWS);

    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), NUM_HIST);
    TS_ASSERT_EQUALS(outWS->getNumberEvents(), NUM_EVENTS);

    // number of empty spectra should match
    size_t numEmpty{0};
    for (std::size_t wi = 0; wi < outWS->getNumberHistograms(); ++wi)
      if (outWS->getSpectrum(wi).empty())
        numEmpty++;
    TS_ASSERT_EQUALS(numEmpty, 31411); // observed from running LoadEventNexus + FilterByTime

    // these are pixels that were showing the wrong behavior during testing
    // should have one event [4325, 20673, 27475, 30675, 46869]
    // near the magic pulse-time of missing events is 2010-Mar-25 16:10:36.997398376 when stoptime is 16:10:37
    TS_ASSERT_EQUALS(outWS->getSpectrum(4325).getNumberEvents(), 1);
    TS_ASSERT_EQUALS(outWS->getSpectrum(20673).getNumberEvents(), 1);
    TS_ASSERT_EQUALS(outWS->getSpectrum(27475).getNumberEvents(), 1);
    TS_ASSERT_EQUALS(outWS->getSpectrum(30675).getNumberEvents(), 1);
    TS_ASSERT_EQUALS(outWS->getSpectrum(46869).getNumberEvents(), 1);

    // cleanup assumes this worked
    AnalysisDataService::Instance().remove(wsName);
  }

  void test_Normal_vs_Precount() {
    Mantid::API::FrameworkManager::Instance();
    LoadEventNexus ld;
    std::string outws_name = "cncs_noprecount";
    ld.initialize();
    ld.setRethrows(true);
    ld.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    ld.setPropertyValue("OutputWorkspace", outws_name);
    ld.setPropertyValue("Precount", "0");
    ld.setProperty("NumberOfBins", 1);
    ld.setProperty<bool>("LoadLogs", false); // Time-saver
    ld.execute();
    TS_ASSERT(ld.isExecuted());

    EventWorkspace_sptr WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws_name);
    // Valid WS and it is an EventWorkspace
    TS_ASSERT(WS);
    // Pixels have to be padded
    TS_ASSERT_EQUALS(WS->getNumberHistograms(), 51200);
    // Events
    TS_ASSERT_EQUALS(WS->getNumberEvents(), 112266);
    // TOF limits found. There is a pad of +-1 given around the actual TOF
    // founds.
    TS_ASSERT_DELTA((*WS->refX(0))[0], 44163.6, 0.05);
    TS_ASSERT_DELTA((*WS->refX(0))[1], 60830.2, 0.05);
    // Valid spectrum info
    TS_ASSERT_EQUALS(WS->getSpectrum(0).getSpectrumNo(), 1);
    TS_ASSERT_EQUALS(WS->getSpectrum(0).getDetectorIDs().size(), 1);
    TS_ASSERT_EQUALS(*WS->getSpectrum(0).getDetectorIDs().begin(), 0);

    // Check one event from one pixel - does it have a reasonable pulse time
    TS_ASSERT(WS->getSpectrum(1000).getEvents()[0].pulseTime() > DateAndTime(int64_t(1e9 * 365 * 10)));

    // Check filename
    TS_ASSERT_EQUALS(ld.getPropertyValue("Filename"), WS->run().getProperty("Filename")->value());

    // Test that asking not to load the logs did what it should
    // Make sure that we throw if we try to read a log (that shouldn't be there)
    TS_ASSERT_THROWS(WS->getLog("proton_charge"), const std::invalid_argument &);

    //----- Now we re-load with precounting and compare memory use ----
    LoadEventNexus ld2;
    std::string outws_name2 = "cncs_precount";
    ld2.initialize();
    ld2.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    ld2.setPropertyValue("OutputWorkspace", outws_name2);
    ld2.setPropertyValue("Precount", "1");
    ld2.setProperty<bool>("LoadLogs", false); // Time-saver
    ld2.setProperty("NumberOfBins", 1);
    ld2.execute();
    TS_ASSERT(ld2.isExecuted());

    EventWorkspace_sptr WS2 = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws_name2);
    // Valid WS and it is an EventWorkspace
    TS_ASSERT(WS2);

    TS_ASSERT_EQUALS(WS->getNumberEvents(), WS2->getNumberEvents());
    // Memory used should be lower (or the same at worst)
    TS_ASSERT_LESS_THAN_EQUALS(WS2->getMemorySize(), WS->getMemorySize());

    // Longer, more thorough test
    if (false) {
      auto load = AlgorithmManager::Instance().create("LoadEventPreNexus", 1);
      load->setPropertyValue("OutputWorkspace", "cncs_pre");
      load->setPropertyValue("EventFilename", "CNCS_7860_neutron_event.dat");
      load->setPropertyValue("PulseidFilename", "CNCS_7860_pulseid.dat");
      load->setPropertyValue("MappingFilename", "CNCS_TS_2008_08_18.dat");
      load->execute();
      TS_ASSERT(load->isExecuted());
      EventWorkspace_sptr WS2 = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("cncs_pre");
      // Valid WS and it is an EventWorkspace
      TS_ASSERT(WS2);

      // Let's compare the proton_charge logs
      TimeSeriesProperty<double> *log =
          dynamic_cast<TimeSeriesProperty<double> *>(WS->mutableRun().getProperty("proton_charge"));
      std::map<DateAndTime, double> logMap = log->valueAsCorrectMap();
      TimeSeriesProperty<double> *log2 =
          dynamic_cast<TimeSeriesProperty<double> *>(WS2->mutableRun().getProperty("proton_charge"));
      std::map<DateAndTime, double> logMap2 = log2->valueAsCorrectMap();
      std::map<DateAndTime, double>::iterator it, it2;

      it = logMap.begin();
      it2 = logMap2.begin();
      for (; it != logMap.end();) {
        // Same times within a millisecond
        // TS_ASSERT_DELTA( it->first, it2->first,
        // DateAndTime::durationFromSeconds(1e-3));
        // Same times?
        TS_ASSERT_LESS_THAN(fabs(DateAndTime::secondsFromDuration(it->first - it2->first)),
                            1); // TODO: Fix the nexus file times here
        // Same proton charge?
        TS_ASSERT_DELTA(it->second, it2->second, 1e-5);
        it++;
        it2++;
      }

      constexpr std::size_t pixelID = 2000;

      const std::vector<TofEvent> events1 = WS->getSpectrum(pixelID).getEvents();
      const std::vector<TofEvent> events2 = WS2->getSpectrum(pixelID).getEvents();

      // std::cout << events1.size() << '\n';
      TS_ASSERT_EQUALS(events1.size(), events2.size());
      if (events1.size() == events2.size()) {
        for (size_t i = 0; i < events1.size(); i++) {
          TS_ASSERT_DELTA(events1[i].tof(), events2[i].tof(), 0.05);
          TS_ASSERT_DELTA(events1[i].pulseTime(), events2[i].pulseTime(),
                          1e9); // TODO:: Fix nexus start times
          // std::cout << (events1[i].pulseTime()-start)/1e9 << " - " <<
          // (events2[i].pulseTime()-start)/1e9 << " sec\n";
          // std::cout << "Pulse time diff " << (events1[i].pulseTime() -
          // events2[i].pulseTime())/1e9 << " sec\n";
        }
      }
    }
  }

  void test_TOF_filtered_loading() {
    const std::string wsName = "test_filtering";
    const double filterStart = 45000;
    const double filterEnd = 59000;

    LoadEventNexus ld;
    ld.initialize();
    ld.setPropertyValue("OutputWorkspace", wsName);
    ld.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    ld.setProperty("FilterByTofMin", filterStart);
    ld.setProperty("FilterByTofMax", filterEnd);
    ld.setProperty<bool>("LoadLogs", false); // Time-saver

    TS_ASSERT(ld.execute());

    auto outWs = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName);

    auto eventList = outWs->getSpectrum(4348);
    auto events = eventList.getEvents();

    double max = events.begin()->tof();
    double min = events.begin()->tof();
    for (auto &event : events) {
      max = event.tof() > max ? event.tof() : max;
      min = event.tof() < min ? event.tof() : min;
    }
    TSM_ASSERT("The max TOF in the workspace should be equal to or less than "
               "the filtered cut-off",
               max <= filterEnd);
    TSM_ASSERT("The min TOF in the workspace should be equal to or greater "
               "than the filtered cut-off",
               min >= filterStart);
  }

  void test_partial_spectra_loading() {
    std::string wsName = "test_partial_spectra_loading_SpectrumList";
    std::vector<int32_t> specList;
    specList.emplace_back(13);
    specList.emplace_back(16);
    specList.emplace_back(21);
    specList.emplace_back(28);

    // A) test SpectrumList
    LoadEventNexus ld;
    ld.initialize();
    ld.setPropertyValue("OutputWorkspace", wsName);
    ld.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    ld.setProperty("SpectrumList", specList);
    ld.setProperty<bool>("LoadLogs", false); // Time-saver

    TS_ASSERT(ld.execute());

    auto outWs = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName);

    TSM_ASSERT("The number of spectra in the workspace should be equal to the "
               "spectra filtered",
               outWs->getNumberHistograms() == specList.size());
    // Spectrum numbers match those that same detector would have in unfiltered
    // load, in this case detID + 1 since IDs in instrument start at 0.
    TS_ASSERT_EQUALS(outWs->getSpectrum(0).getSpectrumNo(), 14);
    TS_ASSERT_EQUALS(outWs->getSpectrum(1).getSpectrumNo(), 17);
    TS_ASSERT_EQUALS(outWs->getSpectrum(2).getSpectrumNo(), 22);
    TS_ASSERT_EQUALS(outWs->getSpectrum(3).getSpectrumNo(), 29);

    // B) test SpectrumMin and SpectrumMax
    wsName = "test_partial_spectra_loading_SpectrumMin_SpectrumMax";
    const size_t specMin = 10;
    const size_t specMax = 29;
    LoadEventNexus ldMinMax;
    ldMinMax.initialize();
    ldMinMax.setPropertyValue("OutputWorkspace", wsName);
    ldMinMax.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    ldMinMax.setProperty<int>("SpectrumMin", specMin);
    ldMinMax.setProperty<int>("SpectrumMax", specMax);
    ldMinMax.setProperty<bool>("LoadLogs", false); // Time-saver

    TS_ASSERT(ldMinMax.execute());

    outWs = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName);

    // check number and indices of spectra
    const size_t numSpecs = specMax - specMin + 1;
    TS_ASSERT_EQUALS(outWs->getNumberHistograms(), numSpecs);
    // Spectrum numbers match those that same detector would have in unfiltered
    // load, in this case detID + 1 since IDs in instrument start at 0.
    for (size_t specIdx = 0; specIdx < numSpecs; specIdx++) {
      TS_ASSERT_EQUALS(outWs->getSpectrum(specIdx).getSpectrumNo(), static_cast<int>(specMin + specIdx + 1));
    }

    // C) test SpectrumList + SpectrumMin and SpectrumMax
    // This will make: 17, 20, 21, 22, 23
    wsSpecFilterAndEventMonitors = "test_partial_spectra_loading_SpectrumList_SpectrumMin_SpectrumMax";
    const size_t sMin = 20;
    const size_t sMax = 22;
    specList.clear();
    specList.emplace_back(17);

    LoadEventNexus ldLMM;
    ldLMM.initialize();
    ldLMM.setPropertyValue("OutputWorkspace", wsSpecFilterAndEventMonitors);
    ldLMM.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    ldLMM.setProperty("SpectrumList", specList);
    ldLMM.setProperty<int>("SpectrumMin", sMin);
    ldLMM.setProperty<int>("SpectrumMax", sMax);
    ldLMM.setProperty<bool>("LoadLogs", false); // Time-saver
    // Note: this is done here to avoid additional loads
    // This will produce a workspace with suffix _monitors, that is used below
    // in test_CNCSMonitors
    ldLMM.setProperty<bool>("LoadMonitors", true);

    TS_ASSERT(ldLMM.execute());

    outWs = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsSpecFilterAndEventMonitors);

    // check number and indices of spectra
    const size_t n = sMax - sMin + 1;                      // this n is the 20...22, excluding '17'
    TS_ASSERT_EQUALS(outWs->getNumberHistograms(), n + 1); // +1 is the '17'
    // Spectrum numbers match those that same detector would have in unfiltered
    // load, in this case detID + 1 since IDs in instrument start at 0.
    // 18 should come from SpectrumList
    TS_ASSERT_EQUALS(outWs->getSpectrum(0).getSpectrumNo(), 18);
    // and then sMin(20)...sMax(22)
    for (size_t specIdx = 0; specIdx < n; specIdx++) {
      TS_ASSERT_EQUALS(outWs->getSpectrum(specIdx + 1).getSpectrumNo(), static_cast<int>(sMin + specIdx + 1));
    }
  }

  void test_partial_spectra_loading_ISIS() {
    // This is to test a specific bug where if you selected any spectra and had
    // precount on you got double the number of events
    std::string wsName = "test_partial_spectra_loading_SpectrumListISIS";
    std::string wsName2 = "test_partial_spectra_loading_SpectrumListISIS2";
    std::string filename = "OFFSPEC00036416.nxs";
    std::vector<int32_t> specList;
    specList.emplace_back(45);

    LoadEventNexus ld;
    ld.initialize();
    ld.setPropertyValue("OutputWorkspace", wsName);
    ld.setPropertyValue("Filename", filename);
    ld.setProperty("SpectrumMin", 10);
    ld.setProperty("SpectrumMax", 20);
    ld.setProperty("SpectrumList", specList);
    ld.setProperty<bool>("Precount", false);
    ld.setProperty<bool>("LoadLogs", false); // Time-saver

    TS_ASSERT(ld.execute());

    LoadEventNexus ld2;
    ld2.initialize();
    ld2.setPropertyValue("OutputWorkspace", wsName2);
    ld2.setPropertyValue("Filename", filename);
    ld2.setProperty("SpectrumMin", 10);
    ld2.setProperty("SpectrumMax", 20);
    ld2.setProperty("SpectrumList", specList);
    ld2.setProperty<bool>("Precount", true);
    ld2.setProperty<bool>("LoadLogs", false); // Time-saver

    TS_ASSERT(ld2.execute());

    auto outWs = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName);
    auto outWs2 = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName2);

    TSM_ASSERT_EQUALS("The number of spectra in the workspace should be 12", outWs->getNumberHistograms(), 12);

    TSM_ASSERT_EQUALS("The number of events in the precount and not precount "
                      "workspaces do not match",
                      outWs->getNumberEvents(), outWs2->getNumberEvents());

    TSM_ASSERT("Some spectra were not found in the workspace", outWs->getSpectrum(0).getSpectrumNo() == 10);

    TSM_ASSERT("Some spectra were not found in the workspace", outWs->getSpectrum(10).getSpectrumNo() == 20);
    TSM_ASSERT("Some spectra were not found in the workspace", outWs->getSpectrum(11).getSpectrumNo() == 45);

    AnalysisDataService::Instance().remove(wsName);
    AnalysisDataService::Instance().remove(wsName2);
  }

  void test_CNCSMonitors() {
    // Re-uses the workspace loaded in test_partial_spectra_loading to save a
    // load execution
    // This is a very simple test for performance issues. There's no real event
    // data, so this just check that the algorithm creates a consistent output
    // (monitors). Real/intensive testing happens in `LoadNexusMonitors` and
    // system
    // tests.
    const std::string mon_outws_name = wsSpecFilterAndEventMonitors + "_monitors";
    auto &ads = AnalysisDataService::Instance();

    // Valid workspace and it is an event workspace
    const auto monWS = ads.retrieveWS<MatrixWorkspace>(mon_outws_name);

    TS_ASSERT(monWS);
    TS_ASSERT_EQUALS(monWS->getTitle(), "test after manual intervention");

    // Check link data --> monitor workspaces
    TS_ASSERT_EQUALS(monWS, ads.retrieveWS<MatrixWorkspace>(wsSpecFilterAndEventMonitors)->monitorWorkspace());
  }

  void test_Load_And_CompressEvents() {
    constexpr std::size_t NUM_HIST{51200};
    const std::string filename{"CNCS_7860_event.nxs"};

    Mantid::API::FrameworkManager::Instance();

    // create uncompressed - first so turning off compression isn't needed
    std::string uncompressed_name = "cncs_uncompressed";
    {
      LoadEventNexus ld;
      ld.initialize();
      ld.setPropertyValue("Filename", filename);
      ld.setPropertyValue("OutputWorkspace", uncompressed_name);
      ld.setProperty<bool>("Precount", false);
      ld.setProperty<bool>("LoadMonitors", true); // For the next test, saving a load
      ld.setProperty<bool>("LoadLogs", false);    // Time-saver
      ld.setProperty("NumberOfBins", 1);
      ld.execute();
      TS_ASSERT(ld.isExecuted());
    }
    // get a reference to the uncompressed workspace
    EventWorkspace_sptr ws_uncompressed;
    TS_ASSERT_THROWS_NOTHING(ws_uncompressed =
                                 AnalysisDataService::Instance().retrieveWS<EventWorkspace>(uncompressed_name));
    TS_ASSERT(ws_uncompressed); // it is an EventWorkspace

    // create compressed
    std::string compressed_name = "cncs_compressed";
    {
      LoadEventNexus ld;
      ld.initialize();
      ld.setPropertyValue("Filename", filename);
      ld.setPropertyValue("OutputWorkspace", compressed_name);
      ld.setProperty<bool>("Precount", false);
      ld.setProperty<bool>("LoadMonitors", true); // For the next test, saving a load
      ld.setProperty<bool>("LoadLogs", false);    // Time-saver
      ld.setPropertyValue("CompressTolerance", "0.05");
      ld.setProperty("NumberOfBins", 1);
      ld.execute();
      TS_ASSERT(ld.isExecuted());
    }
    // get a reference to the uncompressed workspace
    EventWorkspace_sptr ws_compressed;
    TS_ASSERT_THROWS_NOTHING(ws_compressed =
                                 AnalysisDataService::Instance().retrieveWS<EventWorkspace>(compressed_name));
    TS_ASSERT(ws_compressed); // it is an EventWorkspace

    // validate the compressed workspace makes sense compared to uncompressed
    validateUncompressedCompressed(ws_uncompressed, ws_compressed, NUM_HIST);

    // cleanup - intentionally leave compressed workspace behind for test_Monitors
    AnalysisDataService::Instance().remove(uncompressed_name);
  }

  void test_Monitors() {
    // Uses the workspace loaded in the last test to save a load execution
    std::string mon_outws_name = "cncs_compressed_monitors";
    auto &ads = AnalysisDataService::Instance();
    MatrixWorkspace_sptr WS = ads.retrieveWS<MatrixWorkspace>(mon_outws_name);
    // Valid WS and it is an MatrixWorkspace
    TS_ASSERT(WS);
    // Correct number of monitors found
    TS_ASSERT_EQUALS(WS->getNumberHistograms(), 3);
    // Check some histogram data
    // TOF
    TS_ASSERT_EQUALS((*WS->refX(0)).size(), 200002);
    TS_ASSERT_DELTA((*WS->refX(0))[1], 1.0, 1e-6);
    // Data
    TS_ASSERT_EQUALS(WS->dataY(0).size(), 200001);
    TS_ASSERT_DELTA(WS->dataY(0)[12], 0.0, 1e-6);
    // Error
    TS_ASSERT_EQUALS(WS->dataE(0).size(), 200001);
    TS_ASSERT_DELTA(WS->dataE(0)[12], 0.0, 1e-6);
    // Check geometry for a monitor
    const auto &specInfo = WS->spectrumInfo();
    TS_ASSERT(specInfo.isMonitor(2));
    TS_ASSERT_EQUALS(specInfo.detector(2).getID(), -3);
    TS_ASSERT_DELTA(specInfo.samplePosition().distance(specInfo.position(2)), 1.426, 1e-6);

    // Check monitor workspace pointer held in main workspace
    TS_ASSERT_EQUALS(WS, ads.retrieveWS<MatrixWorkspace>("cncs_compressed")->monitorWorkspace());
  }

  void test_Load_And_Filter_Everything() {
    // This test set the FilterByTimeStart value so that everything should be filtered
    // So we should end up with 0 events
    const std::string filename{"ARCS_sim_event.nxs"};
    std::string ws_name = "arcs_filtered0";
    LoadEventNexus ld;
    ld.initialize();
    ld.setPropertyValue("Filename", filename);
    ld.setPropertyValue("OutputWorkspace", ws_name);
    ld.setPropertyValue("FilterByTimeStart", "1000");
    ld.setProperty("NumberOfBins", 1);
    ld.execute();
    TS_ASSERT(ld.isExecuted());

    EventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(ws_name));
    TS_ASSERT(ws); // it is an EventWorkspace

    TS_ASSERT_EQUALS(ws->getNumberEvents(), 0);
  }

  void test_Load_And_CompressEvents_weighted() {
    constexpr std::size_t NUM_HIST{117760};
    const std::string filename{"ARCS_sim_event.nxs"};

    Mantid::API::FrameworkManager::Instance();

    // create uncompressed - first so turning off compression isn't needed
    std::string uncompressed_name = "arcs_uncompressed";
    {
      LoadEventNexus ld;
      ld.initialize();
      ld.setPropertyValue("Filename", filename);
      ld.setPropertyValue("OutputWorkspace", uncompressed_name);
      ld.setProperty<bool>("Precount", false);
      ld.setProperty<bool>("LoadLogs", false); // Time-saver
      ld.setProperty("NumberOfBins", 1);
      ld.execute();
      TS_ASSERT(ld.isExecuted());
    }
    // get a reference to the uncompressed workspace
    EventWorkspace_sptr ws_uncompressed;
    TS_ASSERT_THROWS_NOTHING(ws_uncompressed =
                                 AnalysisDataService::Instance().retrieveWS<EventWorkspace>(uncompressed_name));
    TS_ASSERT(ws_uncompressed); // it is an EventWorkspace

    // create compressed
    std::string compressed_name = "arcs_compressed";
    {
      LoadEventNexus ld;
      ld.initialize();
      ld.setPropertyValue("Filename", filename);
      ld.setPropertyValue("OutputWorkspace", compressed_name);
      ld.setProperty<bool>("Precount", false);
      ld.setProperty<bool>("LoadLogs", false); // Time-saver
      ld.setPropertyValue("CompressTolerance", "0.05");
      ld.setProperty("NumberOfBins", 1);
      ld.execute();
      TS_ASSERT(ld.isExecuted());
    }
    // get a reference to the uncompressed workspace
    EventWorkspace_sptr ws_compressed;
    TS_ASSERT_THROWS_NOTHING(ws_compressed =
                                 AnalysisDataService::Instance().retrieveWS<EventWorkspace>(compressed_name));
    TS_ASSERT(ws_compressed); // it is an EventWorkspace

    // validate the compressed workspace makes sense compared to uncompressed
    validateUncompressedCompressed(ws_uncompressed, ws_compressed, NUM_HIST, EventType::WEIGHTED);

    // cleanup
    AnalysisDataService::Instance().remove(uncompressed_name);
    AnalysisDataService::Instance().remove(compressed_name);
  }

  void test_Load_And_CompressEvents_with_nperiod_data() {
    constexpr std::size_t NUM_HIST{40960};
    const std::string filename{"LARMOR00003368.nxs"};

    Mantid::API::FrameworkManager::Instance();

    // create uncompressed - first so turning off compression isn't needed
    std::string uncompressed_name = "larmor_uncompressed";
    {
      LoadEventNexus ld;
      ld.initialize();
      ld.setPropertyValue("Filename", filename);
      ld.setPropertyValue("OutputWorkspace", uncompressed_name);
      ld.setProperty<bool>("Precount", false);
      ld.setProperty("NumberOfBins", 1);
      ld.execute();
      TS_ASSERT(ld.isExecuted());
    }
    // get a reference to the uncompressed workspace, first workspace only
    EventWorkspace_sptr ws_uncompressed;
    TS_ASSERT_THROWS_NOTHING(ws_uncompressed =
                                 AnalysisDataService::Instance().retrieveWS<EventWorkspace>(uncompressed_name + "_1"));
    TS_ASSERT(ws_uncompressed); // it is an EventWorkspace

    // create compressed
    std::string compressed_name = "larmor_compressed";
    {
      LoadEventNexus ld;
      ld.initialize();
      ld.setPropertyValue("Filename", filename);
      ld.setPropertyValue("OutputWorkspace", compressed_name);
      ld.setProperty<bool>("Precount", false);
      ld.setPropertyValue("CompressTolerance", "0.05");
      ld.setProperty("NumberOfBins", 1);
      ld.execute();
      TS_ASSERT(ld.isExecuted());
    }
    // get a reference to the uncompressed workspace, first workspace only
    EventWorkspace_sptr ws_compressed;
    TS_ASSERT_THROWS_NOTHING(ws_compressed =
                                 AnalysisDataService::Instance().retrieveWS<EventWorkspace>(compressed_name + "_1"));
    TS_ASSERT(ws_compressed); // it is an EventWorkspace

    // validate the compressed workspace makes sense compared to uncompressed
    validateUncompressedCompressed(ws_uncompressed, ws_compressed, NUM_HIST);

    // cleanup
    AnalysisDataService::Instance().remove(uncompressed_name);
    AnalysisDataService::Instance().remove(compressed_name);
  }

  void test_Load_And_CompressEvents_tolerance_0() {
    // the is to verify that the compresssion works when the CompressTolerance=0
    // create compressed
    const std::string filename{"CNCS_7860_event.nxs"};

    std::string compressed_name = "cncs_compressed0";
    {
      LoadEventNexus ld;
      ld.initialize();
      ld.setPropertyValue("Filename", filename);
      ld.setPropertyValue("OutputWorkspace", compressed_name);
      ld.setProperty<bool>("Precount", false);
      ld.setProperty<bool>("LoadLogs", false); // Time-saver
      ld.setPropertyValue("CompressTolerance", "0");
      ld.setProperty("NumberOfBins", 1);
      ld.execute();
      TS_ASSERT(ld.isExecuted());
    }
    // get a reference to the compressed workspace
    EventWorkspace_sptr ws_compressed;
    TS_ASSERT_THROWS_NOTHING(ws_compressed =
                                 AnalysisDataService::Instance().retrieveWS<EventWorkspace>(compressed_name));
    TS_ASSERT(ws_compressed); // it is an EventWorkspace

    /// CNCS_7860_event.nxs has 112266 so we expect slightly fewer events when compressed
    TS_ASSERT_EQUALS(ws_compressed->getNumberEvents(), 111274)
    // cleanup
    AnalysisDataService::Instance().remove(compressed_name);
  }

  void test_Load_And_FilterBadPulses() {
    // This will use ProcessBankData
    const std::string filename{"CNCS_7860_event.nxs"};

    Mantid::API::FrameworkManager::Instance();

    // create expected output workspace by doing FilterBadPulses after loading
    std::string post_filtered_name = "cncs_post_filtered";
    {
      LoadEventNexus ld;
      ld.initialize();
      ld.setPropertyValue("Filename", filename);
      ld.setPropertyValue("OutputWorkspace", post_filtered_name);
      ld.setProperty("NumberOfBins", 1);
      ld.execute();
      TS_ASSERT(ld.isExecuted());

      auto filter_bad = AlgorithmManager::Instance().create("FilterBadPulses", 1);
      filter_bad->setPropertyValue("InputWorkspace", post_filtered_name);
      filter_bad->setPropertyValue("OutputWorkspace", post_filtered_name);
      filter_bad->execute();
      TS_ASSERT(filter_bad->isExecuted());
    }

    // create filtered during load
    std::string filtered_name = "cncs_filtered";
    {
      LoadEventNexus ld;
      ld.initialize();
      ld.setPropertyValue("Filename", filename);
      ld.setPropertyValue("OutputWorkspace", filtered_name);
      ld.setProperty("NumberOfBins", 1);
      ld.setProperty("FilterBadPulsesLowerCutoff", 95.);
      ld.execute();
      TS_ASSERT(ld.isExecuted());
    }

    // validate the resulting workspace is the same for post filtered and filtering during loading
    auto checkAlg = AlgorithmManager::Instance().create("CompareWorkspaces");
    checkAlg->setProperty("Workspace1", filtered_name);
    checkAlg->setProperty("Workspace2", post_filtered_name);
    checkAlg->setProperty("CheckSample", true); // this will check that the logs get filtered correctly
    checkAlg->execute();
    TS_ASSERT(checkAlg->getProperty("Result"));

    // cleanup
    AnalysisDataService::Instance().remove(post_filtered_name);
    AnalysisDataService::Instance().remove(filtered_name);
  }

  void test_Load_And_FilterBadPulses_with_start_time_filter() {
    // This will use ProcessBankData
    // make sure the combination of bad pulse filter and start time filter work together
    const std::string filename{"CNCS_7860_event.nxs"};

    Mantid::API::FrameworkManager::Instance();

    // create expected output workspace by doing FilterBadPulses after loading
    std::string post_filtered_name = "cncs_post_filtered";
    {
      LoadEventNexus ld;
      ld.initialize();
      ld.setPropertyValue("Filename", filename);
      ld.setPropertyValue("OutputWorkspace", post_filtered_name);
      ld.setProperty("NumberOfBins", 1);
      ld.setProperty("FilterByTimeStart", 20.);
      ld.setProperty("FilterByTimeStop", 50.);
      ld.execute();
      TS_ASSERT(ld.isExecuted());

      auto filter_bad = AlgorithmManager::Instance().create("FilterBadPulses", 1);
      filter_bad->setPropertyValue("InputWorkspace", post_filtered_name);
      filter_bad->setPropertyValue("OutputWorkspace", post_filtered_name);
      filter_bad->execute();
      TS_ASSERT(filter_bad->isExecuted());
    }

    // create filtered during load
    std::string filtered_name = "cncs_filtered";
    {
      LoadEventNexus ld;
      ld.initialize();
      ld.setPropertyValue("Filename", filename);
      ld.setPropertyValue("OutputWorkspace", filtered_name);
      ld.setProperty("NumberOfBins", 1);
      ld.setProperty("FilterByTimeStart", 20.);
      ld.setProperty("FilterByTimeStop", 50.);
      ld.setProperty("FilterBadPulsesLowerCutoff", 95.);
      ld.execute();
      TS_ASSERT(ld.isExecuted());
    }

    // validate the resulting workspace is the same for post filtered and filtering during loading
    auto checkAlg = AlgorithmManager::Instance().create("CompareWorkspaces");
    checkAlg->setProperty("Workspace1", filtered_name);
    checkAlg->setProperty("Workspace2", post_filtered_name);
    checkAlg->setProperty("CheckSample", true); // this will check that the logs get filtered correctly
    checkAlg->execute();
    TS_ASSERT(checkAlg->getProperty("Result"));

    // cleanup
    AnalysisDataService::Instance().remove(post_filtered_name);
    AnalysisDataService::Instance().remove(filtered_name);
  }

  void test_Load_And_FilterBadPulses_and_compress() {
    // This will use ProcessBankCompressed
    const std::string filename{"CNCS_7860_event.nxs"};

    Mantid::API::FrameworkManager::Instance();

    // create expected output workspace by doing FilterBadPulses and CompressEvents after loading
    std::string post_filtered_name = "cncs_post_filtered_compressed";
    {
      LoadEventNexus ld;
      ld.initialize();
      ld.setPropertyValue("Filename", filename);
      ld.setPropertyValue("OutputWorkspace", post_filtered_name);
      ld.setProperty("NumberOfBins", 1);
      ld.execute();
      TS_ASSERT(ld.isExecuted());

      auto filter_bad = AlgorithmManager::Instance().create("FilterBadPulses", 1);
      filter_bad->setPropertyValue("InputWorkspace", post_filtered_name);
      filter_bad->setPropertyValue("OutputWorkspace", post_filtered_name);
      filter_bad->execute();
      TS_ASSERT(filter_bad->isExecuted());

      auto compress = AlgorithmManager::Instance().create("CompressEvents", 1);
      compress->setPropertyValue("InputWorkspace", post_filtered_name);
      compress->setPropertyValue("OutputWorkspace", post_filtered_name);
      compress->setProperty("Tolerance", 0.05);
      compress->execute();
      TS_ASSERT(compress->isExecuted());
    }

    // create filtered and compressed during load
    std::string filtered_name = "cncs_filtered_compressed";
    {
      LoadEventNexus ld;
      ld.initialize();
      ld.setPropertyValue("Filename", filename);
      ld.setPropertyValue("OutputWorkspace", filtered_name);
      ld.setProperty("NumberOfBins", 1);
      ld.setProperty("FilterBadPulsesLowerCutoff", 95.);
      ld.setProperty("CompressTolerance", 0.05);
      ld.execute();
      TS_ASSERT(ld.isExecuted());

      // need to sort events so we can directly compare to expected with CompareWorkspaces
      auto sort = AlgorithmManager::Instance().create("SortEvents", 1);
      sort->setPropertyValue("InputWorkspace", filtered_name);
      sort->execute();
      TS_ASSERT(sort->isExecuted());
    }

    // validate the resulting workspace is the same for post filtered/compressed and filtering/compressed during loading
    auto checkAlg = AlgorithmManager::Instance().create("CompareWorkspaces");
    checkAlg->setProperty("Workspace1", filtered_name);
    checkAlg->setProperty("Workspace2", post_filtered_name);
    checkAlg->setProperty("CheckSample", true); // this will check that the logs get filtered correctly
    checkAlg->execute();
    TS_ASSERT(checkAlg->getProperty("Result"));

    // cleanup
    AnalysisDataService::Instance().remove(post_filtered_name);
    AnalysisDataService::Instance().remove(filtered_name);
  }

  void test_Load_And_FilterBadPulses_and_compress_and_start_time_filter() {
    // This will use ProcessBankCompressed
    const std::string filename{"CNCS_7860_event.nxs"};

    Mantid::API::FrameworkManager::Instance();

    // create expected output workspace by doing FilterBadPulses and CompressEvents after loading
    std::string post_filtered_name = "cncs_post_filtered_compressed";
    {
      LoadEventNexus ld;
      ld.initialize();
      ld.setPropertyValue("Filename", filename);
      ld.setPropertyValue("OutputWorkspace", post_filtered_name);
      ld.setProperty("NumberOfBins", 1);
      ld.setProperty("FilterByTimeStart", 10.);
      ld.execute();
      TS_ASSERT(ld.isExecuted());

      auto filter_bad = AlgorithmManager::Instance().create("FilterBadPulses", 1);
      filter_bad->setPropertyValue("InputWorkspace", post_filtered_name);
      filter_bad->setPropertyValue("OutputWorkspace", post_filtered_name);
      filter_bad->execute();
      TS_ASSERT(filter_bad->isExecuted());
      auto compress = AlgorithmManager::Instance().create("CompressEvents", 1);
      compress->setPropertyValue("InputWorkspace", post_filtered_name);
      compress->setPropertyValue("OutputWorkspace", post_filtered_name);
      compress->setProperty("Tolerance", 0.05);
      compress->execute();
      TS_ASSERT(compress->isExecuted());
    }

    // create filtered during load
    std::string filtered_name = "cncs_filtered_compressed";
    {
      LoadEventNexus ld;
      ld.initialize();
      ld.setPropertyValue("Filename", filename);
      ld.setPropertyValue("OutputWorkspace", filtered_name);
      ld.setProperty("NumberOfBins", 1);
      ld.setProperty("FilterBadPulsesLowerCutoff", 95.);
      ld.setProperty("CompressTolerance", 0.05);
      ld.setProperty("FilterByTimeStart", 10.);
      ld.execute();
      TS_ASSERT(ld.isExecuted());

      // need to sort events so we can directly compare to expected with CompareWorkspaces
      auto sort = AlgorithmManager::Instance().create("SortEvents", 1);
      sort->setPropertyValue("InputWorkspace", filtered_name);
      sort->execute();
      TS_ASSERT(sort->isExecuted());
    }

    // validate the resulting workspace is the same for post filtered/compressed and filtering/compressed during loading
    auto checkAlg = AlgorithmManager::Instance().create("CompareWorkspaces");
    checkAlg->setProperty("Workspace1", filtered_name);
    checkAlg->setProperty("Workspace2", post_filtered_name);
    checkAlg->setProperty("CheckSample", true); // this will check that the logs get filtered correctly
    checkAlg->execute();
    TS_ASSERT(checkAlg->getProperty("Result"));

    // cleanup
    AnalysisDataService::Instance().remove(post_filtered_name);
    AnalysisDataService::Instance().remove(filtered_name);
  }

  void doTestSingleBank(bool SingleBankPixelsOnly, bool Precount, const std::string &BankName = "bank36",
                        bool willFail = false) {
    Mantid::API::FrameworkManager::Instance();
    LoadEventNexus ld;
    std::string outws_name = "cncs";
    AnalysisDataService::Instance().remove(outws_name);
    ld.initialize();
    ld.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    ld.setPropertyValue("OutputWorkspace", outws_name);
    ld.setPropertyValue("BankName", BankName);
    ld.setProperty<bool>("SingleBankPixelsOnly", SingleBankPixelsOnly);
    ld.setProperty<bool>("Precount", Precount);
    ld.setProperty<bool>("LoadLogs", false); // Time-saver
    ld.execute();

    EventWorkspace_sptr WS;
    if (willFail) {
      TS_ASSERT(!ld.isExecuted());
      return;
    }

    TS_ASSERT(ld.isExecuted());
    TS_ASSERT_THROWS_NOTHING(WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws_name));
    // Valid WS and it is an EventWorkspace
    TS_ASSERT(WS);

    // Pixels have to be padded
    TS_ASSERT_EQUALS(WS->getNumberHistograms(), SingleBankPixelsOnly ? 1024 : 51200);
    // Events - there are fewer now.
    TS_ASSERT_EQUALS(WS->getNumberEvents(), 7274);
  }

  void test_SingleBank_AllPixels() { doTestSingleBank(false, false); }

  void test_SingleBank_AllPixels_Precount() { doTestSingleBank(false, true); }

  void test_SingleBank_PixelsOnlyInThatBank_Precount() { doTestSingleBank(true, true); }

  void test_SingleBank_ThatDoesntExist() { doTestSingleBank(false, false, "bankDoesNotExist", true); }

  void test_SingleBank_with_no_events() {
    LoadEventNexus load;
    TS_ASSERT_THROWS_NOTHING(load.initialize());
    TS_ASSERT_THROWS_NOTHING(load.setPropertyValue("Filename", "HYSA_12509.nxs.h5"));
    TS_ASSERT_THROWS_NOTHING(load.setPropertyValue("BankName", "bank10"));
    const std::string outws("AnEmptyWS");
    TS_ASSERT_THROWS_NOTHING(load.setPropertyValue("OutputWorkspace", outws));
    if (!load.execute()) {
      TS_FAIL("LoadEventNexus shouldn't fail to load an empty bank");
      return;
    }

    auto ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws);
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 0);
  }

  void test_instrument_inside_nexus_file() {
    LoadEventNexus load;
    TS_ASSERT_THROWS_NOTHING(load.initialize());
    TS_ASSERT_THROWS_NOTHING(load.setPropertyValue("Filename", "HYSA_12509.nxs.h5"));
    const std::string outws("InstInNexus");
    TS_ASSERT_THROWS_NOTHING(load.setPropertyValue("OutputWorkspace", outws));
    TS_ASSERT(load.execute());

    auto ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws);
    auto inst = ws->getInstrument();
    TS_ASSERT_EQUALS(inst->getName(), "HYSPECA");
    TS_ASSERT_EQUALS(inst->getValidFromDate(), std::string("2011-Jul-20 17:02:48.437294000"));
    TS_ASSERT_EQUALS(inst->getNumberDetectors(), 20483);
    TS_ASSERT_EQUALS(inst->baseInstrument()->getMonitors().size(), 3);
    auto params = inst->getParameterMap();
    // Previously this was 49. Position/rotations are now stored in
    // ComponentInfo and DetectorInfo so the following four parameters are no
    // longer in the map:
    // HYSPECA/Tank;double;rotz;0
    // HYSPECA/Tank;double;rotx;0
    // HYSPECA/Tank;Quat;rot;[1,0,0,0]
    // HYSPECA/Tank;V3D;pos;[0,0,0]
    TS_ASSERT_EQUALS(params->size(), 45);
    std::cout << params->asString();

    TS_ASSERT_EQUALS(params->getString(inst.get(), "deltaE-mode"), "direct");
  }

  void test_instrument_and_default_param_loaded_when_inst_not_in_nexus_file() {
    LoadEventNexus load;
    TS_ASSERT_THROWS_NOTHING(load.initialize());
    TS_ASSERT_THROWS_NOTHING(load.setPropertyValue("Filename", "CNCS_7860_event.nxs"));
    load.setProperty<bool>("LoadLogs", false); // Time-saver
    const std::string outws("InstNotInNexus");
    TS_ASSERT_THROWS_NOTHING(load.setPropertyValue("OutputWorkspace", outws));
    TS_ASSERT(load.execute());

    auto ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws);
    auto inst = ws->getInstrument();
    TS_ASSERT(!inst->getFilename().empty()); // This is how we know we didn't
                                             // get it from inside the nexus
                                             // file
    TS_ASSERT_EQUALS(inst->getName(), "CNCS");
    TS_ASSERT_EQUALS(inst->getNumberDetectors(), 51203);
    TS_ASSERT_EQUALS(inst->baseInstrument()->getMonitors().size(), 3);

    // check that CNCS_Parameters.xml has been loaded
    auto params = inst->getParameterMap();
    TS_ASSERT_EQUALS(params->getString(inst.get(), "deltaE-mode"), "direct");
  }

  /** Test with a particular ARCS file that has 2 preprocessors,
   * meaning different-sized pulse ID files.
   * DISABLED AS THE FILE ISN'T IN THE REPOSITORY
   */
  void xtest_MultiplePreprocessors() {
    Mantid::API::FrameworkManager::Instance();
    LoadEventNexus ld;
    std::string outws_name = "arcs";
    ld.initialize();
    try {
      ld.setPropertyValue("Filename", "ARCS_12954_event.nxs");
    } catch (...) {
      std::cout << "Skipping test since file does not exist.";
      return;
    }
    ld.setPropertyValue("OutputWorkspace", outws_name);
    ld.setPropertyValue("CompressTolerance", "-1");
    ld.execute();
    TS_ASSERT(ld.isExecuted());

    EventWorkspace_sptr WS;
    TS_ASSERT_THROWS_NOTHING(WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws_name));
    // Valid WS and it is an EventWorkspace
    TS_ASSERT(WS);

    TS_ASSERT_EQUALS(WS->getNumberHistograms(), 117760);
    TS_ASSERT_EQUALS(WS->getNumberEvents(), 10730347);
    for (size_t wi = 0; wi < WS->getNumberHistograms(); wi++) {
      // Times are NON-zero for ALL pixels.
      if (WS->getSpectrum(wi).getNumberEvents() > 0) {
        int64_t nanosec = WS->getSpectrum(wi).getEvents()[0].pulseTime().totalNanoseconds();
        TS_ASSERT_DIFFERS(nanosec, 0)
        if (nanosec == 0) {
          std::cout << "Failure at WI " << wi << '\n';
          return;
        }
      }
    }
  }

  void test_start_and_end_time_filtered_loading_meta_data_only() {
    const bool metadataonly = true;
    do_test_filtering_start_and_end_filtered_loading(metadataonly);
  }

  void test_start_and_end_time_filtered_loading() {
    const bool metadataonly = false;
    do_test_filtering_start_and_end_filtered_loading(metadataonly);
  }

  void testSimulatedFile() {
    Mantid::API::FrameworkManager::Instance();
    LoadEventNexus ld;
    std::string wsname = "ARCS_sim";
    ld.initialize();
    ld.setPropertyValue("Filename", "ARCS_sim_event.nxs");
    ld.setPropertyValue("OutputWorkspace", wsname);
    ld.setProperty("BankName", "bank27");
    ld.setProperty("SingleBankPixelsOnly", false);
    ld.setProperty("LoadLogs", false);
    TS_ASSERT(ld.execute());

    EventWorkspace_sptr WS;
    TS_ASSERT_THROWS_NOTHING(WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsname));
    // Valid WS and it is an EventWorkspace
    TS_ASSERT(WS);

    const auto numHist = WS->getNumberHistograms();
    TS_ASSERT_EQUALS(numHist, 117760);
    TS_ASSERT_EQUALS(WS->getNumberEvents(), 2);
    for (size_t wi = 0; wi < numHist; wi += 5000) {
      // All events should be weighted events for simulated data
      TS_ASSERT_EQUALS(WS->getSpectrum(wi).getEventType(), WEIGHTED);
    }
    // Check one event
    TS_ASSERT_DELTA(WS->getSpectrum(26798).getWeightedEvents()[0].weight(), 1.8124e-11, 1.0e-4);
    TS_ASSERT_EQUALS(WS->getSpectrum(26798).getWeightedEvents()[0].tof(), 1476.0);
  }

  void test_extract_nperiod_data() {
    LoadEventNexus loader;

    loader.setChild(true);
    loader.initialize();
    loader.setPropertyValue("OutputWorkspace", "dummy");
    loader.setPropertyValue("Filename", "LARMOR00003368.nxs");
    loader.execute();
    Workspace_sptr outWS = loader.getProperty("OutputWorkspace");
    WorkspaceGroup_sptr outGroup = std::dynamic_pointer_cast<WorkspaceGroup>(outWS);
    TSM_ASSERT("Invalid Output Workspace Type", outGroup);

    IEventWorkspace_sptr firstWS = std::dynamic_pointer_cast<IEventWorkspace>(outGroup->getItem(0));
    auto run = firstWS->run();
    const int nPeriods = run.getPropertyValueAsType<int>("nperiods");
    TSM_ASSERT_EQUALS("Wrong number of periods extracted", nPeriods, 4);
    TSM_ASSERT_EQUALS("Groups size should be same as nperiods", outGroup->size(), nPeriods);
    // mean of proton charge for each period
    std::array<double, 4> protonChargeMeans = {0.00110488, 0.00110392, 0.00110336, 0.00110404};
    for (size_t i = 0; i < outGroup->size(); ++i) {
      EventWorkspace_sptr ws = std::dynamic_pointer_cast<EventWorkspace>(outGroup->getItem(i));
      TS_ASSERT(ws);
      TSM_ASSERT("Non-zero events in each period", ws->getNumberEvents() > 0);

      std::stringstream buffer;
      buffer << "period " << i + 1;
      std::string periodBoolLog = buffer.str();

      const int currentPeriod = ws->run().getPropertyValueAsType<int>("current_period");

      TSM_ASSERT("Each period should have a boolean array for masking period numbers",
                 ws->run().hasProperty(periodBoolLog));
      TSM_ASSERT_EQUALS("Current period is not what was expected.", currentPeriod, i + 1);

      // Check we have correctly filtered sample logs based on the period
      auto protonLog = ws->run().getTimeSeriesProperty<double>("proton_charge");
      TS_ASSERT(protonLog->isFiltered());
      TS_ASSERT_DELTA(protonLog->mean(), protonChargeMeans[i], 1e-8);
    }
    // Make sure that the spectraNo are equal for all child workspaces.
    auto isFirstChildWorkspace = true;
    std::vector<Mantid::specnum_t> specids;

    for (size_t i = 0; i < outGroup->size(); ++i) {
      EventWorkspace_sptr ws = std::dynamic_pointer_cast<EventWorkspace>(outGroup->getItem(i));
      if (isFirstChildWorkspace) {
        specids.reserve(ws->getNumberHistograms());
      }
      for (size_t index = 0; index < ws->getNumberHistograms(); ++index) {
        if (isFirstChildWorkspace) {
          specids.emplace_back(ws->getSpectrum(index).getSpectrumNo());
        } else {
          TSM_ASSERT_EQUALS("The spectrNo should be the same for all child workspaces.", specids[index],
                            ws->getSpectrum(index).getSpectrumNo());
        }
      }

      isFirstChildWorkspace = false;
    }
  }

  void test_load_file_with_empty_periods() {
    // The file LARMOR00062766.nxs has the number of periods specified as 2, but actually
    // only one period has any data in it. It should load, but only one workspace instead
    // of two.
    // See https://github.com/mantidproject/mantid/issues/33729 for details
    LoadEventNexus loader;
    loader.initialize();
    loader.setPropertyValue("OutputWorkspace", "dummy");
    loader.setPropertyValue("Filename", "LARMOR00062766.nxs");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    auto ws = AnalysisDataService::Instance().retrieve("dummy");
    TS_ASSERT(!ws->isGroup());
  }

  void test_load_CG3_bad_event_id() {
    // The test file CG3_13118.nxs.h5 being loaded has:
    // bank1: all correct data, only events in this file should end up loaded (6052 events)
    // bank2: all event_id are out of range and should be ignored (91 events)
    // bank_error: all correct data but should be skipped because this bank is junk output (6052 events)
    // bank_unmapped: all junk data and shouldn't be loaded (91 events)

    LoadEventNexus load;
    TS_ASSERT_THROWS_NOTHING(load.initialize());
    TS_ASSERT_THROWS_NOTHING(load.setPropertyValue("Filename", "CG3_13118.nxs.h5"));
    const std::string outws("CG3_bad_id_test");
    TS_ASSERT_THROWS_NOTHING(load.setPropertyValue("OutputWorkspace", outws));
    TS_ASSERT(load.execute());

    auto ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws);

    // only events from bank1 should be loaded
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 6052);
  }

  void test_load_fails_on_corrupted_run() {
    // Some ISIS runs can be corrupted by instrument noise,
    // resulting in incorrect period numbers.
    // LoadEventNexus should fail in this case.
    LoadEventNexus loader;

    loader.setChild(true);
    loader.initialize();
    loader.setPropertyValue("OutputWorkspace", "dummy");
    loader.setPropertyValue("Filename", "SANS2D00059115_corrupted.nxs");
    TS_ASSERT_THROWS(loader.execute(), const InvalidLogPeriods &);
  }

  void test_load_ILL_no_triggers() {
    // ILL runs don't have any pulses, so in event mode, they are replaced in the event nexus by trigger signals.
    // But some of these nexuses don't have any triggers either, so they are modified to be allowed to be loaded.

    LoadEventNexus loader;

    loader.initialize();
    loader.setPropertyValue("Filename", "ILL/D22B/000242_trunc.event.nxs");
    loader.setPropertyValue("OutputWorkspace", "dummy");
    loader.setProperty("LoadAllLogs", true);
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    EventWorkspace_sptr eventWS;
    TS_ASSERT_THROWS_NOTHING(eventWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("dummy"));
    // Valid WS and it is an EventWorkspace
    TS_ASSERT(eventWS);

    TS_ASSERT_EQUALS(eventWS->getNumberEvents(), 1000);
    TS_ASSERT_EQUALS(eventWS->run().startTime(), DateAndTime("2021-01-28T18:07:12"));
    TS_ASSERT_EQUALS(eventWS->getPulseTimeMin(), eventWS->getPulseTimeMax());
    TS_ASSERT_EQUALS(eventWS->getPulseTimeMin().totalNanoseconds(), 980705232000000000);
    TS_ASSERT_DELTA(eventWS->getTofMax(), 13515.0517592763, 1e-2);

    // check that the logs have been loaded by looking at some random example
    TS_ASSERT_DELTA(eventWS->run().getPropertyAsSingleValue("reactor_power"), 43.21, 1e-2);

    AnalysisDataService::Instance().remove("dummy");
  }

  void test_load_ILL_triggers() {
    // ILL runs don't have any pulses, so in event mode, they are replaced in the event nexus by trigger signals.

    LoadEventNexus loader;

    loader.initialize();
    loader.setPropertyValue("Filename", "ILL/D22B/042730_trunc.event.nxs");
    loader.setPropertyValue("OutputWorkspace", "dummy");
    loader.setProperty("LoadAllLogs", true);
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    EventWorkspace_sptr eventWS;
    TS_ASSERT_THROWS_NOTHING(eventWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("dummy"));
    // Valid WS and it is an EventWorkspace
    TS_ASSERT(eventWS);

    TS_ASSERT_EQUALS(eventWS->getNumberEvents(), 1000);
    TS_ASSERT_EQUALS(eventWS->run().startTime(), DateAndTime("2021-03-24T20:52:50"));
    TS_ASSERT_EQUALS(eventWS->getPulseTimeMin().totalNanoseconds(), 985467170046478105);
    TS_ASSERT_EQUALS(eventWS->getPulseTimeMax().totalNanoseconds(), 985467770208320643);

    // check that the logs have been loaded by looking at some random example
    TS_ASSERT_DELTA(eventWS->run().getPropertyAsSingleValue("reactor_power"), 43.2, 1e-2);

    AnalysisDataService::Instance().remove("dummy");
  }

  void test_load_event_nexus_ISIS_exc_inst() {
    // Test new format ISIS event data files which have some instrument information
    // but does not follow Mantid's NexusGeometry specifications
    const std::string file = "MAR28482.nxs";
    LoadEventNexus alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("Filename", file);
    alg.setProperty("OutputWorkspace", "dummy_for_child");
    alg.execute();
    Workspace_sptr ws = alg.getProperty("OutputWorkspace");
    auto eventWS = std::dynamic_pointer_cast<EventWorkspace>(ws);
    TS_ASSERT(eventWS);
    TS_ASSERT_EQUALS(eventWS->getNumberEvents(), 203);
    TS_ASSERT_EQUALS(eventWS->detectorInfo().size(), 921)
  }

  void test_monotonically_increasing_tofs() {
    const std::string file = "CG2_monotonically_increasing_pulse_times.nxs.h5";
    const std::string wsName = "dummy_for_child";
    LoadEventNexus alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("Filename", file);
    alg.setProperty("OutputWorkspace", wsName);
    alg.setProperty("NumberOfBins", 1);
    alg.execute();
    Workspace_sptr ws = alg.getProperty("OutputWorkspace");
    auto eventWS = std::dynamic_pointer_cast<EventWorkspace>(ws);
    TS_ASSERT(eventWS);
    constexpr int expectedNumberEvents = 32494;
    TS_ASSERT_EQUALS(eventWS->getNumberEvents(), expectedNumberEvents);
    double sum = 0.0;
    for (size_t i = 0; i < eventWS->getNumberHistograms(); ++i) {
      TS_ASSERT_EQUALS(eventWS->readX(i).size(), 2)
      sum += eventWS->readY(i)[0];
    }
    TS_ASSERT_DELTA(sum, expectedNumberEvents, 1e-6)
    AnalysisDataService::Instance().remove(wsName);
  }

  void test_no_events() {
    // this test was created for the strange case of an event file having no events anywhere
    // originally it was only an empty monitor but the test file was expanded

    const std::string filename = "CG3_22446_empty.nxs.h5";
    const std::string wsname = "CG3_empty";

    // run the algorithm
    LoadEventNexus loader;
    loader.initialize();
    loader.setPropertyValue("Filename", filename);
    loader.setPropertyValue("OutputWorkspace", wsname);
    //    loader.setProperty("LoadAllLogs", false);
    loader.setProperty("LoadMonitors", true);
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    auto &ads = AnalysisDataService::Instance();

    // validate the event workspace
    {
      MatrixWorkspace_sptr wksp = ads.retrieveWS<MatrixWorkspace>(wsname);
      TS_ASSERT(wksp);
      auto event_wksp = std::dynamic_pointer_cast<EventWorkspace>(wksp);
      TS_ASSERT(event_wksp);
      TS_ASSERT_EQUALS(event_wksp->getNumberEvents(), 0.);
    }

    // validate the monitor workspace
    {
      MatrixWorkspace_sptr wksp_mon = ads.retrieveWS<MatrixWorkspace>(wsname + "_monitors");
      TS_ASSERT(wksp_mon);
      auto event_wksp = std::dynamic_pointer_cast<EventWorkspace>(wksp_mon);
      TS_ASSERT(event_wksp);
      TS_ASSERT_EQUALS(event_wksp->getNumberEvents(), 0.);
    }

    // cleanup
    AnalysisDataService::Instance().remove(wsname);
  }

private:
  std::string wsSpecFilterAndEventMonitors;
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadEventNexusTestPerformance : public CxxTest::TestSuite {
public:
#ifdef _WIN32
  bool windows = true;
#else
  bool windows = false;
#endif // _WIN32
  void testMultiprocessLoadPrecount() {
    if (!windows) {
      LoadEventNexus loader;
      loader.initialize();
      loader.setPropertyValue("Filename", "SANS2D00022048.nxs");
      loader.setPropertyValue("OutputWorkspace", "ws");
      loader.setPropertyValue("Loadtype", "Multiprocess (experimental)");
      loader.setPropertyValue("Precount", std::to_string(true));
      TS_ASSERT(loader.execute());
    }
  }
  void testMultiprocessLoadProducerConsumer() {
    if (!windows) {
      LoadEventNexus loader;
      loader.initialize();
      loader.setPropertyValue("Filename", "SANS2D00022048.nxs");
      loader.setPropertyValue("OutputWorkspace", "ws");
      loader.setPropertyValue("Loadtype", "Multiprocess (experimental)");
      loader.setPropertyValue("Precount", std::to_string(false));
      TS_ASSERT(loader.execute());
    }
  }
  void testDefaultLoad() {
    LoadEventNexus loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    loader.setPropertyValue("OutputWorkspace", "ws");
    TS_ASSERT(loader.execute());
  }
  void testDefaultLoadBankSplitting() {
    LoadEventNexus loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "OFFSPEC00036416.nxs");
    loader.setPropertyValue("OutputWorkspace", "ws");
    TS_ASSERT(loader.execute());
  }
  void testPartialLoad() {
    LoadEventNexus loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    loader.setProperty("SpectrumMin", 10);
    loader.setProperty("SpectrumMax", 20);
    loader.setPropertyValue("OutputWorkspace", "ws");
    TS_ASSERT(loader.execute());
  }
  void testPartialLoadBankSplitting() {
    LoadEventNexus loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "OFFSPEC00036416.nxs");
    loader.setProperty("SpectrumMin", 10);
    loader.setProperty("SpectrumMax", 20);
    loader.setPropertyValue("OutputWorkspace", "ws");
    TS_ASSERT(loader.execute());
  }
};
