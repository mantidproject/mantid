#ifndef MANTID_ALGORITHMS_PDCALIBRATIONTEST_H_
#define MANTID_ALGORITHMS_PDCALIBRATIONTEST_H_

#include <algorithm>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/CreateSampleWorkspace.h"
#include "MantidAlgorithms/PDCalibration.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include "MantidDataHandling/RotateInstrumentComponent.h"
#include "MantidDataObjects/TableColumn.h"
#include "MantidKernel/Diffraction.h"

using Mantid::Algorithms::PDCalibration;
using Mantid::API::Workspace_sptr;
using Mantid::API::MatrixWorkspace;
using Mantid::API::MatrixWorkspace_const_sptr;
using Mantid::API::ITableWorkspace;
using Mantid::API::ITableWorkspace_sptr;
using Mantid::API::FrameworkManager;
using Mantid::API::AnalysisDataService;
using Mantid::Algorithms::CreateSampleWorkspace;
using Mantid::DataHandling::MoveInstrumentComponent;
using Mantid::DataHandling::RotateInstrumentComponent;

namespace {
// -- constants for creating the input event workspace

// detID 155 is the middle at r,theta,phi = 5,90,0; DIFC = 5362.24
const double DIFC_155 = 5362.24;
const size_t WKSPINDEX_155 = 55; // spectrum number 56
// detID 195 is off to the side at r,theta,phi = 5.00063995,90,9166542; DIFC =
// 5405.21
const double DIFC_195 = 5405.21;
const size_t WKSPINDEX_195 = 95; // spectrum number 96

const double TOF_MIN = 300.; // first frame for 60Hz source
const double TOF_MAX = 16666.7;
const std::vector<double> TOF_BINNING = {TOF_MIN, 1., TOF_MAX};
// "Powder Diffraction" function makes 9 values of varying height and width that
// are equally spaced
const double PEAK_TOF_DELTA = (TOF_MAX - TOF_MIN) / 10.;

// there is a systematic shift where all found peaks are at too
// high of TOF by a few microseconds the first values are where
// CreateSampleWorkspace puts the peaks, the second is where
// FindPeaks locates them
// const std::vector<double> PEAK_TOFS = {1635.7, 3271.4, 4907.1, 6542.8,
// 8178.5,
//                                       9814.2, 11449.9, 13085.6, 14721.3};
const std::vector<double> PEAK_TOFS = {1636.5, 3272.5,  4908.5,  6544.5, 8180.5,
                                       9816.5, 11452.5, 13088.5, 14724.5};

/**
* Creates a workspace with peaks at 400, 800, 1300, 1600 us
*/
void createSampleWS() {
  // all values are at the same TOF so calibrations will be the same with
  // different starting guesses

  CreateSampleWorkspace createSampleWS;
  createSampleWS.initialize();
  createSampleWS.setPropertyValue("WorkspaceType", "Event");
  createSampleWS.setPropertyValue("Function", "Powder Diffraction");
  createSampleWS.setProperty("XMin", TOF_MIN); // first frame
  createSampleWS.setProperty("XMax", TOF_MAX);
  createSampleWS.setProperty("BinWidth", 1.); // micro-seconds
  createSampleWS.setProperty("NumBanks", 1);  // detIds = [100,200)
  createSampleWS.setProperty("NumEvents", 100000);
  createSampleWS.setProperty("PixelSpacing", .02); // 2cm pixels
  createSampleWS.setPropertyValue("OutputWorkspace", "PDCalibrationTest_WS");
  createSampleWS.execute();

  // move it to the right place - DIFC of this location vary from 5308 to 5405
  RotateInstrumentComponent rotateInstr;
  rotateInstr.initialize();
  rotateInstr.setPropertyValue("Workspace", "PDCalibrationTest_WS");
  rotateInstr.setPropertyValue("ComponentName", "bank1");
  rotateInstr.setProperty("Y", 1.);
  rotateInstr.setProperty("Angle", 90.);
  rotateInstr.execute();
  MoveInstrumentComponent moveInstr;
  moveInstr.initialize();
  moveInstr.setPropertyValue("Workspace", "PDCalibrationTest_WS");
  moveInstr.setPropertyValue("ComponentName", "bank1");
  moveInstr.setProperty("X", 5.);
  moveInstr.setProperty("Y", -.1);
  moveInstr.setProperty("Z", .1);
  moveInstr.setProperty("RelativePosition", false);
  moveInstr.execute();
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

  void checkDSpacing(const std::string &wsname,
                     const std::vector<double> &dValues) {
    ITableWorkspace_sptr peaksTable =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(wsname);
    Mantid::DataObjects::TableColumn_ptr<int> col0 = peaksTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    // check for workspace index 55 which is spectrum 56
    size_t index =
        std::find(detIDs.begin(), detIDs.end(), 155) - detIDs.begin();
    for (size_t i = 0; i < dValues.size(); ++i) {
      TS_ASSERT_DELTA(peaksTable->cell<double>(index, 1 + i), dValues[i],
                      0.0002);
    }
    // checks for chisq, first one is strange because of test framework missing
    // > operator
    TS_ASSERT_LESS_THAN(0.,
                        peaksTable->cell<double>(index, 1 + dValues.size()));
    TS_ASSERT_LESS_THAN(peaksTable->cell<double>(index, 1 + dValues.size()),
                        10.);

    // check for workspace index 95 which is spectrum 96 - last peak is out of
    // range???
    index = std::find(detIDs.begin(), detIDs.end(), 195) - detIDs.begin();
    for (size_t i = 0; i < dValues.size() - 1; ++i) {
      TS_ASSERT_DELTA(peaksTable->cell<double>(index, 1 + i), dValues[i],
                      0.0002);
    }
    // checks for chisq, first one is strange because of test framework missing
    // > operator
    TS_ASSERT_LESS_THAN(0.,
                        peaksTable->cell<double>(index, 1 + dValues.size()));
    TS_ASSERT_LESS_THAN(peaksTable->cell<double>(index, 1 + dValues.size()),
                        10.);
  }

  void test_exec_difc() {
    // setup the peak postions based on transformation from detID=155
    std::vector<double> dValues(PEAK_TOFS.size());
    std::transform(
        PEAK_TOFS.begin(), PEAK_TOFS.end(), dValues.begin(),
        Mantid::Kernel::Diffraction::getTofToDConversionFunc(DIFC_155, 0., 0.));

    const std::string prefix{"PDCalibration_difc"};

    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspace", "PDCalibrationTest_WS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TofBinning", TOF_BINNING));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputCalibrationTable", prefix + "cal"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("DiagnosticWorkspaces", prefix + "diag"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakPositions", dValues));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(prefix +
                                                                    "cal");

    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    // since the wksp was calculated in TOF, all DIFC end up being the same
    size_t index =
        std::find(detIDs.begin(), detIDs.end(), 155) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 155);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 195) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 195);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, .01); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 3), 0);            // tzero

    MatrixWorkspace_const_sptr mask =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(prefix +
                                                                    "cal_mask");
    // 0 is keep
    TS_ASSERT_EQUALS(mask->y(WKSPINDEX_155)[0], 0);
    TS_ASSERT_EQUALS(mask->y(WKSPINDEX_195)[0], 0);

    checkDSpacing(prefix + "diag_dspacing", dValues);

    Mantid::API::AnalysisDataService::Instance().remove(prefix + "cal");
    Mantid::API::AnalysisDataService::Instance().remove(prefix + "cal_mask");
  }

  void test_exec_difc_tzero() {
    // setup the peak postions based on transformation from detID=155
    const double TZERO = 20.;
    std::vector<double> dValues(PEAK_TOFS.size());
    std::transform(PEAK_TOFS.begin(), PEAK_TOFS.end(), dValues.begin(),
                   Mantid::Kernel::Diffraction::getTofToDConversionFunc(
                       DIFC_155, 0., TZERO));

    const std::string prefix{"PDCalibration_difc_tzero"};

    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspace", "PDCalibrationTest_WS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TofBinning", TOF_BINNING));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputCalibrationTable", prefix + "cal"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("DiagnosticWorkspaces", prefix + "diag"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakPositions", dValues));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("CalibrationParameters", "DIFC+TZERO"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(prefix +
                                                                    "cal");

    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    // since the wksp was calculated in TOF, all DIFC end up being the same
    size_t index =
        std::find(detIDs.begin(), detIDs.end(), 155) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 155);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, 0.1); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_DELTA(calTable->cell<double>(index, 3), TZERO, 0.1);    // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 195) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 195);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, 0.1); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_DELTA(calTable->cell<double>(index, 3), TZERO, 0.1);    // tzero

    MatrixWorkspace_const_sptr mask =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(prefix +
                                                                    "cal_mask");

    // 0 is keep
    TS_ASSERT_EQUALS(mask->y(WKSPINDEX_155)[0], 0);
    TS_ASSERT_EQUALS(mask->y(WKSPINDEX_195)[0], 0);

    checkDSpacing(prefix + "diag_dspacing", dValues);

    Mantid::API::AnalysisDataService::Instance().remove(prefix + "cal");
    Mantid::API::AnalysisDataService::Instance().remove(prefix + "cal_mask");
  }

  void test_exec_difc_tzero_difa() {
    // setup the peak postions based on transformation from detID=155
    // allow refining DIFA, but don't set the transformation to require it
    const double TZERO = 20.;
    std::vector<double> dValues(PEAK_TOFS.size());
    std::transform(PEAK_TOFS.begin(), PEAK_TOFS.end(), dValues.begin(),
                   Mantid::Kernel::Diffraction::getTofToDConversionFunc(
                       DIFC_155, 0., TZERO));

    const std::string prefix{"PDCalibration_difc_tzero_difa"};

    PDCalibration alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("InputWorkspace", "PDCalibrationTest_WS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TofBinning", TOF_BINNING));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputCalibrationTable", prefix + "cal"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("DiagnosticWorkspaces", prefix + "diag"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakPositions", dValues));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("CalibrationParameters", "DIFC+TZERO+DIFA"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr calTable =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(prefix +
                                                                    "cal");

    TS_ASSERT(calTable);

    Mantid::DataObjects::TableColumn_ptr<int> col0 = calTable->getColumn(0);
    std::vector<int> detIDs = col0->data();

    // since the wksp was calculated in TOF, all DIFC end up being the same
    size_t index =
        std::find(detIDs.begin(), detIDs.end(), 155) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 155);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, 0.1); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_DELTA(calTable->cell<double>(index, 3), TZERO, 0.1);    // tzero

    index = std::find(detIDs.begin(), detIDs.end(), 195) - detIDs.begin();
    TS_ASSERT_EQUALS(calTable->cell<int>(index, 0), 195);             // detid
    TS_ASSERT_DELTA(calTable->cell<double>(index, 1), DIFC_155, 0.1); // difc
    TS_ASSERT_EQUALS(calTable->cell<double>(index, 2), 0);            // difa
    TS_ASSERT_DELTA(calTable->cell<double>(index, 3), TZERO, 0.1);    // tzero

    MatrixWorkspace_const_sptr mask =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(prefix +
                                                                    "cal_mask");

    // 0 is keep
    TS_ASSERT_EQUALS(mask->y(WKSPINDEX_155)[0], 0);
    TS_ASSERT_EQUALS(mask->y(WKSPINDEX_195)[0], 0);

    checkDSpacing(prefix + "diag_dspacing", dValues);

    Mantid::API::AnalysisDataService::Instance().remove(prefix + "cal");
    Mantid::API::AnalysisDataService::Instance().remove(prefix + "cal_mask");
  }
};

class PDCalibrationTestPerformance : public CxxTest::TestSuite { // TODO

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
    // setup the peak postions based on transformation from detID=155
    std::vector<double> dValues(PEAK_TOFS.size());
    std::transform(
        PEAK_TOFS.begin(), PEAK_TOFS.end(), dValues.begin(),
        Mantid::Kernel::Diffraction::getTofToDConversionFunc(DIFC_155, 0., 0.));
    createSampleWS();
    pdc.initialize();
    pdc.setProperty("InputWorkspace", "PDCalibrationTest_WS");
    pdc.setProperty("TofBinning", TOF_BINNING);
    pdc.setPropertyValue("OutputCalibrationTable", "outputWS");
    pdc.setPropertyValue("DiagnosticWorkspaces", "diag");
    pdc.setProperty("PeakPositions", dValues);
  }

  void tearDown() override {
    Mantid::API::AnalysisDataService::Instance().remove("outputWS");
    Mantid::API::AnalysisDataService::Instance().remove("diag");
  }

  void testPerformanceWS() { pdc.execute(); }

private:
  PDCalibration pdc;
};

#endif /* MANTID_ALGORITHMS_PDCALIBRATIONTEST_H_ */
