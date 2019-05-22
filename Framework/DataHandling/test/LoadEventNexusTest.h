// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef LOADEVENTNEXUSTEST_H_
#define LOADEVENTNEXUSTEST_H_

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
#include "MantidParallel/Collectives.h"
#include "MantidParallel/Communicator.h"
#include "MantidTestHelpers/ParallelAlgorithmCreation.h"
#include "MantidTestHelpers/ParallelRunner.h"

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

  EventWorkspace_sptr ws =
      AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws_name);
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

  EventWorkspace_sptr wsRef =
      AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws_name);
  TS_ASSERT(wsRef);

  TSM_ASSERT_EQUALS("Different spectrum number in reference ws.",
                    wsRef->getNumberHistograms(), ws->getNumberHistograms());
  if (wsRef->getNumberHistograms() != ws->getNumberHistograms())
    return;
  for (size_t i = 0; i < wsRef->getNumberHistograms(); ++i) {
    auto &eventList = ws->getSpectrum(i).getEvents();
    auto &eventListRef = wsRef->getSpectrum(i).getEvents();
    TSM_ASSERT_EQUALS("Different events number in reference spectra",
                      eventList.size(), eventListRef.size());
    if (eventList.size() != eventListRef.size())
      return;
    for (size_t j = 0; j < eventListRef.size(); ++j) {
      TSM_ASSERT_EQUALS("Events are not equal", eventList[j].tof(),
                        eventListRef[j].tof());
      TSM_ASSERT_EQUALS("Events are not equal", eventList[j].pulseTime(),
                        eventListRef[j].pulseTime());
    }
  }
}

namespace {
boost::shared_ptr<const EventWorkspace>
load_reference_workspace(const std::string &filename) {
  // Construct default communicator *without* threading backend. In non-MPI run
  // (such as when running unit tests) this will thus just be a communicator
  // containing a single rank, independently on all ranks, which is what we want
  // for default loading bhavior.
  Parallel::Communicator comm;
  auto alg = ParallelTestHelpers::create<LoadEventNexus>(comm);
  alg->setProperty("Filename", filename);
  alg->setProperty("LoadLogs", false);
  TS_ASSERT_THROWS_NOTHING(alg->execute());
  TS_ASSERT(alg->isExecuted());
  Workspace_const_sptr out = alg->getProperty("OutputWorkspace");
  return boost::dynamic_pointer_cast<const EventWorkspace>(out);
}
void run_MPI_load(const Parallel::Communicator &comm,
                  boost::shared_ptr<std::mutex> mutex,
                  const std::string &filename) {
  boost::shared_ptr<const EventWorkspace> reference;
  boost::shared_ptr<const EventWorkspace> eventWS;
  {
    std::lock_guard<std::mutex> lock(*mutex);
    reference = load_reference_workspace(filename);
    auto alg = ParallelTestHelpers::create<LoadEventNexus>(comm);
    alg->setProperty("Filename", filename);
    alg->setProperty("LoadLogs", false);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    Workspace_const_sptr out = alg->getProperty("OutputWorkspace");
    if (comm.size() != 1) {
      TS_ASSERT_EQUALS(out->storageMode(), Parallel::StorageMode::Distributed);
    }
    eventWS = boost::dynamic_pointer_cast<const EventWorkspace>(out);
  }
  const size_t localSize = eventWS->getNumberHistograms();
  auto localEventCount = eventWS->getNumberEvents();
  std::vector<size_t> localSizes;
  std::vector<size_t> localEventCounts;
  Parallel::gather(comm, localSize, localSizes, 0);
  Parallel::gather(comm, localEventCount, localEventCounts, 0);
  if (comm.rank() == 0) {
    TS_ASSERT_EQUALS(std::accumulate(localSizes.begin(), localSizes.end(),
                                     static_cast<size_t>(0)),
                     reference->getNumberHistograms());
    TS_ASSERT_EQUALS(std::accumulate(localEventCounts.begin(),
                                     localEventCounts.end(),
                                     static_cast<size_t>(0)),
                     reference->getNumberEvents());
  }

  const auto &indexInfo = eventWS->indexInfo();
  size_t localCompared = 0;
  for (size_t i = 0; i < reference->getNumberHistograms(); ++i) {
    for (const auto &index :
         indexInfo.makeIndexSet({static_cast<Indexing::SpectrumNumber>(
             reference->getSpectrum(i).getSpectrumNo())})) {
      TS_ASSERT_EQUALS(eventWS->getSpectrum(index), reference->getSpectrum(i));
      ++localCompared;
    }
  }
  // Consistency check: Make sure we really compared all spectra (protects
  // against missing spectrum numbers or inconsistent mapping in IndexInfo).
  std::vector<size_t> compared;
  Parallel::gather(comm, localCompared, compared, 0);
  if (comm.rank() == 0) {
    TS_ASSERT_EQUALS(std::accumulate(compared.begin(), compared.end(),
                                     static_cast<size_t>(0)),
                     reference->getNumberHistograms());
  }
}
} // namespace

class LoadEventNexusTest : public CxxTest::TestSuite {
private:
  void
  do_test_filtering_start_and_end_filtered_loading(const bool metadataonly) {
    const std::string wsName = "test_filtering";
    const double filterStart = 1;
    const double filterEnd = 1000;

    LoadEventNexus ld;
    ld.initialize();
    ld.setPropertyValue("OutputWorkspace", wsName);
    ld.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    ld.setProperty("FilterByTimeStart", filterStart);
    ld.setProperty("FilterByTimeStop", filterEnd);
    ld.setProperty("MetaDataOnly", metadataonly);

    TS_ASSERT(ld.execute());

    auto outWs =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName);

    Property *prop = outWs->run().getLogData("SampleTemp");
    TSM_ASSERT_EQUALS("Should have 16 elements after filtering.", 16,
                      prop->size());
    if (prop->size() != 16)
      return;
    // Further tests
    TimeSeriesProperty<double> *sampleTemps =
        dynamic_cast<TimeSeriesProperty<double> *>(prop);
    auto filteredLogStartTime = sampleTemps->nthTime(0);
    auto filteredLogEndTime = sampleTemps->nthTime(sampleTemps->size() - 1);
    TS_ASSERT_EQUALS("2010-Mar-25 16:09:27.620000000",
                     filteredLogStartTime.toSimpleString());
    TS_ASSERT_EQUALS("2010-Mar-25 16:11:51.558003540",
                     filteredLogEndTime.toSimpleString());
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
    auto eventWS = boost::dynamic_pointer_cast<EventWorkspace>(ws);
    TS_ASSERT(eventWS);

    TS_ASSERT_EQUALS(eventWS->getNumberEvents(), 1439);
    TS_ASSERT_EQUALS(eventWS->detectorInfo().size(),
                     (150 * 150) + 2) // Two monitors
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
      auto eventWS = boost::dynamic_pointer_cast<EventWorkspace>(ws);
      TS_ASSERT(eventWS);

      TS_ASSERT_EQUALS(eventWS->getNumberEvents(), 43277);
      TS_ASSERT_EQUALS(eventWS->detectorInfo().size(),
                       (300 * 300) + 2) // Two monitors
      TS_ASSERT_DELTA(eventWS->getTofMin(), 9.815, 1.0e-3);
      TS_ASSERT_DELTA(eventWS->getTofMax(), 130748.563, 1.0e-3);
    }
  }

  void test_load_event_nexus_sans2d_ess() {
    const std::string file = "SANS2D_ESS_example.nxs";
    LoadEventNexus alg;
    alg.setChild(true);
    alg.setRethrows(true);
    alg.initialize();
    alg.setProperty("Filename", file);
    alg.setProperty("OutputWorkspace", "dummy_for_child");
    alg.execute();
    Workspace_sptr ws = alg.getProperty("OutputWorkspace");
    auto eventWS = boost::dynamic_pointer_cast<EventWorkspace>(ws);
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
    auto eventWS = boost::dynamic_pointer_cast<EventWorkspace>(ws);
    TS_ASSERT(eventWS);
    const double duration =
        eventWS->mutableRun().getPropertyValueAsType<double>("duration");
    TS_ASSERT_DELTA(duration, 7200.012, 0.01);
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
    ld.setProperty<bool>("LoadLogs", false); // Time-saver
    ld.execute();
    TS_ASSERT(ld.isExecuted());

    EventWorkspace_sptr WS =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws_name);
    // Valid WS and it is an EventWorkspace
    TS_ASSERT(WS);
    // Pixels have to be padded
    TS_ASSERT_EQUALS(WS->getNumberHistograms(), 51200);
    // Events
    TS_ASSERT_EQUALS(WS->getNumberEvents(), 112266);
    // TOF limits found. There is a pad of +-1 given around the actual TOF
    // founds.
    TS_ASSERT_DELTA((*WS->refX(0))[0], 44162.6, 0.05);
    TS_ASSERT_DELTA((*WS->refX(0))[1], 60830.2, 0.05);
    // Valid spectrum info
    TS_ASSERT_EQUALS(WS->getSpectrum(0).getSpectrumNo(), 1);
    TS_ASSERT_EQUALS(WS->getSpectrum(0).getDetectorIDs().size(), 1);
    TS_ASSERT_EQUALS(*WS->getSpectrum(0).getDetectorIDs().begin(), 0);

    // Check one event from one pixel - does it have a reasonable pulse time
    TS_ASSERT(WS->getSpectrum(1000).getEvents()[0].pulseTime() >
              DateAndTime(int64_t(1e9 * 365 * 10)));

    // Check filename
    TS_ASSERT_EQUALS(ld.getPropertyValue("Filename"),
                     WS->run().getProperty("Filename")->value());

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
    ld2.execute();
    TS_ASSERT(ld2.isExecuted());

    EventWorkspace_sptr WS2 =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws_name2);
    // Valid WS and it is an EventWorkspace
    TS_ASSERT(WS2);

    TS_ASSERT_EQUALS(WS->getNumberEvents(), WS2->getNumberEvents());
    // Memory used should be lower (or the same at worst)
    TS_ASSERT_LESS_THAN_EQUALS(WS2->getMemorySize(), WS->getMemorySize());

    // Longer, more thorough test
    if (false) {
      IAlgorithm_sptr load =
          AlgorithmManager::Instance().create("LoadEventPreNexus", 1);
      load->setPropertyValue("OutputWorkspace", "cncs_pre");
      load->setPropertyValue("EventFilename", "CNCS_7860_neutron_event.dat");
      load->setPropertyValue("PulseidFilename", "CNCS_7860_pulseid.dat");
      load->setPropertyValue("MappingFilename", "CNCS_TS_2008_08_18.dat");
      load->execute();
      TS_ASSERT(load->isExecuted());
      EventWorkspace_sptr WS2 =
          AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
              "cncs_pre");
      // Valid WS and it is an EventWorkspace
      TS_ASSERT(WS2);

      // Let's compare the proton_charge logs
      TimeSeriesProperty<double> *log =
          dynamic_cast<TimeSeriesProperty<double> *>(
              WS->mutableRun().getProperty("proton_charge"));
      std::map<DateAndTime, double> logMap = log->valueAsCorrectMap();
      TimeSeriesProperty<double> *log2 =
          dynamic_cast<TimeSeriesProperty<double> *>(
              WS2->mutableRun().getProperty("proton_charge"));
      std::map<DateAndTime, double> logMap2 = log2->valueAsCorrectMap();
      std::map<DateAndTime, double>::iterator it, it2;

      it = logMap.begin();
      it2 = logMap2.begin();
      for (; it != logMap.end();) {
        // Same times within a millisecond
        // TS_ASSERT_DELTA( it->first, it2->first,
        // DateAndTime::durationFromSeconds(1e-3));
        // Same times?
        TS_ASSERT_LESS_THAN(
            fabs(DateAndTime::secondsFromDuration(it->first - it2->first)),
            1); // TODO: Fix the nexus file times here
        // Same proton charge?
        TS_ASSERT_DELTA(it->second, it2->second, 1e-5);
        it++;
        it2++;
      }

      int pixelID = 2000;

      std::vector<TofEvent> events1 = WS->getSpectrum(pixelID).getEvents();
      std::vector<TofEvent> events2 = WS2->getSpectrum(pixelID).getEvents();

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

    auto outWs =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName);

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
    specList.push_back(13);
    specList.push_back(16);
    specList.push_back(21);
    specList.push_back(28);

    // A) test SpectrumList
    LoadEventNexus ld;
    ld.initialize();
    ld.setPropertyValue("OutputWorkspace", wsName);
    ld.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    ld.setProperty("SpectrumList", specList);
    ld.setProperty<bool>("LoadLogs", false); // Time-saver

    TS_ASSERT(ld.execute());

    auto outWs =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName);

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
      TS_ASSERT_EQUALS(outWs->getSpectrum(specIdx).getSpectrumNo(),
                       static_cast<int>(specMin + specIdx + 1));
    }

    // C) test SpectrumList + SpectrumMin and SpectrumMax
    // This will make: 17, 20, 21, 22, 23
    wsSpecFilterAndEventMonitors =
        "test_partial_spectra_loading_SpectrumList_SpectrumMin_SpectrumMax";
    const size_t sMin = 20;
    const size_t sMax = 22;
    specList.clear();
    specList.push_back(17);

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

    outWs = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
        wsSpecFilterAndEventMonitors);

    // check number and indices of spectra
    const size_t n = sMax - sMin + 1; // this n is the 20...22, excluding '17'
    TS_ASSERT_EQUALS(outWs->getNumberHistograms(), n + 1); // +1 is the '17'
    // Spectrum numbers match those that same detector would have in unfiltered
    // load, in this case detID + 1 since IDs in instrument start at 0.
    // 18 should come from SpectrumList
    TS_ASSERT_EQUALS(outWs->getSpectrum(0).getSpectrumNo(), 18);
    // and then sMin(20)...sMax(22)
    for (size_t specIdx = 0; specIdx < n; specIdx++) {
      TS_ASSERT_EQUALS(outWs->getSpectrum(specIdx + 1).getSpectrumNo(),
                       static_cast<int>(sMin + specIdx + 1));
    }
  }

  void test_partial_spectra_loading_ISIS() {
    // This is to test a specific bug where if you selected any spectra and had
    // precount on you got double the number of events
    std::string wsName = "test_partial_spectra_loading_SpectrumListISIS";
    std::string wsName2 = "test_partial_spectra_loading_SpectrumListISIS2";
    std::string filename = "OFFSPEC00036416.nxs";
    std::vector<int32_t> specList;
    specList.push_back(45);

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

    auto outWs =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName);
    auto outWs2 =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName2);

    TSM_ASSERT_EQUALS("The number of spectra in the workspace should be 12",
                      outWs->getNumberHistograms(), 12);

    TSM_ASSERT_EQUALS("The number of events in the precount and not precount "
                      "workspaces do not match",
                      outWs->getNumberEvents(), outWs2->getNumberEvents());

    TSM_ASSERT("Some spectra were not found in the workspace",
               outWs->getSpectrum(0).getSpectrumNo() == 10);

    TSM_ASSERT("Some spectra were not found in the workspace",
               outWs->getSpectrum(10).getSpectrumNo() == 20);
    TSM_ASSERT("Some spectra were not found in the workspace",
               outWs->getSpectrum(11).getSpectrumNo() == 45);

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
    const std::string mon_outws_name =
        wsSpecFilterAndEventMonitors + "_monitors";
    auto &ads = AnalysisDataService::Instance();

    // Valid workspace and it is an event workspace
    const auto monWS = ads.retrieveWS<MatrixWorkspace>(mon_outws_name);

    TS_ASSERT(monWS);
    TS_ASSERT_EQUALS(monWS->getTitle(), "test after manual intervention");

    // Check link data --> monitor workspaces
    TS_ASSERT_EQUALS(
        monWS, ads.retrieveWS<MatrixWorkspace>(wsSpecFilterAndEventMonitors)
                   ->monitorWorkspace());
  }

  void test_Load_And_CompressEvents() {
    Mantid::API::FrameworkManager::Instance();
    LoadEventNexus ld;
    std::string outws_name = "cncs_compressed";
    ld.initialize();
    ld.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    ld.setPropertyValue("OutputWorkspace", outws_name);
    ld.setPropertyValue("Precount", "0");
    ld.setPropertyValue("CompressTolerance", "0.05");
    ld.setProperty<bool>("LoadMonitors",
                         true);              // For the next test, saving a load
    ld.setProperty<bool>("LoadLogs", false); // Time-saver
    ld.execute();
    TS_ASSERT(ld.isExecuted());

    EventWorkspace_sptr WS;
    TS_ASSERT_THROWS_NOTHING(
        WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            outws_name));
    // Valid WS and it is an EventWorkspace
    TS_ASSERT(WS);
    // Pixels have to be padded
    TS_ASSERT_EQUALS(WS->getNumberHistograms(), 51200);
    // Events
    TS_ASSERT_EQUALS(WS->getNumberEvents(),
                     111274); // There are (slightly) fewer events
    for (size_t wi = 0; wi < WS->getNumberHistograms(); wi++) {
      // Pixels with at least one event will have switched
      if (WS->getSpectrum(wi).getNumberEvents() > 0)
        TS_ASSERT_EQUALS(WS->getSpectrum(wi).getEventType(), WEIGHTED_NOTIME)
    }
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
    TS_ASSERT_DELTA(specInfo.samplePosition().distance(specInfo.position(2)),
                    1.426, 1e-6);

    // Check monitor workspace pointer held in main workspace
    TS_ASSERT_EQUALS(
        WS,
        ads.retrieveWS<MatrixWorkspace>("cncs_compressed")->monitorWorkspace());
  }

  void doTestSingleBank(bool SingleBankPixelsOnly, bool Precount,
                        std::string BankName = "bank36",
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
    TS_ASSERT_THROWS_NOTHING(
        WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            outws_name));
    // Valid WS and it is an EventWorkspace
    TS_ASSERT(WS);
    if (!WS)
      return;
    // Pixels have to be padded
    TS_ASSERT_EQUALS(WS->getNumberHistograms(),
                     SingleBankPixelsOnly ? 1024 : 51200);
    // Events - there are fewer now.
    TS_ASSERT_EQUALS(WS->getNumberEvents(), 7274);
  }

  void test_SingleBank_AllPixels() { doTestSingleBank(false, false); }

  void test_SingleBank_AllPixels_Precount() { doTestSingleBank(false, true); }

  void test_SingleBank_PixelsOnlyInThatBank_Precount() {
    doTestSingleBank(true, true);
  }

  void test_SingleBank_ThatDoesntExist() {
    doTestSingleBank(false, false, "bankDoesNotExist", true);
  }

  void test_SingleBank_with_no_events() {
    LoadEventNexus load;
    TS_ASSERT_THROWS_NOTHING(load.initialize());
    TS_ASSERT_THROWS_NOTHING(
        load.setPropertyValue("Filename", "HYSA_12509.nxs.h5"));
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
    TS_ASSERT_THROWS_NOTHING(
        load.setPropertyValue("Filename", "HYSA_12509.nxs.h5"));
    const std::string outws("InstInNexus");
    TS_ASSERT_THROWS_NOTHING(load.setPropertyValue("OutputWorkspace", outws));
    TS_ASSERT(load.execute());

    auto ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws);
    auto inst = ws->getInstrument();
    TS_ASSERT_EQUALS(inst->getName(), "HYSPECA");
    TS_ASSERT_EQUALS(inst->getValidFromDate(),
                     std::string("2011-Jul-20 17:02:48.437294000"));
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
    TS_ASSERT_THROWS_NOTHING(
        load.setPropertyValue("Filename", "CNCS_7860_event.nxs"));
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
    TS_ASSERT_THROWS_NOTHING(
        WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            outws_name));
    // Valid WS and it is an EventWorkspace
    TS_ASSERT(WS);
    if (!WS)
      return;
    TS_ASSERT_EQUALS(WS->getNumberHistograms(), 117760);
    TS_ASSERT_EQUALS(WS->getNumberEvents(), 10730347);
    for (size_t wi = 0; wi < WS->getNumberHistograms(); wi++) {
      // Times are NON-zero for ALL pixels.
      if (WS->getSpectrum(wi).getNumberEvents() > 0) {
        int64_t nanosec =
            WS->getSpectrum(wi).getEvents()[0].pulseTime().totalNanoseconds();
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
    TS_ASSERT_THROWS_NOTHING(
        WS =
            AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsname));
    // Valid WS and it is an EventWorkspace
    TS_ASSERT(WS);
    if (!WS)
      return;
    const auto numHist = WS->getNumberHistograms();
    TS_ASSERT_EQUALS(numHist, 117760);
    TS_ASSERT_EQUALS(WS->getNumberEvents(), 2);
    for (size_t wi = 0; wi < numHist; wi += 5000) {
      // All events should be weighted events for simulated data
      TS_ASSERT_EQUALS(WS->getSpectrum(wi).getEventType(), WEIGHTED);
    }
    // Check one event
    TS_ASSERT_DELTA(WS->getSpectrum(26798).getWeightedEvents()[0].weight(),
                    1.8124e-11, 1.0e-4);
    TS_ASSERT_EQUALS(WS->getSpectrum(26798).getWeightedEvents()[0].tof(),
                     1476.0);
  }

  void test_extract_nperiod_data() {
    LoadEventNexus loader;

    loader.setChild(true);
    loader.initialize();
    loader.setPropertyValue("OutputWorkspace", "dummy");
    loader.setPropertyValue("Filename", "LARMOR00003368.nxs");
    loader.execute();
    Workspace_sptr outWS = loader.getProperty("OutputWorkspace");
    WorkspaceGroup_sptr outGroup =
        boost::dynamic_pointer_cast<WorkspaceGroup>(outWS);
    TSM_ASSERT("Invalid Output Workspace Type", outGroup);

    IEventWorkspace_sptr firstWS =
        boost::dynamic_pointer_cast<IEventWorkspace>(outGroup->getItem(0));
    auto run = firstWS->run();
    const int nPeriods = run.getPropertyValueAsType<int>("nperiods");
    TSM_ASSERT_EQUALS("Wrong number of periods extracted", nPeriods, 4);
    TSM_ASSERT_EQUALS("Groups size should be same as nperiods",
                      outGroup->size(), nPeriods);

    for (size_t i = 0; i < outGroup->size(); ++i) {
      EventWorkspace_sptr ws =
          boost::dynamic_pointer_cast<EventWorkspace>(outGroup->getItem(i));
      TS_ASSERT(ws);
      TSM_ASSERT("Non-zero events in each period", ws->getNumberEvents() > 0);

      std::stringstream buffer;
      buffer << "period " << i + 1;
      std::string periodBoolLog = buffer.str();

      const int currentPeriod =
          ws->run().getPropertyValueAsType<int>("current_period");

      TSM_ASSERT(
          "Each period should have a boolean array for masking period numbers",
          ws->run().hasProperty(periodBoolLog));
      TSM_ASSERT_EQUALS("Current period is not what was expected.",
                        currentPeriod, i + 1);
    }
    // Make sure that the spectraNo are equal for all child workspaces.
    auto isFirstChildWorkspace = true;
    std::vector<Mantid::specnum_t> specids;

    for (size_t i = 0; i < outGroup->size(); ++i) {
      EventWorkspace_sptr ws =
          boost::dynamic_pointer_cast<EventWorkspace>(outGroup->getItem(i));
      if (isFirstChildWorkspace) {
        specids.reserve(ws->getNumberHistograms());
      }
      for (size_t index = 0; index < ws->getNumberHistograms(); ++index) {
        if (isFirstChildWorkspace) {
          specids.push_back(ws->getSpectrum(index).getSpectrumNo());
        } else {
          TSM_ASSERT_EQUALS(
              "The spectrNo should be the same for all child workspaces.",
              specids[index], ws->getSpectrum(index).getSpectrumNo());
        }
      }

      isFirstChildWorkspace = false;
    }
  }

  void test_MPI_load() {
    // Note that this and other MPI tests currently work only in non-MPI builds
    // with the default event loader, i.e., ParallelEventLoader is not
    // supported. The reason is the locking we need in the test for HDF5 access,
    // which implies that the communication within ParallelEventLoader will
    // simply get stuck. Additionally, it will fail for the CNCS file since
    // empty banks contain a dummy event with an invalid event ID, which
    // ParallelEventLoader does not support.
    int threads = 3; // Limited number of threads to avoid long running test.
    ParallelTestHelpers::ParallelRunner runner(threads);
    // Test reads from multiple threads, which is not supported by our HDF5
    // libraries, so we need a mutex.
    auto hdf5Mutex = boost::make_shared<std::mutex>();
    runner.run(run_MPI_load, hdf5Mutex, "CNCS_7860_event.nxs");
  }

  void test_MPI_load_ISIS() {
    int threads = 3; // Limited number of threads to avoid long running test.
    ParallelTestHelpers::ParallelRunner runner(threads);
    // Test reads from multiple threads, which is not supported by our HDF5
    // libraries, so we need a mutex.
    auto hdf5Mutex = boost::make_shared<std::mutex>();
    runner.run(run_MPI_load, hdf5Mutex, "SANS2D00022048.nxs");
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

#endif /*LOADEVENTNEXUSTEST_H_*/
