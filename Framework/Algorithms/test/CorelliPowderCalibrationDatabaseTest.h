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
#include <boost/filesystem.hpp>

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

  //-----------------------------------------------------------------------------
  /**
   * @brief Test algorithm to convert datetime string to date stamp
   */
  void test_timestamp_conversion() {
    std::string yyyymmdd = CorelliPowderCalibrationDatabase::convertTimeStamp(
        "2018-02-20T12:57:17");
    TS_ASSERT_EQUALS(yyyymmdd, "20180220");
  }

  void test_file_io() {
    // create directory
    std::string test_dir{"TestCorelliPowderCalibrationX"};
    boost::filesystem::create_directory(test_dir);
    TS_ASSERT(boost::filesystem::is_directory(test_dir));

    // clean
    boost::filesystem::remove_all(test_dir);
  }

  //-----------------------------------------------------------------------------
  /**
   * @brief Test ComponentPosition
   */
  void test_component() {
      // compare:
      CorelliCalibration::ComponentPosition pos1{0., 0., 0., 20., 30., 40., 50};
      CorelliCalibration::ComponentPosition pos2{0., 0., 0., 20., 30., 40., 50};
      CorelliCalibration::ComponentPosition pos3{0., 0., 0., 20.003, 30., 40., 50};
      TS_ASSERT(pos1.equalTo(pos2, 1E-7));
      TS_ASSERT(!pos1.equalTo(pos3, 1E-7));
  }

  //-----------------------------------------------------------------------------
  /**
   * @brief Test CalibrationWworkspaceHandler
   */
  void test_calibration_workspace_handler() {
    // Create a correct calibration worksapce
    std::string outwsname("CorelliPowderCalibrationDatabaseTest_TableWS2");
    TableWorkspace_sptr calib_ws =
        createTestCalibrationTableWorkspace(outwsname);
    std::cout << "[DEBUG 2] Table workspace rows: " << calib_ws->rowCount() << "\n";

    // Create an incorrect calibration workspace
    std::string wrongwsname{"CorelliPowderCalibrationDatabaseTest_TableWS_Wrong"};
    TableWorkspace_sptr calib_wrong_ws =
        createIncorrectTestCalibrationTableWorkspace(wrongwsname);

    // Init CalibrationTableHandler instance
    CorelliCalibration::CalibrationTableHandler calib_handler =
        CorelliCalibration::CalibrationTableHandler();

    // Expect to fail set a wrong
    TS_ASSERT_THROWS_ANYTHING(calib_handler.setCalibrationTable(calib_wrong_ws));
    // Shall not throw
    TS_ASSERT_THROWS_NOTHING(calib_handler.setCalibrationTable(calib_ws));

    // Test method to retrieve components names (rows)
    std::vector<std::string> componentnames = calib_handler.getComponentNames();
    std::vector<std::string> expectednames{"source", "sample", "bank1"};
    TS_ASSERT_EQUALS(componentnames.size(), expectednames.size());
    for (size_t i = 0; i < 3; ++i)
        TS_ASSERT_EQUALS(componentnames[i], expectednames[i]);

    // name
    std::vector<std::string> compNames = calib_handler.getComponentNames();
    TS_ASSERT_EQUALS(compNames.size(), 3);

    // Test save
    // component file: name, remove file if it does exist, save and check file existence
    const std::string testcompfilename{"/tmp/testsourcedb2.csv"};
    boost::filesystem::remove(testcompfilename);
    calib_handler.saveCalibrationTable(testcompfilename);
    TS_ASSERT(boost::filesystem::exists(testcompfilename));

    // Load and check
    TableWorkspace_sptr duptable = loadCSVtoTable(testcompfilename, "DuplicatedSource");
    TS_ASSERT_EQUALS(duptable->rowCount(), 1);

    // load back
    calib_handler.load(testcompfilename);
    TableWorkspace_sptr compcalibws = calib_handler.getCalibrationWorkspace();

    return;

    const std::string testcalibfilename("testtable2.csv");
    boost::filesystem::remove(testcalibfilename);
    calib_handler.saveCalibrationTable(testcalibfilename);
    TS_ASSERT(boost::filesystem::exists(testcalibfilename));



    // Clean
    AnalysisDataService::Instance().remove(outwsname);
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
  createTestCalibrationTableWorkspace(const std::string &outWSName) {

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

    std::cout << "[DEBUG 0] Table workspace rows: " << tablews->rowCount() << "\n";

    // append rows
    Mantid::API::TableRow sourceRow = tablews->appendRow();
    sourceRow << "source" << 0. << 0. << -15.560 << 0. << 0. << 0. << 0.;
    Mantid::API::TableRow sampleRow = tablews->appendRow();
    sampleRow << "sample" << 0.0001 << -0.0002 << 0.003 << 0. << 0. << 0. << 0.;
    Mantid::API::TableRow bank1Row = tablews->appendRow();
    bank1Row << "bank1" << 0.9678 << 0.0056 << 0.0003 << 0.4563 << -0.9999
             << 0.3424 << 5.67;

    std::cout << "[DEBUG 1] Table workspace rows: " << tablews->rowCount() << "\n";

    return tablews;
  }

  /**
   * @brief Create an incompatile table workspace for algorithm to throw exception
   * @param outWSName
   * @return
   */
  TableWorkspace_sptr createIncorrectTestCalibrationTableWorkspace(const std::string &outWSName) {
    // Create table workspace
    ITableWorkspace_sptr itablews = WorkspaceFactory::Instance().createTable();
    AnalysisDataService::Instance().addOrReplace(outWSName, itablews);

    TableWorkspace_sptr tablews =
        std::dynamic_pointer_cast<TableWorkspace>(itablews);
    TS_ASSERT(tablews);

    // Set up columns
    for (size_t i = 0;
         i < CorelliCalibration::calibrationTableColumnNames.size() - 1; ++i) {
      std::string colname = CorelliCalibration::calibrationTableColumnNames[i];
      std::string type = CorelliCalibration::calibrationTableColumnTypes[i];
      tablews->addColumn(type, colname);
    }

    // append rows
    Mantid::API::TableRow sourceRow = tablews->appendRow();
    sourceRow << "source" << 0. << 0. << -15.560 << 0. << 0. << 0. ;
    Mantid::API::TableRow sampleRow = tablews->appendRow();
    sampleRow << "sample" << 0.0001 << -0.0002 << 0.003 << 0. << 0. << 0. ;
    Mantid::API::TableRow bank1Row = tablews->appendRow();
    bank1Row << "bank1" << 0.9678 << 0.0056 << 0.0003 << 0.4563 << -0.9999
             << 0.3424 ;

    return tablews;
  }

  TableWorkspace_sptr loadCSVtoTable(const std::string &csvname, const std::string &tablewsname) {
      IAlgorithm_sptr loadAsciiAlg =
          AlgorithmFactory::Instance().create("LoadAscii", 2);
      loadAsciiAlg->initialize();
      loadAsciiAlg->setPropertyValue("Filename", csvname);
      loadAsciiAlg->setPropertyValue("OutputWorkspace", tablewsname);
      loadAsciiAlg->setPropertyValue("Separator", "CSV");
      loadAsciiAlg->setPropertyValue("CommentIndicator", "#");
      loadAsciiAlg->execute();

      TableWorkspace_sptr tablews = std::dynamic_pointer_cast<TableWorkspace>(AnalysisDataService::Instance().retrieve(tablewsname));

      return tablews;
  }

};
