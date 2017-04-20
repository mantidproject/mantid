#ifndef MANTID_ALGORITHMS_PDCALIBRATIONTEST_H_
#define MANTID_ALGORITHMS_PDCALIBRATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/PDCalibration.h"
#include "MantidDataObjects/TableColumn.h"

using Mantid::Algorithms::PDCalibration;
using Mantid::API::Workspace_sptr;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_const_sptr;
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::FrameworkManager;
using Mantid::API::AnalysisDataService;
using Mantid::Algorithms::CreateSampleWorkspace;

namespace {
/**
* Creates a workspace with peaks at 400, 800, 1300, 1600 us
*/
void createSampleWS() {
  CreateSampleWorkspace sampleWS;
  sampleWS.initialize();
  sampleWS.setPropertyValue("WorkspaceType", "Event");
  sampleWS.setPropertyValue("Function", "User Defined");
  sampleWS.setPropertyValue(
      "UserDefinedFunction",
      "name=Gaussian,Height=100,PeakCentre=400,Sigma=10;"
      "name=Gaussian,Height=80,PeakCentre=800,Sigma=12;"
      "name=Gaussian,Height=350,PeakCentre=1300,Sigma=12;"
      "name=Gaussian,Height=210,PeakCentre=1600,Sigma=15");
  sampleWS.setProperty("XMin", 100.0);
  sampleWS.setProperty("XMax", 2000.0);
  sampleWS.setProperty("BinWidth", 1.0);
  sampleWS.setPropertyValue("OutputWorkspace", "PDCalibrationTest_WS");
  sampleWS.execute();
}
}

class PDCalibrationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PDCalibrationTest *createSuite() { return new PDCalibrationTest(); }
  static void destroySuite(PDCalibrationTest *suite) { delete suite; }

  PDCalibrationTest() { FrameworkManager::Instance(); }

  void setUp() override { createSampleWS(); }

  void test_Init() {
    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec_difc() {
    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("SignalWorkspace", "PDCalibrationTest_WS"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("TofBinning", "200,1.0,2000"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeakWindow", "1"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputCalibrationTable", "cal"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeakPositions",
                                                  "9.523809, 22.222222, "
                                                  "38.095238, 47.619047"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("cal");

    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    size_t index =
        std::find(detIDs.begin(), detIDs.end(), 280) - detIDs.begin();
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), 31.5, 0.05); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);         // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);         // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 281) - detIDs.begin();
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), 31.5, 0.05); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);         // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);         // tzero

    MatrixWorkspace_const_sptr mask =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("cal_mask");

    TS_ASSERT_EQUALS(mask->y(179)[0], 1);
    TS_ASSERT_EQUALS(mask->y(180)[0], 0);
    TS_ASSERT_EQUALS(mask->y(181)[0], 0);
    TS_ASSERT_EQUALS(mask->y(182)[0], 0);
    TS_ASSERT_EQUALS(mask->y(183)[0], 1);
  }

  void test_exec_difc_tzero() {
    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("SignalWorkspace", "PDCalibrationTest_WS"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("TofBinning", "200,1.0,2000"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeakWindow", "1"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputCalibrationTable", "cal"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeakPositions",
                                                  "9.476190, 22.174603, "
                                                  "38.047619, 47.571429"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("CalibrationParameters", "DIFC+TZERO"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("cal");

    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    size_t index =
        std::find(detIDs.begin(), detIDs.end(), 256) - detIDs.begin();
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), 31.5, 0.01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);         // difa
    TS_ASSERT_DELTA(calTable->cell<double>(index, 3), 2, 0.01);    // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 265) - detIDs.begin();
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), 31.5, 0.01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);         // difa
    TS_ASSERT_DELTA(calTable->cell<double>(index, 3), 2, 0.01);    // tzero

    MatrixWorkspace_const_sptr mask =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("cal_mask");

    TS_ASSERT_EQUALS(mask->y(155)[0], 1);
    TS_ASSERT_EQUALS(mask->y(156)[0], 0);
    TS_ASSERT_EQUALS(mask->y(157)[0], 1);
    TS_ASSERT_EQUALS(mask->y(158)[0], 1);
    TS_ASSERT_EQUALS(mask->y(159)[0], 1);
  }

  void test_exec_difc_tzero_difa() {
    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("SignalWorkspace", "PDCalibrationTest_WS"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("TofBinning", "200,1.0,2000"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeakWindow", "2"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputCalibrationTable", "cal"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeakPositions",
                                                  "9.207078, 20.801010,"
                                                  "34.310453, 41.977442"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("CalibrationParameters", "DIFC+TZERO+DIFA"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("cal");

    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    size_t index =
        std::find(detIDs.begin(), detIDs.end(), 282) - detIDs.begin();
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), 35.9, 0.1);  // difc
    TS_ASSERT_DELTA(calTable->cell<double>(index, 2), 0.0, 0.01);  // difa
    TS_ASSERT_DELTA(calTable->cell<double>(index, 3), -35.4, 0.1); // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 283) - detIDs.begin();
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), 31.5, 0.1); // difc
    TS_ASSERT_DELTA(calTable->cell<double>(index, 2), 0.1, 0.01); // difa
    TS_ASSERT_DELTA(calTable->cell<double>(index, 3), 2, 0.2);    // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 284) - detIDs.begin();
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), 31.5, 0.1); // difc
    TS_ASSERT_DELTA(calTable->cell<double>(index, 2), 0.1, 0.01); // difa
    TS_ASSERT_DELTA(calTable->cell<double>(index, 3), 2, 0.2);    // tzero

    MatrixWorkspace_const_sptr mask =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("cal_mask");

    TS_ASSERT_EQUALS(mask->y(182)[0], 0);
    TS_ASSERT_EQUALS(mask->y(183)[0], 0);
    TS_ASSERT_EQUALS(mask->y(184)[0], 0);
    TS_ASSERT_EQUALS(mask->y(185)[0], 0);
    TS_ASSERT_EQUALS(mask->y(186)[0], 1);
  }
};

class PDCalibrationTestPerformance : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PDCalibrationTestPerformance *createSuite() {
    return new PDCalibrationTestPerformance();
  }

  static void destroySuite(PDCalibrationTestPerformance *suite) {
    delete suite;
  }

  PDCalibrationTestPerformance() { FrameworkManager::Instance(); }

  void setUp() override {
    createSampleWS();
    pdc.initialize();
    pdc.setProperty("SignalWorkspace", "PDCalibrationTest_WS");
    pdc.setPropertyValue("TofBinning", "200,1.0,2000");
    pdc.setPropertyValue("PeakWindow", "1");
    pdc.setPropertyValue("OutputCalibrationTable", "outputWS");
    pdc.setPropertyValue("PeakPositions", "9.523809, 22.222222, "
                                          "38.095238, 47.619047");
  }

  void tearDown() override {
    Mantid::API::AnalysisDataService::Instance().remove("outputWS");
  }

  void testPerformanceWS() { pdc.execute(); }

private:
  PDCalibration pdc;
};

#endif /* MANTID_ALGORITHMS_PDCALIBRATIONTEST_H_ */
