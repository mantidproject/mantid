// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <MantidAPI/AnalysisDataService.h>
#include <MantidAPI/FileFinder.h>
#include <MantidAPI/Run.h>
#include <MantidDataHandling/LoadNGEM.h>
#include <MantidDataObjects/EventWorkspace.h>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class LoadNGEMTest : public CxxTest::TestSuite {
public:
  void test_init() {
    LoadNGEM alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", getTestFilePath("GEM000005_00_000_short.edb")));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BinWidth", 10.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MinEventsPerFrame", 10));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxEventsPerFrame", 20));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateEventsPerFrame", false));
  }

  void test_exec_loads_data_to_ads() {
    LoadNGEM alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", getTestFilePath("GEM000005_00_000_short.edb")));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BinWidth", 0.1));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateEventsPerFrame", false));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    auto &ads = AnalysisDataService::Instance();
    auto outputWS = ads.retrieveWS<DataObjects::EventWorkspace>("ws");
    // check some random values
    const auto &ydata{outputWS->y(100)};
    const auto &xdata{outputWS->x(100)};
    const auto &edata{outputWS->e(100)};
    TS_ASSERT_DELTA(1.0, ydata[130378], 1e-8);
    TS_ASSERT_DELTA(13037.8, xdata[130378], 1e-8);
    TS_ASSERT_DELTA(13037.9, xdata[130379], 1e-8);
    TS_ASSERT_DELTA(1.0, edata[130378], 1e-8);
    // sample logs
    const auto &run = outputWS->run();
    TS_ASSERT_DELTA(700.92, run.getPropertyValueAsType<double>("min_TOF"), 1e-08);
    TS_ASSERT_DELTA(98132.97, run.getPropertyValueAsType<double>("max_TOF"), 1e-08);
    TS_ASSERT_EQUALS(224, run.getPropertyValueAsType<int>("raw_frames"));
    TS_ASSERT_EQUALS(224, run.getPropertyValueAsType<int>("good_frames"));

    TS_ASSERT_THROWS_NOTHING(ads.remove("ws"));
  }

  void test_exec_loads_event_counts_workspace() {
    LoadNGEM alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", getTestFilePath("GEM000005_00_000_short.edb")));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateEventsPerFrame", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove("ws"));
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove("ws_event_counts"));
  }

  void test_exec_not_load_event_counts_workspace() {
    LoadNGEM alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", getTestFilePath("GEM000005_00_000_short.edb")));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("GenerateEventsPerFrame", false));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().remove("ws"));
    TS_ASSERT_THROWS_ANYTHING(AnalysisDataService::Instance().retrieve("ws_event_counts"));
  }

  void test_init_fails_on_bad_binWidth() {
    LoadNGEM alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_ANYTHING(alg.setProperty("BinWidth", -10.0));
  }

  void test_init_fails_on_bad_MaxEventsPerFrame() {
    LoadNGEM alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_ANYTHING(alg.setProperty("MaxEventsPerFrame", -10));
  }

  void test_init_fails_on_bad_MinEventsPerFrame() {
    LoadNGEM alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_ANYTHING(alg.setProperty("MinEventsPerFrame", -10));
  }

  void test_init_fails_on_MaxEvents_is_less_than_MinEvents() {
    LoadNGEM alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", getTestFilePath("GEM000005_00_000_short.edb")));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MinEventsPerFrame", 20));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxEventsPerFrame", 10));

    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
  }

  void test_MinEventsPerFrame_removes_low_values() {
    LoadNGEM alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", getTestFilePath("GEM000005_00_000_short.edb")));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MinEventsPerFrame", 0));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    DataObjects::EventWorkspace_sptr ws;

    ws = AnalysisDataService::Instance().retrieveWS<DataObjects::EventWorkspace>("ws");
    TS_ASSERT(ws);
    const size_t rawNumEvents = ws->getNumberEvents();

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MinEventsPerFrame", Mantid::EMPTY_INT()));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    ws = AnalysisDataService::Instance().retrieveWS<DataObjects::EventWorkspace>("ws");
    TS_ASSERT(rawNumEvents > ws->getNumberEvents());
  }

  void test_MaxEventsPerFrame_removes_high_values() {
    LoadNGEM alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Filename", getTestFilePath("GEM000005_00_000_short.edb")));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("OutputWorkspace", "ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaxEventsPerFrame", 0));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    DataObjects::EventWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<DataObjects::EventWorkspace>("ws");
    TS_ASSERT(ws);
    TS_ASSERT_EQUALS(ws->getNumberEvents(), 0);
  }

private:
  // Helper function to get the path of a specified file.
  std::string getTestFilePath(const std::string &filename) {
    const std::string filepath = Mantid::API::FileFinder::Instance().getFullPath(filename);
    TS_ASSERT_DIFFERS(filepath, "");
    return filepath;
  }
};
