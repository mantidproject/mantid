// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/LogManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadNexusLogs.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/LogFilter.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

#include "MantidAPI/WorkspaceGroup.h"
#include <cxxtest/TestSuite.h>

class LoadNexusLogsTest : public CxxTest::TestSuite {
public:
  void test_File_With_DASLogs() {
    Mantid::API::FrameworkManager::Instance();
    LoadNexusLogs ld;
    std::string outws_name = "REF_L_instrument";
    ld.initialize();
    ld.setPropertyValue("Filename", "REF_L_32035.nxs");
    MatrixWorkspace_sptr ws = createTestWorkspace();
    // Put it in the object.
    ld.setProperty("Workspace", ws);
    ld.execute();
    TS_ASSERT(ld.isExecuted());

    double val;
    Run &run = ws->mutableRun();
    // Do we have all we expect
    const std::vector<Property *> &logs = run.getLogData();
    TS_ASSERT_EQUALS(logs.size(), 75);
    Property *prop;
    TimeSeriesProperty<double> *dProp;

    prop = run.getLogData("Speed3");
    TS_ASSERT(prop);
    // TS_ASSERT_EQUALS( prop->value(), "60");
    TS_ASSERT_EQUALS(prop->units(), "Hz");

    prop = run.getLogData("PhaseRequest1");
    dProp = dynamic_cast<TimeSeriesProperty<double> *>(prop);
    TS_ASSERT(dProp);
    val = dProp->nthValue(0);
    TS_ASSERT_DELTA(val, 13712.77, 1e-2);
    TS_ASSERT_EQUALS(prop->units(), "microsecond");

    TimeSeriesProperty<double> *tsp;

    prop = run.getLogData("Phase1");
    tsp = dynamic_cast<TimeSeriesProperty<double> *>(prop);
    TS_ASSERT(tsp);
    TS_ASSERT_EQUALS(tsp->units(), "microsecond");
    TS_ASSERT_DELTA(tsp->nthValue(1), 13715.55, 2);

    // The time diff between the 0th and 1st entry is 0.328 seconds
    TS_ASSERT_DELTA(Types::Core::DateAndTime::secondsFromDuration(tsp->nthInterval(0).length()), 0.328, 0.01);

    // Now the stats
  }

  void test_File_With_Runlog_And_Selog() {
    LoadNexusLogs loader;
    loader.initialize();
    MatrixWorkspace_sptr testWS = createTestWorkspace();
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Workspace", testWS));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", "LOQ49886.nxs"));
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    const API::Run &run = testWS->run();
    const std::vector<Property *> &logs = run.getLogData();
    TS_ASSERT_EQUALS(logs.size(),
                     37); // 34 logs in file + 1 synthetic nperiods log
                          // + 1 proton_charge_by_period log
                          // + 1 gd_prtn_chrg_unfiltered log

    TimeSeriesProperty<std::string> *slog =
        dynamic_cast<TimeSeriesProperty<std::string> *>(run.getLogData("icp_event"));
    TS_ASSERT(slog);
    std::string str = slog->value();
    TS_ASSERT_EQUALS(str.size(), 1023);
    TS_ASSERT_EQUALS(str.substr(0, 37), "2009-Apr-28 09:20:29  CHANGE_PERIOD 1");

    slog = dynamic_cast<TimeSeriesProperty<std::string> *>(run.getLogData("icp_debug"));
    TS_ASSERT(slog);
    TS_ASSERT_EQUALS(slog->size(), 50);

    TimeSeriesProperty<int> *ilog = dynamic_cast<TimeSeriesProperty<int> *>(run.getLogData("total_counts"));
    TS_ASSERT(ilog);
    TS_ASSERT_EQUALS(ilog->size(), 172);

    ilog = dynamic_cast<TimeSeriesProperty<int> *>(run.getLogData("period"));
    TS_ASSERT(ilog);
    TS_ASSERT_EQUALS(ilog->size(), 172);

    TimeSeriesProperty<double> *dlog = dynamic_cast<TimeSeriesProperty<double> *>(run.getLogData("proton_charge"));
    TS_ASSERT(dlog);
    TS_ASSERT_EQUALS(dlog->size(), 172);
  }

  void test_File_With_Bad_Property() {
    LoadNexusLogs loader;
    loader.initialize();
    MatrixWorkspace_sptr testWS = createTestWorkspace();
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Workspace", testWS));
    TS_ASSERT_THROWS_NOTHING(loader.setPropertyValue("Filename", "IMAT00003680.nxs"));
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    const API::Run &run = testWS->run();

    TimeSeriesProperty<std::string> *putLog =
        dynamic_cast<TimeSeriesProperty<std::string> *>(run.getLogData("EPICS_PUTLOG"));
    TS_ASSERT(putLog);
    std::string str = putLog->value();
    TS_ASSERT_EQUALS(str.size(), 340);
    // Characters 77 + 6 (i.e. 77-83) contain the bad characters
    // Check they were replaced with space characters
    TS_ASSERT_EQUALS(str.substr(77, 6), "E ( $ ");
  }

  void test_extract_nperiod_log_from_event_nexus() {

    auto testWS = createTestWorkspace();
    auto run = testWS->run();
    TSM_ASSERT("Should not have nperiods until we run LoadNexusLogs", !run.hasProperty("nperiods"));
    LoadNexusLogs loader;

    loader.setChild(true);
    loader.initialize();
    loader.setProperty("Workspace", testWS);
    loader.setPropertyValue("Filename", "LARMOR00003368.nxs");
    loader.execute();
    run = testWS->run();

    const bool hasNPeriods = run.hasProperty("nperiods");
    TSM_ASSERT("Should have nperiods now we have run LoadNexusLogs", hasNPeriods);
    if (hasNPeriods) {
      const int nPeriods = run.getPropertyValueAsType<int>("nperiods");
      TSM_ASSERT_EQUALS("Wrong number of periods extracted", nPeriods, 4);
    }
  }

  void test_extract_periods_log_from_event_nexus() {

    auto testWS = createTestWorkspace();
    auto run = testWS->run();
    TSM_ASSERT("Should not have nperiods until we run LoadNexusLogs", !run.hasProperty("nperiods"));
    LoadNexusLogs loader;

    loader.setChild(true);
    loader.initialize();
    loader.setProperty("Workspace", testWS);
    loader.setPropertyValue("Filename", "LARMOR00003368.nxs");
    loader.execute();
    run = testWS->run();

    const bool hasPeriods = run.hasProperty("period_log");
    TSM_ASSERT("Should have period_log now we have run LoadNexusLogs", hasPeriods);

    auto *temp = run.getProperty("period_log");
    auto *periodLog = dynamic_cast<TimeSeriesProperty<int> *>(temp);
    TSM_ASSERT("Period log should be an int time series property", periodLog);

    std::vector<int> periodValues = periodLog->valuesAsVector();
    std::unordered_set<int> uniquePeriods(periodValues.begin(), periodValues.end());
    TSM_ASSERT_EQUALS("Should have 4 periods in total", 4, uniquePeriods.size());

    std::vector<double> protonChargeByPeriod =
        run.getPropertyValueAsType<std::vector<double>>("proton_charge_by_period");
    TSM_ASSERT_EQUALS("Should have four proton charge entries", 4, protonChargeByPeriod.size());
  }

  void test_extract_run_title_from_event_nexus() {

    auto testWS = createTestWorkspace();
    auto run = testWS->run();

    LoadNexusLogs loader;
    loader.setChild(true);
    loader.initialize();
    loader.setProperty("Workspace", testWS);
    loader.setPropertyValue("Filename", "LARMOR00003368.nxs");
    loader.execute();
    run = testWS->run();

    const bool hasTitle = run.hasProperty("run_title");
    TSM_ASSERT("Should have run_title now we have run LoadNexusLogs", hasTitle);

    std::string title = run.getPropertyValueAsType<std::string>("run_title");
    TSM_ASSERT_EQUALS("Run title is not correct", "3He polariser test 0.9bar Long Polariser 0.75A", title);
  }

  void test_log_non_default_entry() {
    auto testWS = createTestWorkspace();
    LoadNexusLogs loader;

    // default entry Off-Off
    loader.setChild(true);
    loader.initialize();
    loader.setProperty("Workspace", testWS);
    loader.setPropertyValue("Filename", "REF_M_9709_event.nxs");
    loader.execute();
    auto run = testWS->run();
    TimeSeriesProperty<double> *pclog = dynamic_cast<TimeSeriesProperty<double> *>(run.getLogData("proton_charge"));
    TS_ASSERT(pclog);
    TS_ASSERT_EQUALS(pclog->size(), 23806);
    // interesting behavior shown
    // last time in seconds is 4.29497e+09 which is 2147-Oct-11 03:51:09
    // but the maximum DateAndTime to be represented is 2136-Feb-20 23:53:38.427387903
    // so the final time in the log ends up being DateAndTime::maximum()
    const double DURATION_EXP =
        DateAndTime::secondsFromDuration(DateAndTime::maximum() - DateAndTime("2011-Sep-03 21:22:53"));
    TS_ASSERT_EQUALS(pclog->getStatistics().duration, DURATION_EXP);

    // 3rd entry On-Off
    testWS = createTestWorkspace();
    loader.setChild(true);
    loader.initialize();
    loader.setProperty("Workspace", testWS);
    loader.setPropertyValue("Filename", "REF_M_9709_event.nxs");
    loader.setProperty("NXentryName", "entry-On_Off");
    loader.execute();
    run = testWS->run();
    pclog = dynamic_cast<TimeSeriesProperty<double> *>(run.getLogData("proton_charge"));
    TS_ASSERT(pclog);
    TS_ASSERT_EQUALS(pclog->size(), 24150);
    TS_ASSERT(pclog->getStatistics().duration < 3e9);
  }

  void test_no_crash_on_2D_array_of_values_on_load() {
    auto testWS = createTestWorkspace();
    LoadNexusLogs loader;
    loader.setChild(true);
    loader.initialize();
    loader.setProperty("Workspace", testWS);
    loader.setPropertyValue("Filename", "larmor_array_time_series_mock.nxs");
    TS_ASSERT_THROWS_NOTHING(loader.execute());
  }

  void test_last_time_series_log_entry_equals_end_time() {
    LoadNexusLogs ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "REF_L_32035.nxs");
    MatrixWorkspace_sptr ws = createTestWorkspace();
    // Put it in the object.
    ld.setProperty("Workspace", ws);
    ld.execute();
    TS_ASSERT(ld.isExecuted());

    auto run = ws->run();
    auto pclog = dynamic_cast<TimeSeriesProperty<double> *>(run.getLogData("PhaseRequest1"));

    TS_ASSERT(pclog);

    const auto lastTime = pclog->lastTime();
    const auto endTime = run.endTime();

    TS_ASSERT_EQUALS(endTime.totalNanoseconds(), lastTime.totalNanoseconds());
  }

  void test_load_file_with_invalid_log_entries() {
    LoadNexusLogs ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "ENGINX00228061_log_alarm_data.nxs");
    MatrixWorkspace_sptr ws = createTestWorkspace();
    // Put it in the object.
    ld.setProperty("Workspace", ws);
    ld.execute();
    TS_ASSERT(ld.isExecuted());

    auto run = ws->run();

    // This one should not be present as there is no invalid data
    TS_ASSERT_THROWS_ANYTHING(run.getLogData(LogManager::getInvalidValuesFilterLogName("slitpos")));

    // This one should not be present as there is no invalid data
    TS_ASSERT_THROWS_ANYTHING(run.getLogData(LogManager::getInvalidValuesFilterLogName("cryo_Sample")));

    // these two both contain invalid data
    auto pclog1 = dynamic_cast<TimeSeriesProperty<bool> *>(
        run.getLogData(LogManager::getInvalidValuesFilterLogName("cryo_temp1")));
    TS_ASSERT_EQUALS(pclog1->size(), 3);
    TS_ASSERT_EQUALS(pclog1->nthValue(0), true);
    TS_ASSERT_EQUALS(pclog1->nthValue(1), false);
    TS_ASSERT_EQUALS(pclog1->nthValue(2), true);

    auto pclog2 = dynamic_cast<TimeSeriesProperty<bool> *>(
        run.getLogData(LogManager::getInvalidValuesFilterLogName("cryo_temp2")));
    TS_ASSERT_EQUALS(pclog2->size(), 3);
    TS_ASSERT_EQUALS(pclog2->nthValue(0), false);
    TS_ASSERT_EQUALS(pclog2->nthValue(1), false);
    TS_ASSERT_EQUALS(pclog2->nthValue(2), false);

    // force the filtering by passing in a log filter
    auto timeSeries = new Mantid::Kernel::TimeSeriesProperty<bool>("filter");
    timeSeries->addValue("2007-11-30T16:16:50", true);
    timeSeries->addValue("2007-11-30T16:17:25", false);
    timeSeries->addValue("2007-11-30T16:17:39", true);
    auto filter = std::make_unique<LogFilter>(*timeSeries);
    run.filterByLog(filter.get());

    auto pclogFiltered1 = dynamic_cast<TimeSeriesProperty<double> *>(run.getLogData("cryo_temp1"));
    // middle value is invalid and is filtered out
    TS_ASSERT_EQUALS(pclogFiltered1->size(), 2);
    TS_ASSERT_DELTA(pclogFiltered1->nthValue(0), 3, 1e-5);
    TS_ASSERT_DELTA(pclogFiltered1->nthValue(1), 7, 1e-5);

    auto pclogFiltered2 = dynamic_cast<TimeSeriesProperty<double> *>(run.getLogData("cryo_temp2"));
    std::vector<double> correctFiltered2{3., 5., 7.};
    // Here the entire log is filtered out
    // Our filtering in this case does not filter anything.
    // This seems stringe, but actually may be what people want,
    // It also resolves the question of what we should do with an entirely
    // invalid log.
    TS_ASSERT_EQUALS(pclogFiltered2->size(), 3);
    TS_ASSERT_DELTA(pclogFiltered2->nthValue(0), 3, 1e-5);
    TS_ASSERT_DELTA(pclogFiltered2->nthValue(1), 5, 1e-5);
    TS_ASSERT_DELTA(pclogFiltered2->nthValue(2), 7, 1e-5);
  }

  void test_allow_list() {
    auto testWS = createTestWorkspace();

    std::vector<std::string> allowed = {"proton_charge", "S2HGap", "S2VGap"};

    LoadNexusLogs loader;
    loader.setChild(true);
    loader.initialize();
    loader.setProperty("Workspace", testWS);
    loader.setPropertyValue("Filename", "LARMOR00003368.nxs");
    loader.setProperty<std::vector<std::string>>("AllowList", allowed);
    loader.setPropertyValue("BlockList", "");
    loader.execute();
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    // selog versions
    allowed.push_back("selog_S2HGap");
    allowed.push_back("selog_S2VGap");

    // extra proton charge properties
    allowed.push_back("gd_prtn_chrg");
    allowed.push_back("proton_charge_by_period");
    allowed.push_back("nperiods");
    allowed.push_back("gd_prtn_chrg_unfiltered");

    // The default logs that are always present:
    allowed.push_back("start_time");
    allowed.push_back("end_time");
    allowed.push_back("run_title");

    auto run = testWS->run();
    auto properties = run.getProperties();

    TS_ASSERT_EQUALS(properties.size(), allowed.size());

    // Lookup each name in the workspace property list
    for (const auto &name : allowed) {
      bool found = false;
      for (const auto &prop : properties) {
        if (prop->name() == name) {
          found = true;
          break;
        }
      }
      TS_ASSERT_EQUALS(found, true);
      if (!found) {
        break;
      }
    }
  }

  void test_block_list() {
    auto testWS = createTestWorkspace();

    std::vector<std::string> blocked = {"proton_charge", "S2HGap", "S2VGap"};
    std::vector<std::string> blocked_pattern = {"proton_charge", "S2?Gap"};

    LoadNexusLogs loader;
    loader.setChild(true);
    loader.initialize();
    loader.setProperty("Workspace", testWS);
    loader.setPropertyValue("Filename", "LARMOR00003368.nxs");
    loader.setPropertyValue("AllowList", "");
    loader.setProperty<std::vector<std::string>>("BlockList", blocked_pattern);
    loader.execute();
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    auto run = testWS->run();
    auto properties = run.getProperties();

    // add 2 to account for selog_ versions of properties
    TS_ASSERT_EQUALS(properties.size(), 95 - blocked.size() - 2);

    // Lookup each name in the workspace property list
    for (const auto &name : blocked) {
      bool found = false;
      for (const auto &prop : properties) {
        if (prop->name() == name) {
          found = true;
          break;
        }
      }
      TS_ASSERT_EQUALS(found, false);
      if (found) {
        break;
      }
    }
  }

  void test_allow_and_block_list() {
    auto testWS = createTestWorkspace();

    LoadNexusLogs loader;
    loader.setChild(true);
    loader.initialize();
    loader.setProperty("Workspace", testWS);
    loader.setPropertyValue("Filename", "LARMOR00003368.nxs");
    loader.setPropertyValue("AllowList", ""); // Specify nothing for either
    loader.setPropertyValue("BlockList", ""); // To ensure logs load as expected
    loader.execute();
    TS_ASSERT_THROWS_NOTHING(loader.execute());

    auto run = testWS->run();
    auto properties = run.getProperties();

    TS_ASSERT_EQUALS(properties.size(), 95);
  }

private:
  API::MatrixWorkspace_sptr createTestWorkspace() {
    return WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
  }
};
