// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadNexusMonitors2.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/ConfigService.h"
#include "MantidNexus/NeXusFile.hpp"
#include <Poco/File.h>
#include <Poco/Path.h>
#include <cxxtest/TestSuite.h>
#include <memory>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

class LoadNexusMonitorsTest : public CxxTest::TestSuite {
public:
  void testExec() {
    Mantid::API::FrameworkManager::Instance();
    LoadNexusMonitors2 ld2;
    std::string outws_name = "cncs";
    ld2.initialize();
    ld2.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    ld2.setPropertyValue("OutputWorkspace", outws_name);

    ld2.execute();
    TS_ASSERT(ld2.isExecuted());

    MatrixWorkspace_sptr WS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outws_name);
    // Valid WS and it is an MatrixWorkspace
    TS_ASSERT(WS);
    // Correct number of monitors found
    TS_ASSERT_EQUALS(WS->getNumberHistograms(), 3);
    // check the detector and spectrum numbers
    for (size_t wi = 0; wi < WS->getNumberHistograms(); ++wi) {
      const auto &spec = WS->getSpectrum(wi);
      TS_ASSERT_EQUALS(spec.getSpectrumNo(), wi + 1);
      TS_ASSERT_EQUALS(*(spec.getDetectorIDs().begin()), -1 * (wi + 1));
    }
    // Check some histogram data
    // TOF
    TS_ASSERT_EQUALS(WS->x(1).size(), 200002);
    TS_ASSERT_DELTA(WS->x(1)[3412], 3412.0, 1e-6);
    // Data
    TS_ASSERT_EQUALS(WS->y(1).size(), 200001);
    TS_ASSERT_DELTA(WS->y(1)[3412], 197., 1e-6);
    // Error
    TS_ASSERT_EQUALS(WS->e(1).size(), 200001);
    TS_ASSERT_DELTA(WS->e(1)[3412], 14.03567, 1e-4);
    // Check geometry for a monitor
    const auto &specInfo = WS->spectrumInfo();
    TS_ASSERT(specInfo.isMonitor(2));
    TS_ASSERT_EQUALS(specInfo.detector(2).getID(), -3);
    TS_ASSERT_DELTA(specInfo.samplePosition().distance(specInfo.position(2)), 1.426, 1e-6);
    // Check if filename is saved
    TS_ASSERT_EQUALS(ld2.getPropertyValue("Filename"), WS->run().getProperty("Filename")->value());
  }

  void test_with_custom_top_level_entry_name() {
    LoadNexusMonitors2 alg;
    alg.initialize();
    alg.setChild(true);
    alg.setRethrows(true);
    alg.setProperty("Filename", "LARMOR00003368.nxs");
    alg.setProperty("NXentryName",
                    "raw_data_1"); // Actual entry name, provided as custom entry
    alg.setPropertyValue("OutputWorkspace", "dummy");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
  }

  void testExecEvent() {
    Mantid::API::FrameworkManager::Instance();
    LoadNexusMonitors2 ld2;
    std::string outws_name = "hyspec";
    ld2.initialize();
    ld2.setPropertyValue("Filename", "HYSA_2411_monitors.nxs.h5");
    ld2.setPropertyValue("OutputWorkspace", outws_name);

    ld2.execute();
    TS_ASSERT(ld2.isExecuted());
    EventWorkspace_sptr WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws_name);
    // Valid WS and it is an MatrixWorkspace
    TS_ASSERT(WS);
    // Correct number of monitors found
    TS_ASSERT_EQUALS(WS->getNumberHistograms(), 2);
    // Verify number of events loaded
    TS_ASSERT_EQUALS(WS->getSpectrum(0).getNumberEvents(), 15000);
    TS_ASSERT_EQUALS(WS->getSpectrum(1).getNumberEvents(), 15000);
  }

  void testOldFile() {
    // Just need to make sure it runs.
    Mantid::API::FrameworkManager::Instance();
    LoadNexusMonitors2 ld2;
    std::string outws_name = "ARCS_2963_monitors";
    ld2.initialize();
    ld2.setPropertyValue("Filename", "ARCS_2963.nxs");
    ld2.setPropertyValue("OutputWorkspace", outws_name);
    TS_ASSERT_THROWS_NOTHING(ld2.execute());
    TS_ASSERT(ld2.isExecuted());
  }

  void testBrokenISISFile() {
    // Just need to make sure it runs.
    Mantid::API::FrameworkManager::Instance();
    LoadNexusMonitors2 ld2;
    std::string outws_name = "LOQ_49886_monitors";
    ld2.initialize();
    ld2.setPropertyValue("Filename", "LOQ49886.nxs");
    ld2.setPropertyValue("OutputWorkspace", outws_name);
    TS_ASSERT_THROWS_NOTHING(ld2.execute());
    TS_ASSERT(ld2.isExecuted());

    MatrixWorkspace_sptr WS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outws_name);
    // Valid WS and it is an MatrixWorkspace
    TS_ASSERT(WS);
    // Correct number of monitors found
    TS_ASSERT_EQUALS(WS->getNumberHistograms(), 2);
    // Monitors data is correct
    TS_ASSERT_EQUALS(WS->y(0)[0], 0);
    TS_ASSERT_EQUALS(WS->y(1)[0], 0);

    TS_ASSERT_EQUALS(WS->x(0)[0], 5.0);
    TS_ASSERT_EQUALS(WS->x(1)[5], 19995.0);
  }

  void testLET() {
    std::string filename("LET00006278.nxs");
    std::string histoWSname("testLET_hist_mon");
    std::string eventWSname("testLET_event_mon");

    const std::vector<size_t> spec_num{40961, 40962, 40963, 40964, 40965, 40966, 40967, 40968};
    const std::vector<int> det_num{11, 21, 31, 41, 51, 61, 71, 81};

    // these tests read from the same file
    LoadNexusMonitors2 load;
    load.initialize();
    load.setPropertyValue("Filename", filename);

    // do not specify LoadOnly - it should load histograms by default
    load.setPropertyValue("OutputWorkspace", histoWSname);
    load.execute();
    TS_ASSERT(load.isExecuted());
    MatrixWorkspace_sptr histo = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(histoWSname);
    TS_ASSERT_EQUALS(histo->getNumberHistograms(), spec_num.size());
    for (size_t wi = 0; wi < histo->getNumberHistograms(); ++wi) {
      const auto &histoSpec = histo->getSpectrum(wi);
      TS_ASSERT_EQUALS(histoSpec.getSpectrumNo(), spec_num[wi]);
      TS_ASSERT_EQUALS(*(histoSpec.getDetectorIDs().begin()), det_num[wi]);
    }

    // force loading events
    load.setPropertyValue("LoadOnly", "Events");
    load.setPropertyValue("OutputWorkspace", eventWSname);
    load.execute();
    TS_ASSERT(load.isExecuted());
    EventWorkspace_sptr event = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(eventWSname);
    TS_ASSERT_EQUALS(event->getNumberHistograms(), spec_num.size());
    for (size_t wi = 0; wi < histo->getNumberHistograms(); ++wi) {
      const auto &eventSpec = event->getSpectrum(wi);
      TS_ASSERT_EQUALS(eventSpec.getSpectrumNo(), spec_num[wi]);
      TS_ASSERT_EQUALS(*(eventSpec.getDetectorIDs().begin()), det_num[wi]);
    }
  }

  void test_10_monitors() {
    Poco::Path path(ConfigService::Instance().getTempDir().c_str());
    path.append("LoadNexusMonitorsTestFile.nxs");
    std::string filename = path.toString();

    createFakeFile(filename);

    LoadNexusMonitors2 ld2;
    std::string outws_name = "10monitors";
    ld2.initialize();
    ld2.setPropertyValue("Filename", filename);
    ld2.setPropertyValue("OutputWorkspace", outws_name);
    ld2.execute();
    TS_ASSERT(ld2.isExecuted());

    MatrixWorkspace_sptr WS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outws_name);
    // Valid WS and it is an MatrixWorkspace
    TS_ASSERT(WS);
    // Correct number of monitors found
    TS_ASSERT_EQUALS(WS->getNumberHistograms(), 3);
    // Correct spectrum and detector numbers
    const std::vector<int32_t> NUMS{1, 2, 10};
    for (size_t wi = 0; wi < WS->getNumberHistograms(); ++wi) {
      const auto &spec = WS->getSpectrum(wi);
      TS_ASSERT_EQUALS(spec.getSpectrumNo(), NUMS[wi]);
      TS_ASSERT_EQUALS(*(spec.getDetectorIDs().begin()), NUMS[wi]);
    }
    // Monitors are in the right order
    TS_ASSERT_EQUALS(WS->y(0)[0], 1);
    TS_ASSERT_EQUALS(WS->y(1)[0], 2);
    TS_ASSERT_EQUALS(WS->y(2)[0], 10);

    AnalysisDataService::Instance().clear();
    Poco::File(filename).remove();
  }

  void testPythonOutputFix() {
    std::string filename = "LARMOR00003368.nxs";
    std::string outws_name = "ws_group";

    // Version 2 of algorithm
    LoadNexusMonitors2 ld2;
    ld2.initialize();
    ld2.setPropertyValue("Filename", filename);
    ld2.setPropertyValue("OutputWorkspace", outws_name);
    ld2.execute();

    // Count output workspaces
    int ws_count = 0;
    auto props = ld2.getProperties();
    for (auto &prop : props)
      if (prop->type() == "Workspace")
        ws_count++;

    // Version 2 always produces one OutputWorkspace, which may be a group
    // workspace in case of a multiperiod nexus file input.
    TS_ASSERT_EQUALS(ws_count, 1);

    // Child workspace names also have proper output workspace name prefix
    WorkspaceGroup_sptr ws_group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outws_name);
    TS_ASSERT(ws_group->contains(outws_name + "_1"));
  }

  void createFakeFile(const std::string &filename) {
    NeXus::File file(filename, NXACC_CREATE5);

    const bool openGroup = true;
    file.makeGroup("raw_data_1", "NXentry", openGroup);
    {
      addMonitor(file, 1);
      addMonitor(file, 10);
      addMonitor(file, 2);

      file.makeGroup("instrument", "NXinstrument", openGroup);
      file.writeData("name", "FakeInstrument");
      file.closeGroup();
    }
    file.closeGroup(); // raw_data_1
    file.close();
  }

  void addMonitor(NeXus::File &file, int i) {
    const size_t nbins = 3;
    std::string monitorName = "monitor_" + boost::lexical_cast<std::string>(i);
    const bool openGroup = true;
    file.makeGroup(monitorName, "NXmonitor", openGroup);
    file.writeData("monitor_number", i);
    file.writeData("spectrum_index", i);
    ::NeXus::DimVector dims(3, 1);
    dims[2] = nbins;
    std::vector<int> data(nbins, i);
    file.writeData("data", data, dims);
    std::vector<float> timeOfFlight(nbins + 1);
    file.writeData("time_of_flight", timeOfFlight);
    file.closeGroup();
  }
};

class LoadNexusMonitorsTestPerformance : public CxxTest::TestSuite {
public:
  static LoadNexusMonitorsTestPerformance *createSuite() { return new LoadNexusMonitorsTestPerformance(); }

  static void destroySuite(LoadNexusMonitorsTestPerformance *suite) { delete suite; }

  void setUp() override { ld2.initialize(); }

  void tearDown() override {
    AnalysisDataService::Instance().remove("cncs");
    AnalysisDataService::Instance().remove("hyspec");
  }

  void testExecV2() {
    Mantid::API::FrameworkManager::Instance();
    ld2.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    ld2.setPropertyValue("OutputWorkspace", "cncs");

    ld2.execute();
  }

  void testExecEventV2() {
    Mantid::API::FrameworkManager::Instance();
    ld2.setPropertyValue("Filename", "HYSA_2411_monitors.nxs.h5");
    ld2.setPropertyValue("OutputWorkspace", "hyspec");

    ld2.execute();
  }

private:
  LoadNexusMonitors2 ld2;
};
