// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileLoaderRegistry.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadMuonNexusV2.h"
#include "MantidDataHandling/SaveNexusMuon.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/EmptyValues.h"

#include <cxxtest/TestSuite.h>

#include <filesystem>

using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::Kernel;

namespace {
const std::string INPUT_FILE{"EMU00102347.nxs_v2"};

struct LoadedRun {
  MatrixWorkspace_sptr workspace;
  ITableWorkspace_sptr deadTimes;
  ITableWorkspace_sptr grouping;
  double timeZero;
  double firstGoodData;
  double lastGoodData;
  std::string mainFieldDirection;
};

LoadedRun loadRun(const std::string &filename, const std::string &prefix, const bool correctTime = true) {
  LoadMuonNexusV2 loader;
  loader.initialize();
  loader.setPropertyValue("Filename", filename);
  loader.setPropertyValue("OutputWorkspace", prefix + "_ws");
  loader.setPropertyValue("DeadTimeTable", prefix + "_deadtimes");
  loader.setPropertyValue("DetectorGroupingTable", prefix + "_grouping");
  loader.setProperty("CorrectTime", correctTime);
  loader.execute();
  auto &ads = AnalysisDataService::Instance();
  return LoadedRun{ads.retrieveWS<MatrixWorkspace>(prefix + "_ws"),
                   ads.retrieveWS<ITableWorkspace>(prefix + "_deadtimes"),
                   ads.retrieveWS<ITableWorkspace>(prefix + "_grouping"),
                   loader.getProperty("TimeZero"),
                   loader.getProperty("FirstGoodData"),
                   loader.getProperty("LastGoodData"),
                   loader.getPropertyValue("MainFieldDirection")};
}

ITableWorkspace_sptr deadTimeTable(const std::vector<std::pair<int, double>> &rows) {
  auto table = WorkspaceFactory::Instance().createTable();
  table->addColumn("int", "spectrum");
  table->addColumn("double", "dead-time");
  for (const auto &[spectrum, deadTime] : rows) {
    TableRow row = table->appendRow();
    row << spectrum << deadTime;
  }
  return table;
}

ITableWorkspace_sptr groupingTable(const std::vector<std::vector<int>> &groups) {
  auto table = WorkspaceFactory::Instance().createTable();
  table->addColumn("vector_int", "Detectors");
  for (const auto &group : groups) {
    TableRow row = table->appendRow();
    row << group;
  }
  return table;
}
} // namespace

class SaveNexusMuonTest : public CxxTest::TestSuite {
public:
  static SaveNexusMuonTest *createSuite() { return new SaveNexusMuonTest(); }
  static void destroySuite(SaveNexusMuonTest *suite) { delete suite; }

  SaveNexusMuonTest() : m_filename((std::filesystem::temp_directory_path() / "SaveNexusMuonTest.nxs").string()) {}

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    std::filesystem::remove(m_filename);
  }

  void test_init() {
    SaveNexusMuon alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_saved_file_is_loaded_by_LoadMuonNexusV2() {
    const auto original = loadRun(INPUT_FILE, "original");
    save(original);

    TS_ASSERT_EQUALS(FileLoaderRegistry::Instance().chooseLoader(m_filename)->name(), "LoadMuonNexusV2");
  }

  void test_round_trip_preserves_data_and_muon_properties() {
    const auto original = loadRun(INPUT_FILE, "original");
    save(original);
    const auto saved = loadRun(m_filename, "saved");

    assertDataEqual(*original.workspace, *saved.workspace);
    TS_ASSERT_DELTA(saved.timeZero, original.timeZero, 1e-6);
    TS_ASSERT_DELTA(saved.firstGoodData, original.firstGoodData, 1e-6);
    TS_ASSERT_DELTA(saved.lastGoodData, original.lastGoodData, 1e-6);
    TS_ASSERT_EQUALS(saved.mainFieldDirection, original.mainFieldDirection);

    TS_ASSERT_EQUALS(saved.deadTimes->rowCount(), original.deadTimes->rowCount());
    for (size_t row = 0; row < original.deadTimes->rowCount(); ++row) {
      TS_ASSERT_EQUALS(saved.deadTimes->cell<int>(row, 0), original.deadTimes->cell<int>(row, 0));
      TS_ASSERT_DELTA(saved.deadTimes->cell<double>(row, 1), original.deadTimes->cell<double>(row, 1), 1e-6);
    }
    TS_ASSERT_EQUALS(saved.grouping->rowCount(), original.grouping->rowCount());
    for (size_t row = 0; row < original.grouping->rowCount(); ++row) {
      TS_ASSERT_EQUALS(saved.grouping->cell<std::vector<int>>(row, 0),
                       original.grouping->cell<std::vector<int>>(row, 0));
    }
  }

  void test_round_trip_preserves_run_information_and_logs() {
    const auto original = loadRun(INPUT_FILE, "original");
    save(original);
    const auto saved = loadRun(m_filename, "saved");

    const auto &originalRun = original.workspace->run();
    const auto &savedRun = saved.workspace->run();
    // Single values and time series (double, int and string) logs
    for (const auto &logName :
         {"run_number", "run_start", "run_end", "run_title", "goodfrm", "sample_temp", "sample_magn_field",
          "main_field_direction", "FirstGoodData", "period_labels", "frames_period_raw", "total_counts_period",
          "gd_prtn_chrg", "rawfrm", "Temp_Sample", "count_rate", "good_frames", "icp_event", "tpar_file"}) {
      TSM_ASSERT(logName, savedRun.hasProperty(logName));
      if (savedRun.hasProperty(logName)) {
        TSM_ASSERT_EQUALS(logName, savedRun.getProperty(logName)->value(), originalRun.getProperty(logName)->value());
      }
    }
    TS_ASSERT(savedRun.hasProperty("Temp_SP_invalid_values"));
    TS_ASSERT_EQUALS(saved.workspace->getInstrument()->getName(), "EMU");
    TS_ASSERT_EQUALS(saved.workspace->sample().getName(), original.workspace->sample().getName());
    TS_ASSERT_EQUALS(saved.workspace->getComment(), original.workspace->getComment());
  }

  void test_round_trip_without_time_zero_correction() {
    const auto original = loadRun(INPUT_FILE, "original", false);
    save(original, false);
    const auto saved = loadRun(m_filename, "saved", false);

    assertDataEqual(*original.workspace, *saved.workspace);
    TS_ASSERT_DELTA(saved.timeZero, original.timeZero, 1e-6);
    TS_ASSERT_DELTA(saved.firstGoodData, original.firstGoodData, 1e-6);
  }

  void test_explicit_properties_override_logs() {
    const auto original = loadRun(INPUT_FILE, "original");
    auto alg = createSaver(original);
    alg->setProperty("FirstGoodData", 0.48);
    alg->setProperty("FirstGoodDataRelativeToTimeZero", false);
    alg->setProperty("MainFieldDirection", "Transverse");
    alg->setProperty("DeadTimeTable", deadTimeTable({{1, 0.01}, {96, 0.02}}));
    alg->setProperty("DetectorGroupingTable", groupingTable({{1, 2, 3}, {4, 5}}));
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    const auto saved = loadRun(m_filename, "saved");

    TS_ASSERT_DELTA(saved.firstGoodData, 0.48, 1e-6);
    TS_ASSERT_EQUALS(saved.mainFieldDirection, "Transverse");
    TS_ASSERT_DELTA(saved.deadTimes->cell<double>(0, 1), 0.01, 1e-6);
    TS_ASSERT_DELTA(saved.deadTimes->cell<double>(1, 1), 0.0, 1e-6);
    TS_ASSERT_DELTA(saved.deadTimes->cell<double>(95, 1), 0.02, 1e-6);
    TS_ASSERT_EQUALS(saved.grouping->rowCount(), 2);
    TS_ASSERT_EQUALS(saved.grouping->cell<std::vector<int>>(0, 0), std::vector<int>({1, 2, 3}));
    TS_ASSERT_EQUALS(saved.grouping->cell<std::vector<int>>(1, 0), std::vector<int>({4, 5}));
  }

  void test_saving_without_tables_gives_zero_dead_times_and_default_grouping() {
    const auto original = loadRun(INPUT_FILE, "original");
    auto alg = createSaver(original);
    alg->setPropertyValue("DeadTimeTable", "");
    alg->setPropertyValue("DetectorGroupingTable", "");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    const auto saved = loadRun(m_filename, "saved");

    TS_ASSERT_EQUALS(saved.deadTimes->rowCount(), 96);
    TS_ASSERT_DELTA(saved.deadTimes->cell<double>(10, 1), 0.0, 1e-12);
    // Without grouping in the file, the default grouping of the instrument is loaded
    TS_ASSERT_EQUALS(saved.grouping->rowCount(), original.grouping->rowCount());
  }

  void test_workspace_group_is_rejected() {
    const auto original = loadRun(INPUT_FILE, "original");
    auto group = std::make_shared<WorkspaceGroup>();
    group->addWorkspace(original.workspace);
    auto alg = createSaver(original);
    alg->setProperty("InputWorkspace", std::static_pointer_cast<Workspace>(group));
    assertIssue(*alg, "InputWorkspace", "multi-period");
  }

  void test_time_zero_must_be_set() {
    const auto original = loadRun(INPUT_FILE, "original");
    auto alg = createSaver(original);
    alg->setProperty("TimeZero", Mantid::EMPTY_DBL());
    assertIssue(*alg, "TimeZero", "must be set");
  }

  void test_non_integer_counts_are_rejected() {
    const auto original = loadRun(INPUT_FILE, "original");
    original.workspace->mutableY(3)[10] = 1.5;
    auto alg = createSaver(original);
    assertIssue(*alg, "InputWorkspace", "whole numbers");
  }

  void test_irregular_last_bin_is_saved() {
    // Older muon files can have a last bin of a different width, which must still round trip
    const auto original = loadRun(INPUT_FILE, "original");
    setCommonX(*original.workspace, [](auto &x) { x[x.size() - 1] -= 0.3 * (x[x.size() - 1] - x[x.size() - 2]); });
    save(original);
    const auto saved = loadRun(m_filename, "saved");

    // The modified edge is not exactly representable as a float, which is how raw_time is stored in the file
    assertDataEqual(*original.workspace, *saved.workspace, 1e-5);
    TS_ASSERT_DELTA(saved.firstGoodData, original.firstGoodData, 1e-6);
  }

  void test_decreasing_time_axis_is_rejected() {
    const auto original = loadRun(INPUT_FILE, "original");
    setCommonX(*original.workspace, [](auto &x) { x[2] = x[0]; });
    auto alg = createSaver(original);
    assertIssue(*alg, "InputWorkspace", "increasing");
  }

  void test_missing_required_log_is_rejected() {
    const auto original = loadRun(INPUT_FILE, "original");
    original.workspace->mutableRun().removeProperty("sample_temp");
    auto alg = createSaver(original);
    assertIssue(*alg, "InputWorkspace", "sample_temp");
  }

  void test_first_good_data_relative_to_time_zero() {
    // By default FirstGoodData is relative to time zero, as shown in Muon Analysis
    const auto original = loadRun(INPUT_FILE, "original");
    auto alg = createSaver(original);
    alg->setProperty("FirstGoodData", 0.224);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    const auto saved = loadRun(m_filename, "saved");

    TS_ASSERT_DELTA(saved.firstGoodData, 0.224 + original.timeZero, 1e-6);
    TS_ASSERT_DELTA(saved.workspace->run().getPropertyAsSingleValue("FirstGoodData"), 0.224, 1e-6);
  }

  void test_absolute_first_good_data_before_time_zero_is_rejected() {
    // Time zero is 0.16, so an absolute first good data of 0.1 is before it
    const auto original = loadRun(INPUT_FILE, "original");
    auto alg = createSaver(original);
    alg->setProperty("FirstGoodData", 0.1);
    alg->setProperty("FirstGoodDataRelativeToTimeZero", false);
    assertIssue(*alg, "FirstGoodData", "before TimeZero");
  }

  void test_relative_first_good_data_before_time_zero_is_rejected() {
    const auto original = loadRun(INPUT_FILE, "original");
    auto alg = createSaver(original);
    alg->setProperty("FirstGoodData", -0.05);
    assertIssue(*alg, "FirstGoodData", "before TimeZero");
  }

  void test_first_good_data_log_before_time_zero_is_rejected() {
    const auto original = loadRun(INPUT_FILE, "original");
    original.workspace->mutableRun().addProperty("FirstGoodData", -0.05, true);
    auto alg = createSaver(original);
    assertIssue(*alg, "FirstGoodData", "before TimeZero");
  }

  void test_dead_time_for_unknown_spectrum_is_rejected() {
    const auto original = loadRun(INPUT_FILE, "original");
    auto alg = createSaver(original);
    alg->setProperty("DeadTimeTable", deadTimeTable({{1, 0.01}, {200, 0.02}}));
    assertIssue(*alg, "DeadTimeTable", "200");
  }

  void test_detector_in_two_groups_is_rejected() {
    const auto original = loadRun(INPUT_FILE, "original");
    auto alg = createSaver(original);
    alg->setProperty("DetectorGroupingTable", groupingTable({{1, 2, 3}, {3, 4}}));
    assertIssue(*alg, "DetectorGroupingTable", "more than one group");
  }

private:
  std::shared_ptr<SaveNexusMuon> createSaver(const LoadedRun &run, const bool timeZeroCorrected = true) const {
    auto alg = std::make_shared<SaveNexusMuon>();
    alg->initialize();
    alg->setRethrows(true);
    alg->setProperty("InputWorkspace", std::static_pointer_cast<Workspace>(run.workspace));
    alg->setPropertyValue("Filename", m_filename);
    alg->setProperty("TimeZero", run.timeZero);
    alg->setProperty("TimeZeroCorrected", timeZeroCorrected);
    alg->setProperty("DeadTimeTable", run.deadTimes);
    alg->setProperty("DetectorGroupingTable", run.grouping);
    return alg;
  }

  void save(const LoadedRun &run, const bool timeZeroCorrected = true) const {
    auto alg = createSaver(run, timeZeroCorrected);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
  }

  void assertIssue(SaveNexusMuon &alg, const std::string &property, const std::string &message) const {
    const auto issues = alg.validateInputs();
    TSM_ASSERT(property, issues.contains(property));
    if (issues.contains(property)) {
      TSM_ASSERT(issues.at(property), issues.at(property).contains(message));
    }
    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);
    TS_ASSERT(!std::filesystem::exists(m_filename));
  }

  template <typename Modify> static void setCommonX(MatrixWorkspace &ws, Modify modify) {
    auto x = ws.x(0);
    modify(x);
    for (size_t i = 0; i < ws.getNumberHistograms(); ++i) {
      ws.mutableX(i) = x;
    }
  }

  void assertDataEqual(const MatrixWorkspace &expected, const MatrixWorkspace &actual,
                       const double xTolerance = 1e-6) const {
    TS_ASSERT_EQUALS(actual.getNumberHistograms(), expected.getNumberHistograms());
    TS_ASSERT_EQUALS(actual.blocksize(), expected.blocksize());
    for (size_t i = 0; i < expected.getNumberHistograms(); ++i) {
      TS_ASSERT_EQUALS(actual.getSpectrum(i).getSpectrumNo(), expected.getSpectrum(i).getSpectrumNo());
      TS_ASSERT_EQUALS(actual.getSpectrum(i).getDetectorIDs(), expected.getSpectrum(i).getDetectorIDs());
      TS_ASSERT_EQUALS(actual.y(i).rawData(), expected.y(i).rawData());
      TS_ASSERT_EQUALS(actual.e(i).rawData(), expected.e(i).rawData());
      for (size_t j = 0; j < expected.x(i).size(); ++j) {
        TS_ASSERT_DELTA(actual.x(i)[j], expected.x(i)[j], xTolerance);
      }
    }
  }

  std::string m_filename;
};
