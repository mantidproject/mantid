// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/CorelliPowderCalibrationDatabase.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSeriesProperty.h"

using Mantid::Algorithms::CorelliPowderCalibrationDatabase;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using Mantid::Types::Core::DateAndTime;
using Mantid::Types::Event::TofEvent;
using namespace Mantid::Algorithms;

class CorelliPowderCalibrationDatabaseTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CorelliPowderCalibrationDatabaseTest *createSuite() {
    return new CorelliPowderCalibrationDatabaseTest();
  }
  static void destroySuite(CorelliPowderCalibrationDatabaseTest *suite) {
    delete suite;
  }

  void test_Init() {
    CorelliPowderCalibrationDatabase alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  // Test the method to create date stamp YYYYMMDD
  void test_createDateStamp() {
    // Create the algorithm
    CorelliPowderCalibrationDatabase alg;

    // Do the parsing
    // std::string datestampe = alg.convertRunStartToDateStamp();
    // TS_ASSERT_
  }

  void template_test_exec() {
    // Name of the output workspace.
    std::string outWSName("CorelliPowderCalibrationDatabaseTest_OutputWS");

    IAlgorithm_sptr lei =
        AlgorithmFactory::Instance().create("LoadEmptyInstrument", 1);
    lei->initialize();
    lei->setPropertyValue("Filename", "CORELLI_Definition.xml");
    lei->setPropertyValue("OutputWorkspace",
                          "CorelliPowderCalibrationDatabaseTest_OutputWS");
    lei->setPropertyValue("MakeEventWorkspace", "1");
    lei->execute();

    EventWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
        "CorelliPowderCalibrationDatabaseTest_OutputWS");

    DateAndTime startTime("2007-11-30T16:17:00");
    auto &evlist = ws->getSpectrum(0);

    // Add some events to the workspace.
    evlist.addEventQuickly(TofEvent(10.0, startTime + 0.007));
    evlist.addEventQuickly(TofEvent(100.0, startTime + 0.012));
    evlist.addEventQuickly(TofEvent(1000.0, startTime + 0.012));
    evlist.addEventQuickly(TofEvent(10000.0, startTime + 0.012));
    evlist.addEventQuickly(TofEvent(1222.0, startTime + 0.03));

    ws->getAxis(0)->setUnit("TOF");

    ws->sortAll(PULSETIME_SORT, nullptr);

    // Add some chopper TDCs to the workspace.
    double period = 1 / 293.383;
    auto tdc = new TimeSeriesProperty<int>("chopper4_TDC");
    for (int i = 0; i < 10; i++) {
      double tdcTime = i * period;
      tdc->addValue(startTime + tdcTime, 1);
    }
    ws->mutableRun().addLogData(tdc);

    // Add motorSpeed to the workspace
    auto motorSpeed =
        new TimeSeriesProperty<double>("BL9:Chop:Skf4:MotorSpeed");
    motorSpeed->addValue(startTime, 293.383);
    ws->mutableRun().addLogData(motorSpeed);

    CorelliPowderCalibrationDatabase alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "InputWorkspace", "CorelliPowderCalibrationDatabaseTest_OutputWS"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "OutputWorkspace", "CorelliPowderCalibrationDatabaseTest_OutputWS"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("TimingOffset", "20000"));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            "CorelliPowderCalibrationDatabaseTest_OutputWS"));
    TS_ASSERT(ws);
    if (!ws)
      return;

    std::vector<WeightedEvent> &events = evlist.getWeightedEvents();

    TS_ASSERT_DELTA(events[0].weight(), -0.993919, 0.00001)
    TS_ASSERT_DELTA(events[1].weight(), -0.993919, 0.00001)
    TS_ASSERT_DELTA(events[2].weight(), 1.0, 0.00001)
    TS_ASSERT_DELTA(events[3].weight(), -0.993919, 0.00001)
    TS_ASSERT_DELTA(events[4].weight(), 1.0, 0.00001)

    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_exec() {

    // Create workspaces
    EventWorkspace_sptr input_ws = createTestEventWorkspace();
    // Name of the output calibration workspace
    std::string outwsname("CorelliPowderCalibrationDatabaseTest_TableWS");
    TableWorkspace_sptr calib_ws =
        createTestCalibrationTableWorkspace(outwsname);
    TS_ASSERT(input_ws);
    TS_ASSERT(calib_ws);

    // Init algorithm
    CorelliPowderCalibrationDatabase alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    // Set up
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", input_ws));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputCalibrationPatchWorkspace", calib_ws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("DatabaseDirectory", "/tmp/"));

    // Execute
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Clean memory
  }

  /**
   * @brief Test algorithm to convert datetime string to date stamp
   */
  void test_timestamp_conversion() {
    std::string yyyymmdd = CorelliPowderCalibrationDatabase::convertTimeStamp(
        "2018-02-20T12:57:17");
    TS_ASSERT_EQUALS(yyyymmdd, "20180220");
  }

  void test_calibration_workspace_handler() {

    std::string outwsname("CorelliPowderCalibrationDatabaseTest_TableWS2");
    TableWorkspace_sptr calib_ws =
        createTestCalibrationTableWorkspace(outwsname);

    CorelliCalibration::CalibrationTableHandler calib_handler =
        CorelliCalibration::CalibrationTableHandler();
    calib_handler.setCalibrationTable(calib_ws);

    // name
    std::vector<std::string> compNames = calib_handler.getComponentNames();
    TS_ASSERT_EQUALS(compNames.size(), 3);

    calib_handler.saveCompomentDatabase(compNames[0]);
  }

private:
  /**
   * @brief Create testing CORELLI event workspace
   * @return
   */
  EventWorkspace_sptr createTestEventWorkspace() {
    // Name of the output workspace.
    std::string outWSName("CorelliPowderCalibrationDatabaseTest_matrixWS");

    IAlgorithm_sptr lei =
        AlgorithmFactory::Instance().create("LoadEmptyInstrument", 1);
    lei->initialize();
    lei->setPropertyValue("Filename", "CORELLI_Definition.xml");
    lei->setPropertyValue("OutputWorkspace",
                          "CorelliPowderCalibrationDatabaseTest_OutputWS");
    lei->setPropertyValue("MakeEventWorkspace", "1");
    lei->execute();

    EventWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
        "CorelliPowderCalibrationDatabaseTest_OutputWS");

    // Add property start_time
    ws->mutableRun().addProperty<std::string>("start_time",
                                              "2018-02-20T12:57:17", "", true);

    return ws;
  }

  /**
   * @brief Create Test Calibration TableWorkspace
   * @param outWSName
   * @return
   */
  TableWorkspace_sptr
  createTestCalibrationTableWorkspace(std::string outWSName) {

    ITableWorkspace_sptr itablews = WorkspaceFactory::Instance().createTable();
    AnalysisDataService::Instance().addOrReplace(outWSName, itablews);

    TableWorkspace_sptr tablews =
        std::dynamic_pointer_cast<TableWorkspace>(itablews);
    TS_ASSERT(tablews);

    // Set up columns
    for (size_t i = 0;
         i < CorelliCalibration::calibrationTableColumnNames.size(); ++i) {
      std::string colname = CorelliCalibration::calibrationTableColumnNames[i];
      std::string type = CorelliCalibration::calibrationTableColumnTypes[i];
      tablews->addColumn(type, colname);
    }

    // append rows
    Mantid::API::TableRow sourceRow = tablews->appendRow();
    sourceRow << "source" << 0. << 0. << -15.560 << 0. << 0. << 0. << 0.;
    Mantid::API::TableRow sampleRow = tablews->appendRow();
    sampleRow << "sample" << 0.0001 << -0.0002 << 0.003 << 0. << 0. << 0. << 0.;
    Mantid::API::TableRow bank1Row = tablews->appendRow();
    bank1Row << "bank1" << 0.9678 << 0.0056 << 0.0003 << 0.4563 << -0.9999
             << 0.3424 << 5.67;

    return tablews;
  }

  TableWorkspace_sptr createIncorrectTestCalibrationTableWorkspace() {
    // Name of the output calibration workspace
    std::string outWSName(
        "CorelliPowderCalibrationDatabaseTest_IncorrectTableWS");

    ITableWorkspace_sptr itablews = WorkspaceFactory::Instance().createTable();
    AnalysisDataService::Instance().addOrReplace(outWSName, itablews);

    TableWorkspace_sptr tablews =
        std::dynamic_pointer_cast<TableWorkspace>(itablews);
    TS_ASSERT(tablews);

    // Set up columns
    for (size_t i = 0;
         i < CorelliCalibration::calibrationTableColumnNames.size(); ++i) {
      std::string colname = CorelliCalibration::calibrationTableColumnNames[i];
      std::string type = CorelliCalibration::calibrationTableColumnTypes[i];
      tablews->addColumn(type, colname);
    }

    // append rows
    Mantid::API::TableRow sourceRow = tablews->appendRow();
    sourceRow << "source" << 0. << 0. << -15.560 << 0. << 0. << 0. << 0.;
    Mantid::API::TableRow sampleRow = tablews->appendRow();
    sampleRow << "sample" << 0.0001 << -0.0002 << 0.003 << 0. << 0. << 0. << 0.;
    Mantid::API::TableRow bank1Row = tablews->appendRow();
    bank1Row << "bank1" << 0.9678 << 0.0056 << 0.0003 << 0.4563 << -0.9999
             << 0.3424 << 5.67;

    return tablews;
  }
};
