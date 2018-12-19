// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef LOADNEXUSLOGSTEST_H_
#define LOADNEXUSLOGSTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadNexusLogs.h"
#include "MantidDataObjects/Workspace2D.h"
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
    TS_ASSERT_DELTA(Types::Core::DateAndTime::secondsFromDuration(
                        tsp->nthInterval(0).length()),
                    0.328, 0.01);

    // Now the stats
  }

  void test_File_With_Runlog_And_Selog() {
    LoadNexusLogs loader;
    loader.initialize();
    MatrixWorkspace_sptr testWS = createTestWorkspace();
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Workspace", testWS));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("Filename", "LOQ49886.nxs"));
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    const API::Run &run = testWS->run();
    const std::vector<Property *> &logs = run.getLogData();
    TS_ASSERT_EQUALS(logs.size(),
                     36); // 34 logs in file + 1 synthetic nperiods log
                          // + 1 proton_charge_by_period log

    TimeSeriesProperty<std::string> *slog =
        dynamic_cast<TimeSeriesProperty<std::string> *>(
            run.getLogData("icp_event"));
    TS_ASSERT(slog);
    std::string str = slog->value();
    TS_ASSERT_EQUALS(str.size(), 1023);
    TS_ASSERT_EQUALS(str.substr(0, 37),
                     "2009-Apr-28 09:20:29  CHANGE_PERIOD 1");

    slog = dynamic_cast<TimeSeriesProperty<std::string> *>(
        run.getLogData("icp_debug"));
    TS_ASSERT(slog);
    TS_ASSERT_EQUALS(slog->size(), 50);

    TimeSeriesProperty<int> *ilog =
        dynamic_cast<TimeSeriesProperty<int> *>(run.getLogData("total_counts"));
    TS_ASSERT(ilog);
    TS_ASSERT_EQUALS(ilog->size(), 172);

    ilog = dynamic_cast<TimeSeriesProperty<int> *>(run.getLogData("period"));
    TS_ASSERT(ilog);
    TS_ASSERT_EQUALS(ilog->size(), 172);

    TimeSeriesProperty<double> *dlog =
        dynamic_cast<TimeSeriesProperty<double> *>(
            run.getLogData("proton_charge"));
    TS_ASSERT(dlog);
    TS_ASSERT_EQUALS(dlog->size(), 172);
  }

  void test_File_With_Bad_Property() {
    LoadNexusLogs loader;
    loader.initialize();
    MatrixWorkspace_sptr testWS = createTestWorkspace();
    TS_ASSERT_THROWS_NOTHING(loader.setProperty("Workspace", testWS));
    TS_ASSERT_THROWS_NOTHING(
        loader.setPropertyValue("Filename", "IMAT00003680.nxs"));
    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT(loader.isExecuted());

    const API::Run &run = testWS->run();

    TimeSeriesProperty<std::string> *putLog =
        dynamic_cast<TimeSeriesProperty<std::string> *>(
            run.getLogData("EPICS_PUTLOG"));
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
    TSM_ASSERT("Should not have nperiods until we run LoadNexusLogs",
               !run.hasProperty("nperiods"));
    LoadNexusLogs loader;

    loader.setChild(true);
    loader.initialize();
    loader.setProperty("Workspace", testWS);
    loader.setPropertyValue("Filename", "LARMOR00003368.nxs");
    loader.execute();
    run = testWS->run();

    const bool hasNPeriods = run.hasProperty("nperiods");
    TSM_ASSERT("Should have nperiods now we have run LoadNexusLogs",
               hasNPeriods);
    if (hasNPeriods) {
      const int nPeriods = run.getPropertyValueAsType<int>("nperiods");
      TSM_ASSERT_EQUALS("Wrong number of periods extracted", nPeriods, 4);
    }
  }

  void test_extract_periods_log_from_event_nexus() {

    auto testWS = createTestWorkspace();
    auto run = testWS->run();
    TSM_ASSERT("Should not have nperiods until we run LoadNexusLogs",
               !run.hasProperty("nperiods"));
    LoadNexusLogs loader;

    loader.setChild(true);
    loader.initialize();
    loader.setProperty("Workspace", testWS);
    loader.setPropertyValue("Filename", "LARMOR00003368.nxs");
    loader.execute();
    run = testWS->run();

    const bool hasPeriods = run.hasProperty("period_log");
    TSM_ASSERT("Should have period_log now we have run LoadNexusLogs",
               hasPeriods);

    auto *temp = run.getProperty("period_log");
    auto *periodLog = dynamic_cast<TimeSeriesProperty<int> *>(temp);
    TSM_ASSERT("Period log should be an int time series property", periodLog);

    std::vector<int> periodValues = periodLog->valuesAsVector();
    std::unordered_set<int> uniquePeriods(periodValues.begin(),
                                          periodValues.end());
    TSM_ASSERT_EQUALS("Should have 4 periods in total", 4,
                      uniquePeriods.size());

    std::vector<double> protonChargeByPeriod =
        run.getPropertyValueAsType<std::vector<double>>(
            "proton_charge_by_period");
    TSM_ASSERT_EQUALS("Should have four proton charge entries", 4,
                      protonChargeByPeriod.size());
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
    TSM_ASSERT_EQUALS("Run title is not correct",
                      "3He polariser test 0.9bar Long Polariser 0.75A", title);
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
    TimeSeriesProperty<double> *pclog =
        dynamic_cast<TimeSeriesProperty<double> *>(
            run.getLogData("proton_charge"));
    TS_ASSERT(pclog);
    TS_ASSERT_EQUALS(pclog->size(), 23806);
    TS_ASSERT(pclog->getStatistics().duration > 4e9);

    // 3rd entry On-Off
    testWS = createTestWorkspace();
    loader.setChild(true);
    loader.initialize();
    loader.setProperty("Workspace", testWS);
    loader.setPropertyValue("Filename", "REF_M_9709_event.nxs");
    loader.setProperty("NXentryName", "entry-On_Off");
    loader.execute();
    run = testWS->run();
    pclog = dynamic_cast<TimeSeriesProperty<double> *>(
        run.getLogData("proton_charge"));
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
    TS_ASSERT_THROWS_NOTHING(loader.execute())
  }

  void test_last_time_series_log_entry_equals_end_time() {
    LoadNexusLogs ld;
    std::string outws_name = "REF_L_instrument";
    ld.initialize();
    ld.setPropertyValue("Filename", "REF_L_32035.nxs");
    MatrixWorkspace_sptr ws = createTestWorkspace();
    // Put it in the object.
    ld.setProperty("Workspace", ws);
    ld.execute();
    TS_ASSERT(ld.isExecuted());

    auto run = ws->run();
    auto pclog = dynamic_cast<TimeSeriesProperty<double> *>(
        run.getLogData("PhaseRequest1"));

    TS_ASSERT(pclog);

    const auto lastTime = pclog->lastTime();
    const auto endTime = run.endTime();

    TS_ASSERT_EQUALS(endTime.totalNanoseconds(), lastTime.totalNanoseconds());
  }

private:
  API::MatrixWorkspace_sptr createTestWorkspace() {
    return WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
  }
};

#endif /* LOADNEXUSLOGS_H_*/
