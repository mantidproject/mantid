#ifndef MANTID_ALGORITHMS_PDCALIBRATIONTEST_H_
#define MANTID_ALGORITHMS_PDCALIBRATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/PDCalibration.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"

using Mantid::Algorithms::PDCalibration;
using Mantid::API::Workspace_sptr;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_const_sptr;
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::FrameworkManager;
using Mantid::API::AnalysisDataService;
using Mantid::Algorithms::CreateSampleWorkspace;

class PDCalibrationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PDCalibrationTest *createSuite() { return new PDCalibrationTest(); }
  static void destroySuite(PDCalibrationTest *suite) { delete suite; }

  PDCalibrationTest() { FrameworkManager::Instance(); }

  void setUp() override {
    /**
     * Creates a workspace with peaks at 400, 800, 1300, 1600 us
     */
    CreateSampleWorkspace create;
    create.initialize();
    create.setPropertyValue("WorkspaceType", "Event");
    create.setPropertyValue("Function", "User Defined");
    create.setPropertyValue(
        "UserDefinedFunction",
        "name=Gaussian,Height=100,PeakCentre=400,Sigma=10;"
        "name=Gaussian,Height=80,PeakCentre=800,Sigma=12;"
        "name=Gaussian,Height=350,PeakCentre=1300,Sigma=12;"
        "name=Gaussian,Height=210,PeakCentre=1600,Sigma=15");
    create.setProperty("XMin", 100.0);
    create.setProperty("XMax", 2000.0);
    create.setProperty("BinWidth", 1.0);
    create.setPropertyValue("OutputWorkspace", "PDCalibrationTest_WS");
    create.execute();
  }

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
        alg.setProperty("UncalibratedWorkspace", "PDCalibrationTest_WS"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("TofBinning", "200,1.0,2000"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("PeakWindow", "1"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputCalibrationTable", "cal"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "PeakPositions", "9.523809523809524, 22.22222222222222, "
                         "38.095238095238095, 47.61904761904762"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable =
      AnalysisDataService::Instance().retrieveWS<ITableWorkspace>("cal");

    TS_ASSERT(calTable);
    TS_ASSERT_DELTA(calTable->cell<double>(180, 1), 31.5, 0.05); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(180, 2), 0); // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(180, 3), 0); // tzero
    TS_ASSERT_DELTA(calTable->cell<double>(181, 1), 31.5, 0.05); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(181, 2), 0); // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(181, 3), 0); // tzero
    TS_ASSERT_DELTA(calTable->cell<double>(182, 1), 31.5, 0.05); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(182, 2), 0); // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(182, 3), 0); // tzero

    MatrixWorkspace_const_sptr mask =
      AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("cal_mask");

    //TS_ASSERT(mask->getInstrument()->getDetector(279)->isMasked());
    TS_ASSERT(!mask->getInstrument()->getDetector(280)->isMasked());
    TS_ASSERT(!mask->getInstrument()->getDetector(281)->isMasked());
    TS_ASSERT(!mask->getInstrument()->getDetector(282)->isMasked());
    //TS_ASSERT(mask->getInstrument()->getDetector(283)->isMasked());
  }
};

#endif /* MANTID_ALGORITHMS_PDCALIBRATIONTEST_H_ */
