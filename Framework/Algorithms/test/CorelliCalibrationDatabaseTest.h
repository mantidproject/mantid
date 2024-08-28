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
#include "MantidAlgorithms/CorelliCalibrationDatabase.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <filesystem>
#include <fstream>

using Mantid::Algorithms::CorelliCalibrationDatabase;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using Mantid::Types::Core::DateAndTime;
using Mantid::Types::Event::TofEvent;
using namespace Mantid::Algorithms;

class CorelliCalibrationDatabaseTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CorelliCalibrationDatabaseTest *createSuite() { return new CorelliCalibrationDatabaseTest(); }
  static void destroySuite(CorelliCalibrationDatabaseTest *suite) { delete suite; }

  //-----------------------------------------------------------------------------
  void test_Init() {
    CorelliCalibrationDatabase alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  //-----------------------------------------------------------------------------
  /**
   * @brief Test basic file IO library methods
   */
  void test_file_io() {
    // create directory
    std::string test_dir{"TestCorelliCalibrationX"};
    std::filesystem::create_directory(test_dir);
    TS_ASSERT(std::filesystem::is_directory(test_dir));

    // clean
    std::filesystem::remove_all(test_dir);
  }

  //-----------------------------------------------------------------------------
  /**
   * @brief Test algorithm to convert datetime string to date stamp
   */
  void test_timestamp_conversion() {
    std::string yyyymmdd = CorelliCalibrationDatabase::convertTimeStamp("2018-02-20T12:57:17");
    TS_ASSERT_EQUALS(yyyymmdd, "20180220");
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
    std::string outwsname("CorelliCalibrationDatabaseTest_TableWS2");
    TableWorkspace_sptr calib_ws = createTestCalibrationTableWorkspace(outwsname);

    // Create an incorrect calibration workspace
    std::string wrongwsname{"CorelliCalibrationDatabaseTest_TableWS_Wrong"};
    TableWorkspace_sptr calib_wrong_ws = createIncorrectTestCalibrationTableWorkspace(wrongwsname);

    // Init CalibrationTableHandler instance
    CorelliCalibration::CalibrationTableHandler calib_handler = CorelliCalibration::CalibrationTableHandler();

    // Expect to fail set a wrong
    TS_ASSERT_THROWS_ANYTHING(calib_handler.setCalibrationTable(calib_wrong_ws));
    // Shall not throw
    TS_ASSERT_THROWS_NOTHING(calib_handler.setCalibrationTable(calib_ws));

    // Test method to retrieve components names (rows)
    std::vector<std::string> componentnames = calib_handler.getComponentNames();
    std::vector<std::string> expectednames{"moderator", "sample-position", "bank1/sixteenpack"};
    TS_ASSERT_EQUALS(componentnames.size(), expectednames.size());
    for (size_t i = 0; i < 3; ++i)
      TS_ASSERT_EQUALS(componentnames[i], expectednames[i]);

    // Test: get component names
    std::vector<std::string> compNames = calib_handler.getComponentNames();
    TS_ASSERT_EQUALS(compNames.size(), 3);

    // Test: get component calibrated positions
    CorelliCalibration::ComponentPosition goldsourcepos{0., 0., -15.560, 0., 0., 0., 0.};
    CorelliCalibration::ComponentPosition testsourcepos = calib_handler.getComponentCalibratedPosition("moderator");
    TS_ASSERT(testsourcepos.equalTo(goldsourcepos, 1.E-10));

    CorelliCalibration::ComponentPosition goldbank1pos{0.9678, 0.0056, 0.0003, 0.4563, -0.9999, 0.3424, 5.67};
    CorelliCalibration::ComponentPosition testbank1pos =
        calib_handler.getComponentCalibratedPosition("bank1/sixteenpack");
    TS_ASSERT(testbank1pos.equalTo(goldbank1pos, 1.E-10));

    // Test: save calibration table
    // component file: name, remove file if it does exist, save and check file
    // existence
    const std::string testcalibtablefilename{"/tmp/testsourcedb2.csv"};
    std::filesystem::remove(testcalibtablefilename);
    calib_handler.saveCalibrationTable(testcalibtablefilename);
    TS_ASSERT(std::filesystem::exists(testcalibtablefilename));
    // load file and check
    TableWorkspace_sptr duptable = loadCSVtoTable(testcalibtablefilename, "DuplicatedSource");
    TS_ASSERT_EQUALS(duptable->rowCount(), 3);
    TS_ASSERT_DELTA(duptable->cell<double>(2, 6), 0.3424, 0.00001);

    // Test: save single component file
    const std::string testsamplecalfilename{"/tmp/testsampledb2.csv"};
    std::filesystem::remove(testsamplecalfilename);
    // save
    calib_handler.saveCompomentDatabase("20201117", "sample-position", testsamplecalfilename);
    TS_ASSERT(std::filesystem::exists(testsamplecalfilename));

    // load
    TableWorkspace_sptr dupsampletable =
        calib_handler.loadComponentCalibrationTable(testsamplecalfilename, "TestSampleCalib1");
    // check row number and value
    TS_ASSERT_EQUALS(dupsampletable->rowCount(), 1);
    TS_ASSERT_DELTA(dupsampletable->cell<double>(0, 2), -0.0002, 0.000001);

    // Clean
    AnalysisDataService::Instance().remove(outwsname);
  }

  //-----------------------------------------------------------------------------
  /**
   * @brief Test main features required by the algorithm
   * CorelliCalibrationDatabase
   */
  void test_exec() {

    // Create the test environment
    // create directory database
    std::string calibdir{"/tmp/TestCorelliCalibration1117"};
    // clean previous
    std::filesystem::remove_all(calibdir);
    // create data base
    std::filesystem::create_directory(calibdir);
    // create a previously generated database file
    // will create the following files:
    // moderator.csv, sample-position.csv, bank2.csv, bank42.csv
    std::vector<std::string> banks{"moderator", "sample-position", "bank2", "bank42"};
    create_existing_database_files(calibdir, banks);

    // Create workspaces
    EventWorkspace_sptr input_ws = createTestEventWorkspace();
    // Name of the output calibration workspace
    std::string outwsname("CorelliCalibrationDatabaseTest_CombinedTableWS");
    TableWorkspace_sptr calib_ws = createTestCalibrationTableWorkspace(outwsname);
    TS_ASSERT(input_ws);
    TS_ASSERT(calib_ws);

    // Init algorithm
    CorelliCalibrationDatabase alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    // Set up
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", input_ws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputCalibrationPatchWorkspace", calib_ws));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("DatabaseDirectory", calibdir));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", outwsname));

    // Execute
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    // Verifying results
    // Output 3: the combined calibration workspace
    TS_ASSERT(AnalysisDataService::Instance().doesExist(outwsname));
    TableWorkspace_sptr combinedcalibws =
        std::dynamic_pointer_cast<TableWorkspace>(AnalysisDataService::Instance().retrieve(outwsname));
    TS_ASSERT(combinedcalibws);
    // shall be 5 components
    TS_ASSERT_EQUALS(combinedcalibws->rowCount(), 5);
    TS_ASSERT_EQUALS(combinedcalibws->cell<std::string>(1, 0), "sample-position");
    TS_ASSERT_EQUALS(combinedcalibws->cell<std::string>(2, 0), "bank1/sixteenpack");
    TS_ASSERT_EQUALS(combinedcalibws->cell<std::string>(4, 0), "bank42/sixteenpack");

    // Output 2: search the saved output calibration file
    std::filesystem::path pdir(calibdir);
    std::filesystem::path pbase("corelli_instrument_20201117.csv");
    std::filesystem::path ptodaycalfile = pdir / pbase;
    std::string todaycalfile = ptodaycalfile.string();
    TS_ASSERT(std::filesystem::exists(todaycalfile));
    // load and compare
    // ... ...

    // Output 1: check all the files
    std::vector<std::string> compnames{"moderator", "sample-position", "bank1", "bank2", "bank42"};
    std::vector<size_t> expectedrows{2, 2, 1, 1, 1};
    for (size_t i = 0; i < 5; ++i) {
      verify_component_files(calibdir, compnames[i], expectedrows[i]);
    }

    // Clean memory
    // Remove workspace from the data service.
    // AnalysisDataService::Instance().remove(outWSName);
  }

private:
  /**
   * @brief Create testing CORELLI event workspace
   * @return
   */
  EventWorkspace_sptr createTestEventWorkspace() {
    // Name of the output workspace.
    std::string outWSName("CorelliCalibrationDatabaseTest_matrixWS");

    auto lei = AlgorithmFactory::Instance().create("LoadEmptyInstrument", 1);
    lei->initialize();
    lei->setPropertyValue("Filename", "CORELLI_Definition.xml");
    lei->setPropertyValue("OutputWorkspace", "CorelliCalibrationDatabaseTest_OutputWS");
    lei->setPropertyValue("MakeEventWorkspace", "1");
    lei->execute();

    EventWorkspace_sptr ws;
    ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("CorelliCalibrationDatabaseTest_OutputWS");

    // Add property start_time
    ws->mutableRun().addProperty<std::string>("start_time", "2020-11-17T12:57:17", "", true);

    return ws;
  }

  /**
   * @brief Create Test Calibration TableWorkspace
   *
   * @details this table would be the output of CorelliPowderCalibrationCreate
   *
   * @param outWSName
   * @return
   */
  TableWorkspace_sptr createTestCalibrationTableWorkspace(const std::string &outWSName) {

    ITableWorkspace_sptr itablews = WorkspaceFactory::Instance().createTable();
    AnalysisDataService::Instance().addOrReplace(outWSName, itablews);

    TableWorkspace_sptr tablews = std::dynamic_pointer_cast<TableWorkspace>(itablews);
    TS_ASSERT(tablews);

    // Set up columns
    for (size_t i = 0; i < CorelliCalibration::calibrationTableColumnNames.size(); ++i) {
      std::string colname = CorelliCalibration::calibrationTableColumnNames[i];
      std::string type = CorelliCalibration::calibrationTableColumnTypes[i];
      tablews->addColumn(type, colname);
    }

    std::cout << "[DEBUG 0] Table workspace rows: " << tablews->rowCount() << "\n";

    // append rows
    Mantid::API::TableRow sourceRow = tablews->appendRow();
    sourceRow << "moderator" << 0. << 0. << -15.560 << 0. << 0. << 0. << 0.;
    Mantid::API::TableRow sampleRow = tablews->appendRow();
    sampleRow << "sample-position" << 0.0001 << -0.0002 << 0.003 << 0. << 0. << 0. << 0.;
    Mantid::API::TableRow bank1Row = tablews->appendRow();
    bank1Row << "bank1/sixteenpack" << 0.9678 << 0.0056 << 0.0003 << 0.4563 << -0.9999 << 0.3424 << 5.67;

    std::cout << "[DEBUG 1] Table workspace rows: " << tablews->rowCount() << "\n";

    return tablews;
  }

  /**
   * @brief Create an incompatile table workspace for algorithm to throw
   * exception
   * @param outWSName
   * @return
   */
  TableWorkspace_sptr createIncorrectTestCalibrationTableWorkspace(const std::string &outWSName) {
    // Create table workspace
    ITableWorkspace_sptr itablews = WorkspaceFactory::Instance().createTable();
    AnalysisDataService::Instance().addOrReplace(outWSName, itablews);

    TableWorkspace_sptr tablews = std::dynamic_pointer_cast<TableWorkspace>(itablews);
    TS_ASSERT(tablews);

    // Set up columns
    for (size_t i = 0; i < CorelliCalibration::calibrationTableColumnNames.size() - 1; ++i) {
      std::string colname = CorelliCalibration::calibrationTableColumnNames[i];
      std::string type = CorelliCalibration::calibrationTableColumnTypes[i];
      tablews->addColumn(type, colname);
    }

    // append rows
    Mantid::API::TableRow sourceRow = tablews->appendRow();
    sourceRow << "moderator" << 0. << 0. << -15.560 << 0. << 0. << 0.;
    Mantid::API::TableRow sampleRow = tablews->appendRow();
    sampleRow << "sample-position" << 0.0001 << -0.0002 << 0.003 << 0. << 0. << 0.;
    Mantid::API::TableRow bank1Row = tablews->appendRow();
    bank1Row << "bank1" << 0.9678 << 0.0056 << 0.0003 << 0.4563 << -0.9999 << 0.3424;

    return tablews;
  }

  TableWorkspace_sptr loadCSVtoTable(const std::string &csvname, const std::string &tablewsname) {
    auto loadAsciiAlg = AlgorithmFactory::Instance().create("LoadAscii", 2);
    loadAsciiAlg->initialize();
    loadAsciiAlg->setPropertyValue("Filename", csvname);
    loadAsciiAlg->setPropertyValue("OutputWorkspace", tablewsname);
    loadAsciiAlg->setPropertyValue("Separator", "CSV");
    loadAsciiAlg->setPropertyValue("CommentIndicator", "#");
    loadAsciiAlg->execute();

    TableWorkspace_sptr tablews =
        std::dynamic_pointer_cast<TableWorkspace>(AnalysisDataService::Instance().retrieve(tablewsname));

    return tablews;
  }

  /**
   * @brief Create some existing database (csv) files
   */
  void create_existing_database_files(const std::string &calibdir, std::vector<std::string> &banks) {

    std::filesystem::path dir(calibdir);

    for (auto bankname : banks) {
      // create full path database name
      std::string basename = bankname + ".csv";
      std::filesystem::path basepath(basename);
      std::filesystem::path fullpath = dir / basename;
      std::string filename = fullpath.string();
      // write file
      std::ofstream bankofs(filename, std::ofstream::out);
      bankofs << "# YYYMMDD , Xposition , Yposition , Zposition , XdirectionCosine "
                 ", YdirectionCosine , ZdirectionCosine , RotationAngle\n";
      bankofs << "# str , double , double , double , double , double , double "
                 ", double\n";
      bankofs << "20120321,0.0001,-0.0002,0.003,0,-23.3,98.02,0";
      bankofs.close();
    }
  }

  /**
   * @brief verify single component file existence and check some specific value
   * @param calfiledir
   * @param component
   * @param expectedrecordsnumber
   */
  void verify_component_files(const std::string &calfiledir, const std::string &component,
                              size_t expectedrecordsnumber) {
    // Create full file path
    std::filesystem::path pdir(calfiledir);
    std::filesystem::path pbase(component + ".csv");
    std::filesystem::path pcompcalfile = pdir / pbase;
    std::string compcalfile = pcompcalfile.string();

    // Assert file existence
    TS_ASSERT(std::filesystem::exists(compcalfile));

    // Load table
    TableWorkspace_sptr tablews = loadCSVtoTable(compcalfile, "CorelliVerify_" + component);

    // Check records
    TS_ASSERT_EQUALS(tablews->rowCount(), expectedrecordsnumber);
  }
};
